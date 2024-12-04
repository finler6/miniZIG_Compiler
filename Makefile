# Название конечного исполняемого файла
TARGET = ifj24_compiler

# Компилятор
CC = gcc

# Опции компилятора (включаем отладочные символы и стандарт C99)
CFLAGS = -std=c99 -Wall -Wextra -g 

# Папка с исходными файлами
SRC_DIR = src

# Папка для объектных файлов
OBJ_DIR = obj

# Список исходных файлов
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Список объектных файлов (каждое .c превращается в .o в папке obj)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Заголовочные файлы (зависимости)
DEPS = $(wildcard $(SRC_DIR)/*.h)

# Правило сборки всего проекта
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Правило для сборки объектных файлов
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Удаление скомпилированных файлов
clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)
	rm -rf $(OBJ_DIR)

# Файлы, которые нужно добавить в make: all, clean
.PHONY: all clean
