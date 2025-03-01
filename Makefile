CXX = clang++
CFLAGS = -Wall -Wextra -O2
CXXFLAGS = $(CFLAGS) -std=c++20
TARGET = zzz

# Raylib paths
RAYLIB_PATH = vendor/raylib
RAYLIB_INCLUDE = -I$(RAYLIB_PATH)/src
RAYGUI_PATH = vendor/raygui
RAYGUI_INCLUDE = -I$(RAYGUI_PATH)/src
RAYLIB_LIB = -L$(RAYLIB_PATH)/src -lraylib

UNAME = $(shell uname)
# mac only :)
PLATFORM_LIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo

SRCS = $(wildcard *.cxx)
OBJS = $(SRCS:.cxx=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(RAYLIB_LIB) $(PLATFORM_LIBS)

%.o: %.cxx
	$(CXX) -c $< -o $@ $(CXXFLAGS) $(RAYLIB_INCLUDE) $(RAYGUI_INCLUDE)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
