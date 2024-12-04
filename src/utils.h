/**
 * @file utils.h
 *
 * Header file for utility functions.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */
#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

// Function to duplicate a string
char* string_duplicate(const char *str);
void remove_decimal(char* str);
char *add_decimal(const char *str);
char* construct_variable_name(const char* str1, const char* str2);
char* construct_builtin_name(const char* str1, const char* str2);

typedef struct {
    void** pointers;
    size_t count;
    size_t capacity;
} PointerStorage;

extern PointerStorage global_storage;

void init_pointers_storage(size_t initial_capacity);
void add_pointer_to_storage(void* ptr);
void *safe_malloc(size_t size);
void *safe_realloc(void *ptr, size_t new_size);
void safe_free(void *ptr);
void cleanup_pointers_storage(void);
#endif // UTILS_H
