ifeq ($(shell which sdl2-config 2>/dev/null),)
$(error SDL2 not found. Run 'make install-deps' first)
endif

# Compiler & flags
CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 \
           $(shell sdl2-config --cflags)
LIBS    := $(shell sdl2-config --libs) \
           -lSDL2_image

#install dependencies
.PHONY: install-deps
install-deps:
        @if [ "$$(uname)" = "Darwin" ]; then \
                echo "Installing SDL2 via Homebrew..."; \
                brew install sdl2 sdl2_image; \
        elif [ "$$(uname)" = "Linux" ]; then \
                echo "Installing SDL2 via apt..."; \
                sudo apt-get install -y libsdl2-dev libsdl2-image-dev; \
        else \
                echo "Windows: download SDL2 from https://libsdl.org"; \
        fi

# Create output directories
.PHONY: dirs
dirs:
        mkdir -p $(OBJ_DIR) $(BIN_DIR)



# Directories
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

# Target executable
TARGET  := $(BIN_DIR)/game

# Sources & objects
SRCS    := $(wildcard *.c) $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(SRCS)))

# Default target
.PHONY: all
all: dirs $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Compile .c → .o (root-level files)
$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile .c → .o (src/ files)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create output directories
.PHONY: dirs
dirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Debug build (adds sanitizers + debug symbols)
.PHONY: debug
debug: CFLAGS += -g -O0 -fsanitize=address,undefined
debug: LIBS   += -fsanitize=address,undefined
debug: all

# Release build (optimized)
.PHONY: release
release: CFLAGS += -O2 -DNDEBUG
release: all

# Run the game
.PHONY: run
run: all
	./$(TARGET)

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Show detected sources
.PHONY: info
info:
	@echo "Sources: $(SRCS)"
	@echo "Objects: $(OBJS)"
	@echo "Target:  $(TARGET)"
