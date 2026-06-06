# Makefile for Modern OpenGL 3.3 Project on macOS
# Using GLFW, GLAD, and GLM

CXX = clang++
CC = clang
CXXFLAGS = -Wall -Wextra -std=c++17 -g -Wno-deprecated-declarations -I/opt/homebrew/include -Iglad/include
CFLAGS = -Wall -Wextra -g -Iglad/include
LDFLAGS = -L/opt/homebrew/lib -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

TARGET = solar
CPPSRC = main.cpp Window.cpp Renderer.cpp
CSRC = glad/src/glad.c

CPPOBJ = $(CPPSRC:.cpp=.o)
COBJ = $(CSRC:.c=.o)

all: $(TARGET)

$(TARGET): $(CPPOBJ) $(COBJ)
	$(CXX) $(CPPOBJ) $(COBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(CPPOBJ) $(COBJ)
	rm -rf $(TARGET).dSYM

.PHONY: all clean
