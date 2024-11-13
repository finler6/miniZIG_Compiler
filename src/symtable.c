/*
 * Symbol table implementation for IFJ24 compiler
 * This implementation uses a hash table with chaining (TRP-izp).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"
#include "error.h"
#include "parser.h"

#ifdef DEBUG_SYMTABLE
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

#define INITIAL_SYMTABLE_SIZE 64
#define LOAD_FACTOR 0.75

extern BuiltinFunctionInfo builtin_functions[];

static void symtable_grow(SymTable *symtable);

// Initializes the symbol table
void symtable_init(SymTable *symtable) {
    symtable->size = INITIAL_SYMTABLE_SIZE;
    symtable->count = 0;
    symtable->table = (Symbol **)malloc(sizeof(Symbol) * symtable->size);
    if (symtable->table == NULL) {
        error_exit(ERR_INTERNAL, "Memory allocation failed");
    }
    for (int i = 0; i < symtable->size; i++) {
        symtable->table[i] = NULL;
    }

    load_builtin_functions(symtable);
}

void load_builtin_functions(SymTable *symtable) {
    size_t num_functions = get_num_builtin_functions();

    for (size_t i = 0; i < num_functions; i++) {
        char *name_with_prefix = (char *)malloc(strlen("ifj.") + strlen(builtin_functions[i].name) + 1);
        if (name_with_prefix == NULL) {
            error_exit(ERR_INTERNAL, "Memory allocation failed for built-in function name.");
        }

        strcpy(name_with_prefix, "ifj.");
        strcat(name_with_prefix, builtin_functions[i].name);

        Symbol *new_function = (Symbol *)malloc(sizeof(Symbol));
        if (new_function == NULL) {
            error_exit(ERR_INTERNAL, "Memory allocation failed for built-in function symbol.");
        }

        new_function->name = name_with_prefix;
        new_function->symbol_type = SYMBOL_FUNCTION;
        new_function->data_type = builtin_functions[i].return_type;
        new_function->is_defined = true;
        new_function->is_used = false;
        new_function->is_constant = true;
        new_function->next = NULL;

        symtable_insert(symtable, name_with_prefix, new_function);
    }
}


// Frees all memory allocated for the symbol table
void symtable_free(SymTable *symtable) {
    for (int i = 0; i < symtable->size; i++) {
        Symbol *symbol = symtable->table[i];
        while (symbol != NULL) {
            Symbol *temp = symbol;
            symbol = symbol->next;
            free(temp->name);  // Free symbol name
            free(temp);        // Free symbol structure
        }
    }
    free(symtable->table);
    symtable->table = NULL;
    symtable->size = 0;
    symtable->count = 0;
}

// Hash function to calculate the index for a given key
unsigned int symtable_hash(char *key, int size) {
    if (key == NULL) {
        error_exit(ERR_INTERNAL, "NULL key passed to symtable_hash");
    }
    LOG("DEBUG_SYMTABLE: symtable_search called with key: %s\n", key);
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % size;
}
// Inserts a symbol into the symbol table
Symbol *symtable_insert(SymTable *symtable, char *key, Symbol *symbol) {
    // Проверка ключа и символа на NULL
    if (key == NULL) {
        error_exit(ERR_INTERNAL, "NULL key passed to symtable_insert");
    }
    LOG("DEBUG_SYMTABLE: symtable_search called with key: %s\n", key);
    if (symbol == NULL) {
        error_exit(ERR_INTERNAL, "NULL symbol passed to symtable_insert");
    }

    // Проверка имени символа на NULL
    if (symbol->name == NULL) {
        error_exit(ERR_INTERNAL, "Symbol with NULL name passed to symtable_insert");
    }

    // Если коэффициент загрузки превышает порог, расширяем таблицу
    if ((float)symtable->count / symtable->size >= LOAD_FACTOR) {
        symtable_grow(symtable);
    }

    unsigned int index = symtable_hash(key, symtable->size);
    Symbol *current = symtable->table[index];

    // Проверка на существование символа с таким же ключом
    while (current != NULL) {
        LOG("DEBUG_SYMTABLE: Comparing key: '%s' with existing symbol: '%s'\n", key, current->name);
        
        // Проверяем, что имя символа не NULL перед сравнением
        if (current->name == NULL) {
            error_exit(ERR_INTERNAL, "NULL symbol name encountered in symtable during insertion");
        }

        if (strcmp(current->name, key) == 0) {
            // Символ уже существует, возвращаем NULL для индикации неудачи
            return NULL;
        }
        current = current->next;
    }

    // Вставляем символ в начало списка
    symbol->next = symtable->table[index];
    symtable->table[index] = symbol;
    symtable->count++;

    LOG("DEBUG_SYMTABLE: Inserted symbol: '%s' at index: %u\n", symbol->name, index);

    return symbol;
}


// Searches for a symbol in the table by key
Symbol *symtable_search(SymTable *symtable, char *key) {
    // Проверяем, что ключ не равен NULL
    if (key == NULL) {
        error_exit(ERR_INTERNAL, "NULL key passed to symtable_search");
    }
    LOG("DEBUG_SYMTABLE: symtable_search called with key: %s\n", key);

    unsigned int index = symtable_hash(key, symtable->size);
    Symbol *current = symtable->table[index];

    // Цикл по связному списку символов в ячейке таблицы
    while (current != NULL) {
        // Отладочный вывод для проверки ключей и символов
        LOG("DEBUG_SYMTABLE: Comparing key: '%s' with symbol: '%s'\n", key, current->name);

        // Проверка, что имя символа не равно NULL перед сравнением
        if (current->name == NULL) {
            error_exit(ERR_INTERNAL, "NULL symbol name encountered in symtable");
        }

        // Сравнение ключа с именем символа
        if (strcmp(current->name, key) == 0) {
            current->is_used = true;
            return current;  // Символ найден
        }

        current = current->next;
    }

    return NULL;  // Символ не найден
}

// Checks if all symbols in the table have is_used set to true
bool is_symtable_all_used(SymTable *symtable) {
    for (int i = 0; i < symtable->size; i++) {
        Symbol *current = symtable->table[i];
        while (current != NULL) {
            // Проверка флага is_used для каждого символа, кроме встроенных функций
            if (!current->is_used && strncmp(current->name, "ifj.", 4) != 0) {
                return false; // Если хотя бы один символ не использован
            }
            current = current->next;
        }
    }
    return true; // Все символы использованы
}


// Removes a symbol from the symbol table
void symtable_remove(SymTable *symtable, char *key) {
    unsigned int index = symtable_hash(key, symtable->size);
    Symbol *current = symtable->table[index];
    Symbol *prev = NULL;
    while (current != NULL) {
        if (strcmp(current->name, key) == 0) {
            if (prev == NULL) {
                symtable->table[index] = current->next;
            } else {
                prev->next = current->next;
            }
            free(current->name);
            free(current);
            symtable->count--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Grows the symbol table when load factor exceeds threshold
static void symtable_grow(SymTable *symtable) {
    int old_size = symtable->size;
    symtable->size *= 2;
    Symbol **old_table = symtable->table;

    // Allocate new, larger table
    symtable->table = (Symbol **)malloc(sizeof(Symbol *) * symtable->size);
    if (symtable->table == NULL) {
        error_exit(ERR_INTERNAL, "Memory allocation failed during resizing");
    }

    // Initialize the new table
    for (int i = 0; i < symtable->size; i++) {
        symtable->table[i] = NULL;
    }

    // Rehash all existing symbols into the new table
    for (int i = 0; i < old_size; i++) {
        Symbol *symbol = old_table[i];
        while (symbol != NULL) {
            Symbol *next = symbol->next;
            unsigned int new_index = symtable_hash(symbol->name, symtable->size);
            symbol->next = symtable->table[new_index];
            symtable->table[new_index] = symbol;
            symbol = next;
        }
    }

    // Free the old table
    free(old_table);
}
