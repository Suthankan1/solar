# Makefile for OpenGL 3D Solar System setup on macOS

CXX = clang++
CXXFLAGS = -Wall -Wextra -std=c++17 -Wno-deprecated-declarations
LDFLAGS = -framework OpenGL -framework GLUT

TARGET = solar
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
