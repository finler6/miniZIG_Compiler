TARGET = ifj24_compiler

CC = gcc

CFLAGS = -std=c99 -Wall -Wextra -g -pedantic -Werror

SRC_DIR = src

OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.c)

OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

DEPS = $(wildcard $(SRC_DIR)/*.h)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)
	rm -rf $(OBJ_DIR)
	rm -f valgrind_log.txt

.PHONY: all clean
