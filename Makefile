# Detect the os: windows or linux, other os is not supported
DETECTED_OS := 
ifdef OS
	DETECTED_OS = Windows
else
	DETECTED_OS = $(shell uname)
endif

TARGET      := bin/main
SRC_DIR     := src
BUILD_DIR   := build

SRC_FILES   := 
ifeq ($(DETECTED_OS), Windows)
	SRC_FILES = $(foreach dir,src,$(wildcard $(dir)/*.cpp) $(wildcard $(dir)/*/*.cpp))
else
	SRC_FILES = $(shell find $(SRC_DIR) -name "*.cpp")
endif
OBJ_FILES   := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_FILES))

# Compiler and Flags
CC          := g++
CFLAGS      := -Wall -Wextra -I$(SRC_DIR)
DEBUG_FLAGS := -g

# Define phony targets
.PHONY: all debug run clean clean-bin clean-all

# Build the program
all: $(TARGET)

# link the final executable
$(TARGET): $(OBJ_FILES)
ifeq ($(DETECTED_OS), Windows)
	@if not exist "$(dir $@)" md "$(dir $@)"
	$(CC) $(OBJ_FILES) -o $@.exe
else
	@mkdir -p $(dir $@)
	$(CC) $(OBJ_FILES) -o $@
endif

# compile each .c into .o under build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
ifeq ($(DETECTED_OS), Windows)
	@if not exist "$(dir $@)" md "$(dir $@)"
else
	@mkdir -p $(dir $@)
endif
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

# Debug build
debug: CFLAGS += -DDEBUG
debug: all

# Run the program
run: all
	@./$(TARGET)

# Clean build files
clean:
	@echo "Cleaning build files..."
ifeq ($(DETECTED_OS), Windows)
	@if exist "$(BUILD_DIR)" @rmdir /Q /S "$(BUILD_DIR)"
	@if exist "$(subst /,\,$(TARGET)).exe" @del "$(subst /,\,$(TARGET)).exe"
else
	rm -rf $(BUILD_DIR) $(TARGET)
endif

# Clean binary file
clean-bin:
	@echo "Removing bin directory..."
ifeq ($(DETECTED_OS), Windows)
	@rmdir /Q /S "bin\"
else
	rm -rf bin
endif

clean-all: clean clean-bin

print:
	@echo $(DETECTED_OS)
	@echo $(SRC_FILES)
	@echo $(OBJ_FILES)