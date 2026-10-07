#include "frontend.hpp"
#include "common.hpp"

#include <fstream>
#include <regex>
#include <sstream>
#include <cstdlib>

namespace lyra {
namespace {

struct BreakSignal {};
struct ContinueSignal {};

std::string stripComment(const std::string& input) {
    bool quoted = false;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '"' && (i == 0 || input[i - 1] != '\\')) quoted = !quoted;
        if (!quoted && input[i] == '#' &&
            (i == 0 || std::isspace(static_cast<unsigned char>(input[i - 1]))))
            return trim(input.substr(0, i));
    }
    return trim(input);
}

std::vector<std::string> splitComma(const std::string& text) {
    std::vector<std::string> out;
    std::string current;
    int depth = 0;
    bool quoted = false;
    for (char c : text) {
        if (c == '"') quoted = !quoted;
        if (!quoted && c == '(') ++depth;
        if (!quoted && c == ')') --depth;
        if (!quoted && c == ',' && depth == 0) {
            out.push_back(trim(current));
            current.clear();
        } else current += c;
    }
    if (!trim(current).empty()) out.push_back(trim(current));
    return out;
}

class Expression {
public:
    Expression(std::string text, const std::map<std::string, std::string>& values)
        : text_(std::move(text)), values_(values) {}

    double parse() {
        pos_ = 0;
        double value = logicalOr();
        skip();
        if (pos_ != text_.size()) throw std::runtime_error("Invalid expression near: " + text_.substr(pos_));
        return value;
    }

private:
    std::string text_;
    const std::map<std::string, std::string>& values_;
    size_t pos_ = 0;

    void skip() { while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) ++pos_; }
    bool take(const std::string& token) {
        skip();
        if (text_.compare(pos_, token.size(), token) == 0) { pos_ += token.size(); return true; }
        return false;
    }
    double logicalOr() {
        double value = logicalAnd();
        while (take("||")) value = (value != 0.0 || logicalAnd() != 0.0) ? 1.0 : 0.0;
        return value;
    }
    double logicalAnd() {
        double value = equality();
        while (take("&&")) value = (value != 0.0 && equality() != 0.0) ? 1.0 : 0.0;
        return value;
    }
    double equality() {
        double value = comparison();
        while (true) {
            if (take("==")) value = value == comparison() ? 1.0 : 0.0;
            else if (take("!=")) value = value != comparison() ? 1.0 : 0.0;
            else return value;
        }
    }
    double comparison() {
        double value = term();
        while (true) {
            if (take("<=")) value = value <= term() ? 1.0 : 0.0;
            else if (take(">=")) value = value >= term() ? 1.0 : 0.0;
            else if (take("<")) value = value < term() ? 1.0 : 0.0;
            else if (take(">")) value = value > term() ? 1.0 : 0.0;
            else return value;
        }
    }
    double term() {
        double value = product();
        while (true) {
            if (take("+")) value += product();
            else if (take("-")) value -= product();
            else return value;
        }
    }
    double product() {
        double value = unary();
        while (true) {
            if (take("*")) value *= unary();
            else if (take("/")) {
                double rhs = unary();
                if (rhs == 0.0) throw std::runtime_error("Division by zero");
                value /= rhs;
            } else if (take("%")) {
                double rhs = unary();
                if (rhs == 0.0) throw std::runtime_error("Modulo by zero");
                value = std::fmod(value, rhs);
            } else return value;
        }
    }
    double unary() {
        if (take("!")) return unary() == 0.0 ? 1.0 : 0.0;
        if (take("-")) return -unary();
        if (take("+")) return unary();
        return primary();
    }
    double primary() {
        skip();
        if (take("(")) {
            double value = logicalOr();
            if (!take(")")) throw std::runtime_error("Missing ')' in expression");
            return value;
        }
        if (pos_ < text_.size() && (std::isdigit(static_cast<unsigned char>(text_[pos_])) || text_[pos_] == '.')) {
            size_t used = 0;
            double value = std::stod(text_.substr(pos_), &used);
            pos_ += used;
            return value;
        }
        if (pos_ < text_.size() && (std::isalpha(static_cast<unsigned char>(text_[pos_])) || text_[pos_] == '_')) {
            size_t start = pos_++;
            while (pos_ < text_.size() && (std::isalnum(static_cast<unsigned char>(text_[pos_])) || text_[pos_] == '_')) ++pos_;
            std::string name = text_.substr(start, pos_ - start);
            if (name == "true") return 1.0;
            if (name == "false") return 0.0;
            auto it = values_.find(name);
            if (it == values_.end()) throw std::runtime_error("Unknown variable in expression: " + name);
            return std::stod(it->second);
        }
        throw std::runtime_error("Expected value in expression");
    }
};

std::string numberText(double value) {
    std::ostringstream out;
    out.precision(12);
    out << value;
    return out.str();
}

std::string substitute(std::string line, const std::map<std::string, std::string>& values) {
    size_t pos = 0;
    while ((pos = line.find("${", pos)) != std::string::npos) {
        size_t end = line.find('}', pos + 2);
        if (end == std::string::npos) throw std::runtime_error("Unclosed ${expression}");
        std::string expr = line.substr(pos + 2, end - pos - 2);
        std::string value = numberText(Expression(expr, values).parse());
        line.replace(pos, end - pos + 1, value);
        pos += value.size();
    }
    pos = 0;
    while ((pos = line.find('$', pos)) != std::string::npos) {
        size_t start = pos + 1;
        size_t end = start;
        while (end < line.size() && (std::isalnum(static_cast<unsigned char>(line[end])) || line[end] == '_')) ++end;
        if (end == start) { ++pos; continue; }
        std::string name = line.substr(start, end - start);
        auto it = values.find(name);
        if (it == values.end()) throw std::runtime_error("Unknown variable: " + name);
        line.replace(pos, end - pos, it->second);
        pos += it->second.size();
    }
    return line;
}

std::pair<std::vector<std::string>, size_t> collectBlock(const std::vector<std::string>& lines, size_t start) {
    if (lines[start].find('{') == std::string::npos) throw std::runtime_error("Expected '{'");
    std::vector<std::string> body;
    int depth = 1;
    for (size_t i = start + 1; i < lines.size(); ++i) {
        std::string clean = stripComment(lines[i]);
        for (char c : clean) {
            if (c == '{') ++depth;
            if (c == '}') --depth;
        }
        if (depth == 0) return {body, i};
        body.push_back(lines[i]);
    }
    throw std::runtime_error("Unclosed block starting with: " + trim(lines[start]));
}

std::string transposeNote(const std::string& note, int semitones) {
    if (semitones == 0) return note;
    static const std::map<std::string, int> pitch = {
        {"c",0},{"c#",1},{"db",1},{"d",2},{"d#",3},{"eb",3},{"e",4},{"f",5},
        {"f#",6},{"gb",6},{"g",7},{"g#",8},{"ab",8},{"a",9},{"a#",10},{"bb",10},{"b",11}
    };
    static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    std::string lower = toLower(note);
    size_t split = 1;
    if (lower.size() > 1 && (lower[1] == '#' || lower[1] == 'b')) split = 2;
    auto it = pitch.find(lower.substr(0, split));
    if (it == pitch.end() || split >= lower.size()) return note;
    int octave = std::stoi(lower.substr(split));
    int midi = (octave + 1) * 12 + it->second + semitones;
    if (midi < 0 || midi > 119) throw std::runtime_error("Transposed note is outside Lyra range: " + note);
    return std::string(names[midi % 12]) + std::to_string(midi / 12 - 1);
}

std::string applyTranspose(const std::string& line, int semitones) {
    if (semitones == 0) return line;
    std::istringstream in(line);
    std::vector<std::string> tokens;
    std::string token;
    while (in >> token) tokens.push_back(token);
    if (tokens.empty()) return line;
    std::string cmd = toLower(tokens[0]);
    if (cmd == "note" && tokens.size() >= 2) tokens[1] = transposeNote(tokens[1], semitones);
    else if (cmd == "chord" && tokens.size() >= 3) {
        for (size_t i = 1; i + 1 < tokens.size(); ++i) tokens[i] = transposeNote(tokens[i], semitones);
    } else return line;
    std::ostringstream out;
    for (size_t i = 0; i < tokens.size(); ++i) { if (i) out << ' '; out << tokens[i]; }
    return out.str();
}

} // namespace

std::string Frontend::processFile(const std::string& filename) {
    variables_.clear(); lists_.clear(); constants_.clear(); patterns_.clear(); functions_.clear(); scalarFunctions_.clear();
    instruments_.clear(); effects_.clear(); importStack_.clear(); imported_.clear();
    std::vector<std::string> output;
    try {
        output = processPath(std::filesystem::absolute(filename));
    } catch (const BreakSignal&) {
        throw std::runtime_error("break used outside a Lyra 3 loop");
    } catch (const ContinueSignal&) {
        throw std::runtime_error("continue used outside a Lyra 3 loop");
    }
    std::ostringstream joined;
    for (const auto& line : output) joined << line << '\n';
    return joined.str();
}

std::vector<std::string> Frontend::processPath(const std::filesystem::path& rawPath) {
    // Do not require filesystem canonicalisation here.  On sandboxed Windows
    // installations weakly_canonical can reject a perfectly readable source
    // file while resolving one of its parents.  An absolute, normalised path
    // is stable enough for import identity and also preserves useful errors.
    std::filesystem::path path = std::filesystem::absolute(rawPath).lexically_normal();
    if (importStack_.count(path)) throw std::runtime_error("Circular import: " + path.string());
    if (imported_.count(path)) return {};
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot import: " + path.string());
    importStack_.insert(path);
    imported_.insert(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (std::regex_match(stripComment(line), std::regex(R"(^\}\s*else\s*\{$)", std::regex::icase))) {
            lines.push_back("}");
            lines.push_back("else {");
        } else lines.push_back(line);
    }
    std::vector<std::string> output;
    try {
        output = expand(lines, path.parent_path());
    } catch (const std::runtime_error& error) {
        importStack_.erase(path);
        throw std::runtime_error(path.string() + ": " + error.what());
    }
    importStack_.erase(path);
    return output;
}

std::string Frontend::resolveListAccess(std::string text,
                                        const std::map<std::string, std::string>& values) const {
    std::regex access(R"(\$([A-Za-z_]\w*)\[([^\]]+)\])");
    std::smatch match;
    while (std::regex_search(text, match, access)) {
        auto list = lists_.find(match[1].str());
        if (list == lists_.end()) throw std::runtime_error("Unknown list: " + match[1].str());
        std::string indexExpr = substitute(match[2].str(), values);
        int index = static_cast<int>(std::lround(Expression(indexExpr, values).parse()));
        if (index < 0) index += static_cast<int>(list->second.size());
        if (index < 0 || static_cast<size_t>(index) >= list->second.size())
            throw std::runtime_error("List index out of range: " + match[1].str());
        text.replace(match.position(), match.length(), list->second[static_cast<size_t>(index)]);
    }
    return text;
}

std::string Frontend::resolveScalarCalls(std::string text,
                                         const std::map<std::string, std::string>& values,
                                         int depth) const {
    if (depth > 32) throw std::runtime_error("Function recursion limit exceeded");
    std::regex lengthCall(R"(len\s*\(\s*([A-Za-z_]\w*)\s*\))", std::regex::icase);
    std::smatch lengthMatch;
    while (std::regex_search(text, lengthMatch, lengthCall)) {
        auto list = lists_.find(lengthMatch[1].str());
        if (list == lists_.end()) throw std::runtime_error("len() expects a list: " + lengthMatch[1].str());
        text.replace(lengthMatch.position(), lengthMatch.length(), std::to_string(list->second.size()));
    }
    bool replaced = true;
    while (replaced) {
        replaced = false;
        for (const auto& entry : scalarFunctions_) {
            const std::string needle = entry.first + "(";
            size_t pos = text.find(needle);
            if (pos == std::string::npos || (pos > 0 &&
                (std::isalnum(static_cast<unsigned char>(text[pos - 1])) || text[pos - 1] == '_'))) continue;
            size_t cursor = pos + needle.size();
            int parenDepth = 1;
            for (; cursor < text.size() && parenDepth > 0; ++cursor) {
                if (text[cursor] == '(') ++parenDepth;
                else if (text[cursor] == ')') --parenDepth;
            }
            if (parenDepth != 0) throw std::runtime_error("Unclosed function call: " + entry.first);
            size_t close = cursor - 1;
            auto args = splitComma(text.substr(pos + needle.size(), close - (pos + needle.size())));
            if (args.size() != entry.second.params.size())
                throw std::runtime_error("Wrong argument count for function: " + entry.first);
            auto functionValues = values;
            for (size_t i = 0; i < args.size(); ++i) {
                std::string arg = resolveListAccess(args[i], values);
                arg = resolveScalarCalls(arg, values, depth + 1);
                arg = substitute(arg, values);
                functionValues[entry.second.params[i]] = numberText(Expression(arg, values).parse());
            }
            std::string expression = resolveListAccess(entry.second.expression, functionValues);
            expression = substitute(expression, functionValues);
            expression = resolveScalarCalls(expression, functionValues, depth + 1);
            std::string result = numberText(Expression(expression, functionValues).parse());
            text.replace(pos, close - pos + 1, result);
            replaced = true;
            break;
        }
    }
    return text;
}

std::vector<std::string> Frontend::expand(const std::vector<std::string>& lines,
                                          const std::filesystem::path& baseDir,
                                          const std::map<std::string, std::string>& locals,
                                          int transpose) {
    std::vector<std::string> output;
    auto values = variables_;
    values.insert(locals.begin(), locals.end());
    auto resolve = [&](std::string text) {
        text = resolveListAccess(std::move(text), values);
        text = resolveScalarCalls(std::move(text), values);
        return substitute(std::move(text), values);
    };

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = stripComment(lines[i]);
        if (line.empty()) continue;

        if (toLower(line) == "break") throw BreakSignal{};
        if (toLower(line) == "continue") throw ContinueSignal{};

        std::smatch match;
        if (std::regex_match(line, match, std::regex(R"(^import\s+[\"]([^\"]+)[\"]\s*$)", std::regex::icase))) {
            std::filesystem::path requested(match[1].str());
            std::filesystem::path target;
            std::string generic = requested.generic_string();
            if (generic.rfind("std/", 0) == 0) {
                std::string libraryFile = generic.substr(4);
                if (const char* configured = std::getenv("LYRA_STDLIB"))
                    target = std::filesystem::path(configured) / libraryFile;
                if (target.empty() || !std::filesystem::exists(target)) {
                    std::filesystem::path cursor = baseDir;
                    while (!cursor.empty()) {
                        auto candidate = cursor / "stdlib" / libraryFile;
                        if (std::filesystem::exists(candidate)) { target = candidate; break; }
                        if (cursor == cursor.parent_path()) break;
                        cursor = cursor.parent_path();
                    }
                }
                if (target.empty() || !std::filesystem::exists(target))
                    target = std::filesystem::current_path() / "stdlib" / libraryFile;
            } else target = baseDir / requested;
            auto imported = processPath(target);
            output.insert(output.end(), imported.begin(), imported.end());
            values = variables_;
            values.insert(locals.begin(), locals.end());
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^(let|const)\s+([A-Za-z_]\w*)\s*=\s*\[(.*)\]\s*$)", std::regex::icase))) {
            std::string name = match[2].str();
            if (variables_.count(name) || lists_.count(name))
                throw std::runtime_error("Variable already defined: " + name);
            std::vector<std::string> items;
            for (const auto& item : splitComma(match[3].str()))
                items.push_back(resolve(item));
            lists_[name] = items;
            if (toLower(match[1].str()) == "const") constants_.insert(name);
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^set\s+([A-Za-z_]\w*)\[([^\]]+)\]\s*=\s*(.+)$)", std::regex::icase))) {
            std::string name = match[1].str();
            if (constants_.count(name)) throw std::runtime_error("Cannot change const list: " + name);
            auto list = lists_.find(name);
            if (list == lists_.end()) throw std::runtime_error("Unknown list: " + name);
            int index = static_cast<int>(std::lround(Expression(resolve(match[2].str()), values).parse()));
            if (index < 0) index += static_cast<int>(list->second.size());
            if (index < 0 || static_cast<size_t>(index) >= list->second.size()) throw std::runtime_error("List index out of range: " + name);
            list->second[static_cast<size_t>(index)] = resolve(match[3].str());
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^(push|remove|pop)\s+([A-Za-z_]\w*)(?:\s*,?\s*(.*))?$)", std::regex::icase))) {
            std::string operation = toLower(match[1].str());
            std::string name = match[2].str();
            if (constants_.count(name)) throw std::runtime_error("Cannot change const list: " + name);
            auto list = lists_.find(name);
            if (list == lists_.end()) throw std::runtime_error("Unknown list: " + name);
            if (operation == "push") {
                if (!match[3].matched || trim(match[3].str()).empty()) throw std::runtime_error("push requires a value");
                list->second.push_back(resolve(match[3].str()));
            } else if (operation == "pop") {
                if (list->second.empty()) throw std::runtime_error("Cannot pop an empty list: " + name);
                list->second.pop_back();
            } else {
                if (!match[3].matched) throw std::runtime_error("remove requires an index");
                int index = static_cast<int>(std::lround(Expression(resolve(match[3].str()), values).parse()));
                if (index < 0) index += static_cast<int>(list->second.size());
                if (index < 0 || static_cast<size_t>(index) >= list->second.size()) throw std::runtime_error("List index out of range: " + name);
                list->second.erase(list->second.begin() + index);
            }
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^(let|const|set)\s+([A-Za-z_]\w*)\s*=\s*(.+)$)", std::regex::icase))) {
            std::string kind = toLower(match[1].str());
            std::string name = match[2].str();
            std::string valueText = trim(match[3].str());
            if (kind == "set" && constants_.count(name)) throw std::runtime_error("Cannot change const: " + name);
            if (kind != "set" && variables_.count(name)) throw std::runtime_error("Variable already defined: " + name);
            std::string value;
            if (valueText.size() >= 2 && valueText.front() == '"' && valueText.back() == '"')
                value = valueText.substr(1, valueText.size() - 2);
            else value = numberText(Expression(resolve(valueText), values).parse());
            variables_[name] = value;
            if (kind == "const") constants_.insert(name);
            values[name] = value;
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^(instrument|effect)\s+([A-Za-z_]\w*)(?:\s+extends\s+([A-Za-z_]\w*))?\s*\{$)", std::regex::icase))) {
            auto block = collectBlock(lines, i); i = block.second;
            std::string kind = toLower(match[1].str());
            std::string name = match[2].str();
            std::string parent = match[3].matched ? match[3].str() : "";
            Preset preset;
            auto& table = kind == "instrument" ? instruments_ : effects_;
            if (!parent.empty()) {
                auto parentIt = table.find(parent);
                if (parentIt == table.end()) throw std::runtime_error("Unknown parent " + kind + ": " + parent);
                preset.body = parentIt->second.body;
            }
            preset.body.insert(preset.body.end(), block.first.begin(), block.first.end());
            table[name] = preset;
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^(pattern|function)\s+([A-Za-z_]\w*)\s*\(([^)]*)\)\s*\{$)", std::regex::icase))) {
            auto block = collectBlock(lines, i); i = block.second;
            Macro macro;
            for (const auto& param : splitComma(match[3].str())) if (!param.empty()) macro.params.push_back(param);
            macro.body = block.first;
            std::string kind = toLower(match[1].str());
            if (kind == "function") {
                std::vector<std::string> meaningful;
                for (const auto& bodyLine : block.first) {
                    std::string clean = stripComment(bodyLine);
                    if (!clean.empty()) meaningful.push_back(clean);
                }
                std::smatch returnMatch;
                if (meaningful.size() == 1 && std::regex_match(meaningful[0], returnMatch,
                    std::regex(R"(^return\s+(.+)$)", std::regex::icase))) {
                    scalarFunctions_[match[2].str()] = ScalarFunction{macro.params, returnMatch[1].str()};
                    continue;
                }
            }
            (kind == "pattern" ? patterns_ : functions_)[match[2].str()] = macro;
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^song\s+[A-Za-z_]\w*\s*\{$)", std::regex::icase))) {
            auto block = collectBlock(lines, i); i = block.second;
            auto expanded = expand(block.first, baseDir, locals, transpose);
            output.insert(output.end(), expanded.begin(), expanded.end());
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^if\s+(.+)\s*\{$)", std::regex::icase))) {
            auto trueBlock = collectBlock(lines, i);
            i = trueBlock.second;
            std::vector<std::string> falseBlock;
            size_t elseLine = i + 1;
            while (elseLine < lines.size() && stripComment(lines[elseLine]).empty()) ++elseLine;
            if (elseLine < lines.size() &&
                std::regex_match(stripComment(lines[elseLine]), std::regex(R"(^else\s*\{$)", std::regex::icase))) {
                auto collectedElse = collectBlock(lines, elseLine);
                falseBlock = collectedElse.first;
                i = collectedElse.second;
            }
            bool condition = Expression(resolve(match[1].str()), values).parse() != 0.0;
            const auto& selected = condition ? trueBlock.first : falseBlock;
            if (!selected.empty()) {
                auto expanded = expand(selected, baseDir, locals, transpose);
                output.insert(output.end(), expanded.begin(), expanded.end());
            }
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^for\s+([A-Za-z_]\w*)\s+in\s+(.+)\s*\{$)", std::regex::icase))) {
            auto block = collectBlock(lines, i); i = block.second;
            std::string iterator = match[1].str();
            std::string source = trim(match[2].str());
            std::vector<std::string> items;
            std::smatch rangeMatch;
            if (std::regex_match(source, rangeMatch, std::regex(R"(^range\s*\((.*)\)$)", std::regex::icase))) {
                auto args = splitComma(rangeMatch[1].str());
                if (args.size() < 2 || args.size() > 3)
                    throw std::runtime_error("range requires start, end, and optional step");
                double startValue = Expression(resolve(args[0]), values).parse();
                double endValue = Expression(resolve(args[1]), values).parse();
                double stepValue = args.size() == 3
                    ? Expression(resolve(args[2]), values).parse() : 1.0;
                if (stepValue == 0.0) throw std::runtime_error("range step cannot be zero");
                if (stepValue > 0.0) {
                    for (double value = startValue; value < endValue; value += stepValue)
                        items.push_back(numberText(value));
                } else {
                    for (double value = startValue; value > endValue; value += stepValue)
                        items.push_back(numberText(value));
                }
            } else {
                auto list = lists_.find(source);
                if (list == lists_.end()) throw std::runtime_error("Unknown list in for: " + source);
                items = list->second;
            }
            for (const auto& item : items) {
                auto loopLocals = locals;
                loopLocals[iterator] = item;
                try {
                    auto expanded = expand(block.first, baseDir, loopLocals, transpose);
                    output.insert(output.end(), expanded.begin(), expanded.end());
                } catch (const ContinueSignal&) {
                    continue;
                } catch (const BreakSignal&) {
                    break;
                }
            }
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^while\s+(.+)\s*\{$)", std::regex::icase))) {
            auto block = collectBlock(lines, i); i = block.second;
            std::string condition = match[1].str();
            size_t iterations = 0;
            while (true) {
                values = variables_;
                values.insert(locals.begin(), locals.end());
                if (Expression(resolve(condition), values).parse() == 0.0) break;
                if (++iterations > 10000) throw std::runtime_error("while loop exceeded 10000 iterations");
                try {
                    auto expanded = expand(block.first, baseDir, locals, transpose);
                    output.insert(output.end(), expanded.begin(), expanded.end());
                } catch (const ContinueSignal&) {
                    continue;
                } catch (const BreakSignal&) {
                    break;
                }
            }
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^transpose\s+(.+)\s*\{$)", std::regex::icase))) {
            auto block = collectBlock(lines, i); i = block.second;
            int amount = static_cast<int>(std::lround(Expression(resolve(match[1].str()), values).parse()));
            auto expanded = expand(block.first, baseDir, locals, transpose + amount);
            output.insert(output.end(), expanded.begin(), expanded.end());
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^repeat\s+(.+)\s*\{$)", std::regex::icase))) {
            auto block = collectBlock(lines, i); i = block.second;
            int count = static_cast<int>(std::lround(Expression(resolve(match[1].str()), values).parse()));
            if (count <= 0) throw std::runtime_error("repeat count must be > 0");
            for (int iteration = 0; iteration < count; ++iteration) {
                try {
                    auto expanded = expand(block.first, baseDir, locals, transpose);
                    output.insert(output.end(), expanded.begin(), expanded.end());
                } catch (const ContinueSignal&) {
                    continue;
                } catch (const BreakSignal&) {
                    break;
                }
            }
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^track\s+([A-Za-z_]\w*)(?:\s+uses\s+([A-Za-z_]\w*))?\s*\{$)", std::regex::icase))) {
            auto block = collectBlock(lines, i); i = block.second;
            output.push_back("track " + match[1].str() + " {");
            if (match[2].matched) {
                auto preset = instruments_.find(match[2].str());
                if (preset == instruments_.end()) throw std::runtime_error("Unknown instrument object: " + match[2].str());
                auto expandedPreset = expand(preset->second.body, baseDir, locals, transpose);
                output.insert(output.end(), expandedPreset.begin(), expandedPreset.end());
            }
            auto expanded = expand(block.first, baseDir, locals, transpose);
            output.insert(output.end(), expanded.begin(), expanded.end());
            output.push_back("}");
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^use\s+(instrument|effect)\s+([A-Za-z_]\w*)\s*$)", std::regex::icase))) {
            std::string kind = toLower(match[1].str());
            auto& table = kind == "instrument" ? instruments_ : effects_;
            auto preset = table.find(match[2].str());
            if (preset == table.end()) throw std::runtime_error("Unknown " + kind + " object: " + match[2].str());
            auto expanded = expand(preset->second.body, baseDir, locals, transpose);
            output.insert(output.end(), expanded.begin(), expanded.end());
            continue;
        }

        if (std::regex_match(line, match, std::regex(R"(^(play|call)\s+([A-Za-z_]\w*)\s*\((.*)\)\s*$)", std::regex::icase))) {
            std::string kind = toLower(match[1].str());
            auto& table = kind == "play" ? patterns_ : functions_;
            auto macroIt = table.find(match[2].str());
            if (macroIt == table.end()) throw std::runtime_error("Unknown " + kind + " target: " + match[2].str());
            auto args = splitComma(match[3].str());
            if (args.size() != macroIt->second.params.size()) throw std::runtime_error("Wrong argument count for: " + match[2].str());
            std::map<std::string, std::string> macroLocals = locals;
            for (size_t p = 0; p < args.size(); ++p)
                macroLocals[macroIt->second.params[p]] = resolve(args[p]);
            if (kind == "play") output.push_back("patternbegin " + match[2].str());
            auto expanded = expand(macroIt->second.body, baseDir, macroLocals, transpose);
            output.insert(output.end(), expanded.begin(), expanded.end());
            if (kind == "play") output.push_back("patternend");
            continue;
        }

        // Existing loop blocks may contain Lyra 2 constructs, so process their body too.
        if (std::regex_match(line, match, std::regex(R"(^loop\s+(.+)\s*\{$)", std::regex::icase))) {
            auto block = collectBlock(lines, i); i = block.second;
            output.push_back(applyTranspose(resolve(line), transpose));
            auto expanded = expand(block.first, baseDir, locals, transpose);
            output.insert(output.end(), expanded.begin(), expanded.end());
            output.push_back("}");
            continue;
        }

        if (std::regex_match(line, match,
            std::regex(R"(^(clip|sample)\s+\"([^\"]+)\"(.*)$)", std::regex::icase))) {
            std::filesystem::path asset(match[2].str());
            if (asset.is_relative()) asset = baseDir / asset;
            output.push_back(toLower(match[1].str()) + " \"" +
                std::filesystem::absolute(asset).lexically_normal().string() + "\"" + match[3].str());
            continue;
        }

        output.push_back(applyTranspose(resolve(line), transpose));
    }
    return output;
}

} // namespace lyra
