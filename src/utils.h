#ifndef UTILS_H
#define UTILS_H

// Function to duplicate a string
char* string_duplicate(const char *str);
void remove_decimal(char* str);
void add_decimal(char* str);
char* construct_variable_name(const char* str1, const char* str2);
char* construct_builtin_name(const char* str1, const char* str2);

#endif // UTILS_H
