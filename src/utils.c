#include "utils.h"

// Function to duplicate a string
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

void remove_decimal(char *str)
{
    char *dot = strchr(str, '.');
    if (dot != NULL)
    {
        *dot = '\0';
    }
}

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
    char *new_str = (char *)malloc(new_len);
    if(new_str == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation failed in add_decimal");
    }

    snprintf(new_str, new_len, "%s.0", str);
    return new_str;
}

char *construct_variable_name(const char *str1, const char *str2)
{
    int len = strlen(str1) + strlen(str2) + (2 * sizeof(char));
    char *result = (char *)malloc(len);
    if (result == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation failed in construct_variable_name\n");
    }
    snprintf(result, len, "%s.%s", str1, str2);
    add_pointer_to_storage(result);

    return result;
}

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

void *safe_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation failed.\n");
    }
    add_pointer_to_storage(ptr); // Добавляем указатель в глобальное хранилище
    return ptr;
}


void *safe_realloc(void *ptr, size_t new_size)
{
    if (ptr == NULL)
    {
        // Если ptr равен NULL, просто выделяем новую память
        void *new_ptr = malloc(new_size);
        if (new_ptr != NULL)
        {
            add_pointer_to_storage(new_ptr); // Добавляем новый указатель в хранилище
        }
        return new_ptr;
    }

    // Реальная работа realloc
    void *new_ptr = realloc(ptr, new_size);
    if (new_ptr != NULL)
    {
        // Обновляем указатель в хранилище
        for (size_t i = 0; i < global_storage.count; i++)
        {
            if (global_storage.pointers[i] == ptr)
            {
                global_storage.pointers[i] = new_ptr; // Обновляем указатель в хранилище
                break;
            }
        }
    }
    else
    {
        fprintf(stderr, "Error: realloc failed.\n");
    }

    return new_ptr;
}

void safe_free(void *ptr)
{
    if (ptr == NULL)
    {
        return; // Нельзя освободить NULL
    }

    // Проверяем, был ли указатель добавлен в хранилище
    for (size_t i = 0; i < global_storage.count; i++)
    {
        if (global_storage.pointers[i] == ptr)
        {
            // Освобождаем память
            free(ptr);
            // Удаляем указатель из хранилища
            global_storage.pointers[i] = NULL;

            // Перемещаем остальные элементы на одну позицию влево (можно улучшить сдвиг с использованием метода удаления)
            for (size_t j = i; j < global_storage.count - 1; j++)
            {
                global_storage.pointers[j] = global_storage.pointers[j + 1];
            }

            global_storage.count--;
            return;
        }
    }

    fprintf(stderr, "Error: Pointer not found in storage.\n");
}

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