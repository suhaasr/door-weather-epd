# Host build: unit tests and PNG previews of every screen.
#   make test     run unit tests
#   make preview  render docs/img/*.png
#   make server   try the reminders page at http://localhost:8080
CXX ?= g++
CC ?= gcc
CORE := ../lib/doorcore/src
DEPS := .deps
BUILD := build

CPPFLAGS := -DARDUINO=100 -I shim -I $(CORE) -I $(DEPS)/gfx -I $(DEPS)/ArduinoJson/src \
  -DARDUINOJSON_ENABLE_ARDUINO_STRING=0 -DARDUINOJSON_ENABLE_ARDUINO_STREAM=0 \
  -DARDUINOJSON_ENABLE_ARDUINO_PRINT=0 -DARDUINOJSON_ENABLE_PROGMEM=0
CXXFLAGS := -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter -Wno-format-truncation
CORE_SRCS := $(wildcard $(CORE)/*.cpp)
GFX_SRCS := $(DEPS)/gfx/Adafruit_GFX.cpp
CORE_OBJS := $(patsubst $(CORE)/%.cpp,$(BUILD)/core/%.o,$(CORE_SRCS))
GFX_OBJS := $(BUILD)/gfx/Adafruit_GFX.o

.PHONY: all test preview server clean deps
all: test preview

deps:
	./fetch_deps.sh

$(DEPS)/gfx/Adafruit_GFX.cpp:
	./fetch_deps.sh

$(BUILD)/core/%.o: $(CORE)/%.cpp $(wildcard $(CORE)/*.h) | $(DEPS)/gfx/Adafruit_GFX.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/gfx/Adafruit_GFX.o: $(DEPS)/gfx/Adafruit_GFX.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -w -c $< -o $@

$(BUILD)/%.o: %.cpp $(wildcard *.h) $(wildcard $(CORE)/*.h) | $(DEPS)/gfx/Adafruit_GFX.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/tests: $(BUILD)/tests.o $(CORE_OBJS) $(GFX_OBJS)
	$(CXX) $^ -o $@

$(BUILD)/preview: $(BUILD)/preview.o $(CORE_OBJS) $(GFX_OBJS)
	$(CXX) $^ -o $@

$(BUILD)/server: $(BUILD)/server.o $(CORE_OBJS) $(GFX_OBJS)
	$(CXX) $^ -o $@

server: $(BUILD)/server
	$(BUILD)/server

test: $(BUILD)/tests
	$(BUILD)/tests

preview: $(BUILD)/preview
	$(BUILD)/preview ../docs/img

clean:
	rm -rf $(BUILD)
