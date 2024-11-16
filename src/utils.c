#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"

// Function to duplicate a string
char* string_duplicate(const char* src) {
    if (src == NULL) {
        return NULL;
    }
    int len_str = strlen(src) + 1;
    char* copy = (char *)malloc(len_str); // +1 для нуль-терминатора
    if (copy != NULL) {
        strcpy(copy, src);
    }
    return copy;
}


void remove_decimal(char* str) {
    // Ищем позицию точки
    char* dot = strchr(str, '.');
    if (dot != NULL) {
        // Обнуляем символ после точки
        *dot = '\0';
    }
}

void add_decimal(char* str) {
    // Ищем позицию точки
    if (strchr(str, '.') == NULL) {
        // Если точки нет, добавляем ".0" в конец строки
        strcat(str, ".0");
    }
}

char* construct_variable_name(const char* str1, const char* str2) {
    // Вычисление необходимой длины результирующей строки
    int len = strlen(str1) + strlen(str2) + (2*sizeof(char)); // Запас для точки, подчеркивания и числа
    char* result = (char*)malloc(len);

    if (result == NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }

    // Форматируем строку по шаблону "str1.str2_number"
    snprintf(result, len, "%s_%s", str1, str2);

    return result;
}

char* construct_builtin_name(const char* str1, const char* str2) {
    // Вычисление необходимой длины результирующей строки
    int len = strlen(str1) + strlen(str2) + (2*sizeof(char)); // Запас для точки, подчеркивания и числа
    char* result = (char*)malloc(len);

    if (result == NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }

    // Форматируем строку по шаблону "str1.str2_number"
    snprintf(result, len, "%s.%s", str1, str2);

    return result;
}