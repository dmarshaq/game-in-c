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
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
SRC_DIR := src

MAIN_SRC := $(SRC_DIR)/main.c
GAME_SRC := $(wildcard $(SRC_DIR)/game/*.c)
CORE_SRC :=	$(wildcard $(SRC_DIR)/core/*.c)

INCLUDES := -I$(SRC_DIR)


# Target executable
TARGET_MAIN_EXE = $(BIN_DIR)/main.exe

# Target plug dll
TARGET_PLUG_DLL = $(BIN_DIR)/plug.dll

# Target core dll
TARGET_CORE_DLL = $(BIN_DIR)/core.dll

# Release executable
TARGET_RELEASE = $(BIN_DIR)/release.exe

# Default target logic
all: 
ifeq ($(BUILD),dev)
	$(MAKE) clean core plug main
else
	$(MAKE) clean release
endif

# Link into core dll
$(TARGET_CORE_DLL): | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ -fPIC -shared $(INCLUDES) $(CORE_SRC) $(LIBFLAGS)

# Link into plug dll
$(TARGET_PLUG_DLL): | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ -fPIC -shared $(INCLUDES) $(GAME_SRC) $(LIBFLAGS) -L$(BIN_DIR) -lcore

# Link into main exe
$(TARGET_MAIN_EXE): | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(INCLUDES) $(MAIN_SRC) $(LIBFLAGS) -L$(BIN_DIR) -lcore

# Link all into a single main exe
$(TARGET_RELEASE): | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(INCLUDES) $(MAIN_SRC) $(CORE_SRC) $(GAME_SRC) $(LIBFLAGS)


core: $(TARGET_CORE_DLL)

plug: $(TARGET_PLUG_DLL)

main: $(TARGET_MAIN_EXE)

release: clean $(TARGET_RELEASE)
	@echo "Copying resources to $(BUILD_DIR)/res..."
	@mkdir -p $(BUILD_DIR)/res
	@cp -r res/* $(BUILD_DIR)/res/



$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@


# Clean build artifacts
clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(BIN_DIR)/*.dll
	rm -f $(BIN_DIR)/*.exe




# Phony targets
.PHONY: all clean

