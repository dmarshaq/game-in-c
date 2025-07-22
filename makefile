# Compiler and flags
CC = gcc
CFLAGS = -std=gnu11
DEV_CFLAGS = -g -O0 -DDEBUG -DDEV
RELEASE_CFLAGS = -O2 -DNDEBUG
LIBFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -lopengl32 -lglew32

# Build mode: dev or release
BUILD ?= dev
	
ifeq ($(BUILD),dev)
    CFLAGS += $(DEV_CFLAGS)
	BUILD_DIR := .
else
    CFLAGS += $(RELEASE_CFLAGS)
	BUILD_DIR := build/$(BUILD)
endif

# Directories
OBJ_DIR := obj
BIN_DIR := $(BUILD_DIR)/bin
SRC_DIR := src

MAIN_SRC := $(SRC_DIR)/main.c
GAME_SRC := $(wildcard $(SRC_DIR)/game/*.c)
CORE_SRC :=	$(wildcard $(SRC_DIR)/core/*.c)
META_SRC := $(wildcard $(SRC_DIR)/meta/*.c)

CORE_OBJ := $(patsubst $(SRC_DIR)/core/%.c, $(OBJ_DIR)/core/%.o, $(CORE_SRC))

INCLUDES := -I$(SRC_DIR)

# Meta preprocess target executable
TARGET_META_EXE = $(BIN_DIR)/meta.exe

# Target executable
TARGET_MAIN_EXE = $(BIN_DIR)/main.exe

# Target plug.dll
TARGET_PLUG_DLL = $(BIN_DIR)/plug.dll

# Target core.dll
TARGET_CORE_DLL = $(BIN_DIR)/dyncore.dll

# Target libcore.a
TARGET_CORE_STATIC = $(BIN_DIR)/libcore.a

# Release executable
TARGET_RELEASE = $(BIN_DIR)/release.exe

# Default target logic
all: 
ifeq ($(BUILD),dev)
	$(MAKE) clean libcore dyncore plug main
else
	$(MAKE) clean libcore release
endif

# Make .o files for libcore.a

# Rule to build obj/core/*.o from src/core/*.c
$(OBJ_DIR)/core/%.o: $(SRC_DIR)/core/%.c | $(OBJ_DIR)/core
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDES)

# Link to libcore.a
$(TARGET_CORE_STATIC): | $(BIN_DIR)
	ar rcs $@ $(CORE_OBJ) 

# Link into core.dll
$(TARGET_CORE_DLL): | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ -fPIC -shared $(INCLUDES) $(CORE_SRC) $(LIBFLAGS)

# Link into plug.dll
$(TARGET_PLUG_DLL): | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ -fPIC -shared $(INCLUDES) $(GAME_SRC) $(LIBFLAGS) -L$(BIN_DIR) -ldyncore 

# Link into main.exe
$(TARGET_MAIN_EXE): | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(INCLUDES) $(MAIN_SRC) $(LIBFLAGS) -L$(BIN_DIR) -ldyncore 

# Link all into a single release.exe
$(TARGET_RELEASE): | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(INCLUDES) $(MAIN_SRC) $(GAME_SRC) -L$(BIN_DIR) -lcore $(LIBFLAGS)


libcore: $(CORE_OBJ) $(TARGET_CORE_STATIC)

dyncore: $(TARGET_CORE_DLL)

plug: $(TARGET_PLUG_DLL)

main: $(TARGET_MAIN_EXE)

release: clean $(TARGET_RELEASE)
	@echo "Copying resources to $(BUILD_DIR)/res..."
	@mkdir -p $(BUILD_DIR)/res
	@cp -r res/* $(BUILD_DIR)/res/

# core_obj: $(CORE_OBJ)

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@


# Clean build artifacts
clean:
	rm -f $(OBJ_DIR)/core/*.o
	rm -f $(OBJ_DIR)/*.o
	rm -f $(BIN_DIR)/*.a
	rm -f $(BIN_DIR)/*.dll
	rm -f $(BIN_DIR)/*.exe




# Phony targets
.PHONY: all clean

