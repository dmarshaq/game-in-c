# Compiler and flags
CC = gcc
CFLAGS = -std=gnu11
DEV_CFLAGS = -g -O0 -DDEBUG
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

GAME_SRC := $(wildcard $(SRC_DIR)/game/*.c)
GAME_HEADERS := $(wildcard $(SRC_DIR)/game/*.h)
META_GAME_SRC := $(wildcard $(SRC_DIR)/game/*_meta_.c)
CORE_SRC :=	$(wildcard $(SRC_DIR)/core/*.c)
META_SRC := $(wildcard $(SRC_DIR)/meta/*.c)
META_GENERATED_SRC := $(SRC_DIR)/meta_generated.c

CORE_OBJ := $(patsubst $(SRC_DIR)/core/%.c, $(OBJ_DIR)/core/%.o, $(CORE_SRC))


INCLUDES := -I$(SRC_DIR)

# Meta preprocess target executable
TARGET_META_EXE = $(BIN_DIR)/meta.exe

# Target executable
TARGET_MAIN_EXE = $(BIN_DIR)/main.exe

# Target libcore.a
TARGET_CORE_STATIC = $(BIN_DIR)/libcore.a


# Default target logic
ifeq ($(BUILD),dev)
all: 
	$(MAKE) clean libcore meta
	$(MAKE) main clean_meta
else
all: 
	$(MAKE) clean libcore meta 
	$(MAKE) main clean_meta copy_resources
endif


# Rule to build obj/core/*.o from src/core/*.c
$(OBJ_DIR)/core/%.o: $(SRC_DIR)/core/%.c | $(OBJ_DIR)/core
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDES)

# Link to libcore.a
$(TARGET_CORE_STATIC): | $(BIN_DIR)
	ar rcs $@ $(CORE_OBJ) 


# Making meta.exe
$(TARGET_META_EXE): | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(INCLUDES) $(META_SRC) -L$(BIN_DIR) -lcore


# Link all into a single main.exe
$(TARGET_MAIN_EXE): | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(INCLUDES) $(META_GENERATED_SRC) $(META_GAME_SRC) -L$(BIN_DIR) -lcore $(LIBFLAGS)


libcore: $(CORE_OBJ) $(TARGET_CORE_STATIC)

meta: $(TARGET_META_EXE)
	$(TARGET_META_EXE) $(GAME_SRC) $(GAME_HEADERS)

main: $(TARGET_MAIN_EXE)

headers:
	@echo $(GAME_HEADERS)


copy_resources:
	@echo "Copying resources to $(BUILD_DIR)/res..."
	@mkdir -p $(BUILD_DIR)/res
	@cp -r res/* $(BUILD_DIR)/res/


restore:
	for f in $(META_BACKUP); do \
		cp $$f $${f%.bak}; \
	done
	sed -i 's/\r$$//' $$(echo $(META_BACKUP) | sed 's/\.bak//g')


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

clean_meta:
	rm -f $(SRC_DIR)/game/*_meta_.c
	rm -f $(SRC_DIR)/game/*_meta_.h




# Phony targets
.PHONY: all clean

