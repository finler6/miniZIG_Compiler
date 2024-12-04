/**
 * @file utils.c
 *
 * Utility functions for the IFJ project 2024.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "parser.h"

/**
 *  Function to safely duplicate a string
 */
char *string_duplicate(const char *src)
{
    if (src == NULL)
    {
        return NULL;
    }
    int len_str = strlen(src) + 1;
    char *copy = (char *)malloc(len_str);
    if (copy == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation failed in string_duplicate\n");
    }
    strcpy(copy, src);
    add_pointer_to_storage(copy);
    return copy;
}

/**
 * Function to add a decimal point
 */
char *add_decimal(const char *str)
{
    if (!str)
    {
        return NULL;
    }
    if (strchr(str, '.') != NULL)
    {
        return string_duplicate(str);
    }
    size_t new_len = strlen(str) + 3;
    char *new_str = (char *)safe_malloc(new_len);
    if(new_str == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation failed in add_decimal");
    }

    snprintf(new_str, new_len, "%s.0", str);
    return new_str;
}

/**
 * Adding scope_id and function_name as prefixes to the variable name
 */
char *construct_variable_name(const char *variable_name, const char *function_name)
{
    int scope_id = current_scope_id();
    size_t len = strlen(function_name) + strlen(variable_name) + 20;
    char *result = (char *)safe_malloc(len);
    if (result == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation failed in construct_variable_name\n");
    }
    snprintf(result, len, "%s.%d.%s", variable_name, scope_id, function_name);

    return result;
}

/**
 * Construct a name for a built-in function (ifj. functions)
 */
char *construct_builtin_name(const char *str1, const char *str2)
{
    int len = strlen(str1) + strlen(str2) + (2 * sizeof(char));
    char *result = (char *)malloc(len);
    if (result == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation failed in construct_builtin_name\n");
    }
    snprintf(result, len, "%s.%s", str1, str2);
    add_pointer_to_storage(result);

    return result;
}

PointerStorage global_storage;

/**
 * Initialize the global pointer storage with an initial capacity
 */
void init_pointers_storage(size_t initial_capacity)
{
    global_storage.pointers = (void **)malloc(initial_capacity * sizeof(void *));
    if (!global_storage.pointers)
    {
        error_exit(ERR_INTERNAL, "Failed to allocate memory for global storage.\n");
    }
    global_storage.count = 0;
    global_storage.capacity = initial_capacity;
}

/**
 * Add a pointer to the global pointer storage
 */
void add_pointer_to_storage(void *ptr)
{
    if (global_storage.count >= global_storage.capacity)
    {
        global_storage.capacity *= 2;
        void **new_pointers = (void **)realloc(global_storage.pointers, global_storage.capacity * sizeof(void *));
        if (!new_pointers)
        {
            error_exit(ERR_INTERNAL, "Failed to expand global pointer storage.\n");
        }
        global_storage.pointers = new_pointers;
    }
    global_storage.pointers[global_storage.count++] = ptr;
}

/**
 * Safe memory allocation with pointer storage management
 */
void *safe_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation failed.\n");
    }
    add_pointer_to_storage(ptr); // Add pointer to the global storage
    return ptr;
}

/**
 * Safe memory reallocation with pointer storage management
 */
void *safe_realloc(void *ptr, size_t new_size)
{
    if (ptr == NULL)
    {
        return safe_malloc(new_size);
    }

    void *new_ptr = realloc(ptr, new_size);
    if (new_ptr == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory reallocation failed.\n");
    }
    for (size_t i = 0; i < global_storage.count; i++)
    {
        if (global_storage.pointers[i] == ptr)
        {
            global_storage.pointers[i] = new_ptr;
            break;
        }
    }

    return new_ptr;
}

/**
 * Safely free memory and remove the pointer from storage
 */
void safe_free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    for (size_t i = 0; i < global_storage.count; i++)
    {
        if (global_storage.pointers[i] == ptr)
        {
            free(ptr);

            for (size_t j = i; j < global_storage.count - 1; j++)
            {
                global_storage.pointers[j] = global_storage.pointers[j + 1];
            }

            global_storage.count--;
            global_storage.pointers[global_storage.count] = NULL;

            return;
        }
    }

    fprintf(stderr, "Error: Pointer not found in storage.\n");
}

/**
 * Clean up the global pointer storage and free all stored pointers
 */
void cleanup_pointers_storage(void)
{
    for (size_t i = 0; i < global_storage.count; i++)
    {
        free(global_storage.pointers[i]);
    }
    free(global_storage.pointers);
    global_storage.pointers = NULL;
    global_storage.count = 0;
    global_storage.capacity = 0;
}
