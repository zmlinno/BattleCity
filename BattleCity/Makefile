# Simple Makefile for UDP server (standalone Asio)

TARGET := udp_server

# Search sources in these directories
SRCDIRS := . src network common game
SRC := $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.cpp))

INC := $(addprefix -I,$(SRCDIRS))

# Optionally set ASIO_INCLUDE_DIR via environment, e.g.
#   make ASIO_INCLUDE_DIR=$(brew --prefix)/include
# Auto-detect common locations if not provided
ifeq ($(ASIO_INCLUDE_DIR),)
  ifneq (,$(wildcard /opt/homebrew/include/asio.hpp))
    ASIO_INCLUDE_DIR := /opt/homebrew/include
  else ifneq (,$(wildcard /usr/local/include/asio.hpp))
    ASIO_INCLUDE_DIR := /usr/local/include
  else ifneq (,$(wildcard /usr/include/asio.hpp))
    ASIO_INCLUDE_DIR := /usr/include
  else ifneq (,$(wildcard include/asio.hpp))
    ASIO_INCLUDE_DIR := include
  else ifneq (,$(wildcard third_party/asio/include/asio.hpp))
    ASIO_INCLUDE_DIR := third_party/asio/include
  endif
endif

ifeq ($(ASIO_INCLUDE_DIR),)
  $(info Hint: If you see 'asio.hpp file not found', install Asio or run 'make ASIO_INCLUDE_DIR=/path/to/include')
else
  INC += -I$(ASIO_INCLUDE_DIR)
endif

CXX := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -Wpedantic -pthread -DASIO_STANDALONE
LDFLAGS := -pthread

OBJ := $(patsubst %.cpp,%.o,$(SRC))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

run: $(TARGET)
	./$(TARGET) 7777

clean:
	rm -f $(OBJ) $(TARGET)

