CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Isrc
TARGET   = lyra

SRCS = src/main.cpp src/parser.cpp src/project.cpp src/dsp.cpp src/audio_file.cpp src/synth.cpp src/export.cpp src/frontend.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS) *.wav *.mid

.PHONY: all clean
