CC := gcc

TARGET_EXEC := server

BUILD_DIR := ./build

OBJ_DIR := ./obj
SRC_DIR := ./src

# Find all the C files
SRCS := $(shell find $(SRC_DIRS) -name '*.c')

# Prepends BUILD_DIR and appends .o to every src file
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

LIB_FLAGS := -ltoml

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(LIB_FLAGS) $(OBJS) -o $@


$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) -g -c $< -o $@

clean:
	rm -r $(BUILD_DIR)
