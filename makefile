# CC = gcc
# CFLAGS = -g
# LIBFLAGS = -lmingw32 -lSDL2main -lSDL2 -lopengl32 -lglew32
# 
# TARGET = main3.exe
# SRC = src\test.c
# 
# all: $(TARGET)
# 
# $(TARGET): $(SRC)
# 	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBFLAGS)


# Compiler and flags
CC = gcc
CFLAGS = -g -Wall
LIBFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -lopengl32 -lglew32

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Target executable
TARGET = $(BIN_DIR)/main.exe

# Source and object files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Default target
all: run

clean-run: clean run

# Link object files into the final executable
$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBFLAGS)

# Compile each source file into an object file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if they don't exist
$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(BIN_DIR):
	mkdir $(BIN_DIR)

# Run rule
run: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(BIN_DIR)/*.exe


# Phony targets
.PHONY: all clean

