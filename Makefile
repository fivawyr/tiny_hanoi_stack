CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra

# Use pkg-config to get SDL2 flags. Fallback flags for macOS if pkg-config isn't configured.
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null || echo "-I/usr/local/include/SDL2 -D_THREAD_SAFE")
SDL_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null || echo "-L/usr/local/lib -lSDL2")

TARGET := tiny_hanoi
SRCS := code.cpp
OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(SDL_LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
