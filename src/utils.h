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
char *add_decimal(const char *str);
// Function to construct a variable name from two strings
char* construct_variable_name(const char* str1, const char* str2);
// Function to construct a builtin function name from two strings
char* construct_builtin_name(const char* str1, const char* str2);

/*
 * Structure to store pointers for safe memory management.
 * This structure is used to keep track of all pointers allocated during the program execution.
 */
typedef struct {
    void** pointers;
    size_t count;
    size_t capacity;
} PointerStorage;

extern PointerStorage global_storage;

// Function to initialize the pointer storage
void init_pointers_storage(size_t initial_capacity);
// Function to add a pointer to the storage
void add_pointer_to_storage(void* ptr);
// Function to safely allocate memory
void *safe_malloc(size_t size);
// Function to safely reallocate memory
void *safe_realloc(void *ptr, size_t new_size);
// Function to safely free memory
void safe_free(void *ptr);
// Function to clean up the pointer storage to prevent memory leaks
void cleanup_pointers_storage(void);

#endif // UTILS_H
