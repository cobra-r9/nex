# ==============================================================================
# 1. COMPILER & FLAGS CONFIGURATION
# ==============================================================================
SRC_DIR  := src
INC_DIR  := include
OBJ_DIR  := build
BIN_DIR  := $(OBJ_DIR)/bin

CC       := gcc
CFLAGS   := -std=c23 -pedantic -Wall -Wextra
CPPFLAGS := -I$(SRC_DIR) -I$(INC_DIR) -D_POSIX_C_SOURCE=200809L -MMD -MP
LDLIBS   := -lm -lxcb -lxcb-util -lxcb-keysyms -lxcb-icccm -lxcb-ewmh -lxcb-randr -lxcb-xinerama -lxcb-shape

# ==============================================================================
# 2. SOURCE LISTS
# ==============================================================================
ALL_SRC   := $(wildcard $(SRC_DIR)/*.c)

NEXL_SRC  := $(SRC_DIR)/nexl.c $(SRC_DIR)/helpers.c
NEX_SRC   := $(filter-out $(SRC_DIR)/nexl.c,$(ALL_SRC))

NEX_OBJ   := $(NEX_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
NEXL_OBJ  := $(NEXL_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

DEPS      := $(sort $(NEX_OBJ:.o=.d) $(NEXL_OBJ:.o=.d))

NEX_TARGET  := $(BIN_DIR)/nex
NEXL_TARGET := $(BIN_DIR)/nexl

# ==============================================================================
# 3. BUILD RULES
# ==============================================================================
.PHONY: all build run clean test reset purge

all: build

build: $(NEX_TARGET) $(NEXL_TARGET)

$(NEX_TARGET): $(NEX_OBJ)
	@mkdir -p $(BIN_DIR)
	@echo "Linking: $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(NEXL_TARGET): $(NEXL_OBJ)
	@mkdir -p $(BIN_DIR)
	@echo "Linking: $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling: $< -> $@"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

run: build
	@echo ""
	@echo "-------------------------[test]--------------------------------"
	@echo ""
	@$(NEX_TARGET)
	@$(NEXL_TARGET)
	@echo ""
	@echo "---------------------------------------------------------------"
	@echo ""

clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJ_DIR)

test: run clean

reset:
	@echo "Resetting the project..."
	@rm -rf $(SRC_DIR)/*
	@rm -rf $(INC_DIR)/*
	@rm -rf $(OBJ_DIR)

# For arch makepkg -si helper.
purge:
	@echo "Cleaning makepkg residues..."
	@rm -rf pkg
	@rm -f *.tar.zst

# ==============================================================================
# 4. HEADER DEPENDENCY INCLUSION
# ==============================================================================
-include $(DEPS)
