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

# Target plug dll
TARGET_PLUG_DLL = $(BIN_DIR)/plug.dll

# Target core dll
TARGET_CORE_DLL = $(BIN_DIR)/core.dll

# Automatic sourcing.
## SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
## OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Manual sourcing.
SRC_FILES = $(SRC_DIR)/main.c
OBJ_FILES = $(OBJ_DIR)/main.o

# Default target
all: clean core plug main
	./$(TARGET)

# # Link object files into the final executable
# $(TARGET): $(OBJ_FILES) | $(BIN_DIR)
# 	$(CC) $(CFLAGS) -o $@ $^ $(LIBFLAGS) -Lbin/ -lcore
# 
# # Compile each source file into an object file
# $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
# 	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if they don't exist
$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(BIN_DIR):
	mkdir $(BIN_DIR)

# Link into core dll
$(TARGET_CORE_DLL):
	$(CC) $(CFLAGS) -o ./$(TARGET_CORE_DLL) -fPIC -shared src/core.c $(LIBFLAGS)

# Link into plug dll
$(TARGET_PLUG_DLL):
	$(CC) $(CFLAGS) -o ./$(TARGET_PLUG_DLL) -fPIC -shared src/plug.c $(LIBFLAGS) -Lbin/ -lcore

# Link into main exe
$(TARGET):
	$(CC) $(CFLAGS) -o ./$(TARGET) src/main.c $(LIBFLAGS) -Lbin/ -lcore


core: $(TARGET_CORE_DLL)

plug: $(TARGET_PLUG_DLL)

main: $(TARGET)


# Clean build artifacts
clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(BIN_DIR)/*.dll
	rm -f $(BIN_DIR)/*.exe


# Phony targets
.PHONY: all clean

