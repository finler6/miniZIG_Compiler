#!/bin/bash

# Проверка, что передан параметр
if [ -z "$1" ]; then
    echo "Usage: $0 <parameter>"
    exit 1
fi

# Параметры
PARAM="$1"
DIRECTORY="./tests"

# Убедиться, что директория существует
if [ ! -d "$DIRECTORY" ]; then
    echo "Directory $DIRECTORY does not exist."
    exit 1
fi

# Найти максимальное число в файлах формата test_1_*.ifj24
MAX_NUMBER=$(find "$DIRECTORY" -type f -name "test_*.ifj24" | sed -E 's/.*test_([0-9]+)_.*/\1/' | sort -n | tail -n 1)

# Если файлов нет, установить MAX_NUMBER в 0
if [ -z "$MAX_NUMBER" ]; then
    MAX_NUMBER=0
fi

# Инкрементировать максимальное число
NEW_NUMBER=$((MAX_NUMBER + 1))

# Сформировать имя нового файла
NEW_FILENAME="$DIRECTORY/test_${NEW_NUMBER}_${PARAM}.ifj24"

# Создать новый файл (или выполнить другое действие)
touch "$NEW_FILENAME"

# Вывести результат
echo "New file created: $NEW_FILENAME"

