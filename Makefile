CXX := xcrun clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -g -Isrc -Ivendor/metal-cpp $(shell sdl2-config --cflags)

LDLIBS := $(shell sdl2-config --libs) \
	-framework Foundation \
	-framework Metal \
	-framework QuartzCore

APP := build/MetalLab.app/Contents
TARGET := $(APP)/MacOS/MetalLab
SHADER := $(APP)/Resources/mesh.metallib

SOURCES := \
	src/main.cpp \
	src/app/App.cpp \
	src/renderer/Renderer.cpp \
	src/math/Transform.cpp \
	src/scene/Camera.cpp \
	src/scene/Scene.cpp \
	src/renderer/mesh/Mesh.cpp \
	src/assets/ObjLoader.cpp

HEADERS := $(shell find src -name '*.hpp')

.PHONY: build run clean

build: $(TARGET) $(SHADER)

$(TARGET): $(SOURCES) $(HEADERS) resources/Info.plist Makefile
	mkdir -p $(APP)/MacOS
	$(CXX) $(CXXFLAGS) $(SOURCES) $(LDLIBS) -o $@
	cp resources/Info.plist $(APP)/Info.plist

$(SHADER): shaders/mesh.metal Makefile
	mkdir -p $(APP)/Resources
	xcrun -sdk macosx metal -c $< -o build/mesh.air
	xcrun -sdk macosx metallib build/mesh.air -o $@

run: build
	./$(TARGET)

clean:
	rm -rf build