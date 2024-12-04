/**
 * @file symtable.c
 *
 * Implementation of the symbol table data structure and its operations.
 * The symbol table is implemented as a hash table with separate chaining.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */

#include "symtable.h"
#include "error.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_SYMTABLE_SIZE 64
#define LOAD_FACTOR 0.75

extern BuiltinFunctionInfo builtin_functions[];

static void symtable_grow(SymTable *symtable);

/**
 * Initializes the symbol table.
 */
void symtable_init(SymTable *symtable)
{
    symtable->size = INITIAL_SYMTABLE_SIZE;
    symtable->count = 0;
    symtable->table = (Symbol **)safe_malloc(sizeof(Symbol) * symtable->size);
    for (int i = 0; i < symtable->size; i++)
    {
        symtable->table[i] = NULL;
    }

    insert_underscore(symtable);
}

/**
 * Loads the built-in functions into the symbol table.
 */
void load_builtin_functions(SymTable *symtable, ASTNode *import_node)
{
    size_t num_functions = get_num_builtin_functions();
    import_node = import_node;
    for (size_t i = 0; i < num_functions; i++)
    {
        char *name_with_prefix = (char *)safe_malloc(strlen("ifj.") + strlen(builtin_functions[i].name) + 1);
        strcpy(name_with_prefix, "ifj.");
        strcat(name_with_prefix, builtin_functions[i].name);

        Symbol *new_function = (Symbol *)safe_malloc(sizeof(Symbol));
        new_function->name = name_with_prefix;
        new_function->symbol_type = SYMBOL_FUNCTION;
        new_function->data_type = builtin_functions[i].return_type;
        new_function->is_defined = true;
        new_function->is_used = false;
        new_function->is_constant = true;
        new_function->parent_function = NULL;
        new_function->declaration_node = NULL;
        new_function->next = NULL;

        symtable_insert(symtable, name_with_prefix, new_function);
    }
}

/**
 * Inserts an underscore symbol into the symbol table.
 */
void insert_underscore(SymTable *symtable)
{
    Symbol *underscore = (Symbol *)safe_malloc(sizeof(Symbol));
    underscore->name = "_";
    underscore->symbol_type = SYMBOL_VARIABLE;
    underscore->data_type = TYPE_ALL;
    underscore->is_defined = true;
    underscore->is_used = true;
    underscore->is_constant = false;
    underscore->next = NULL;

    symtable_insert(symtable, "_", underscore);
}

/**
 * Frees all memory allocated for the symbol table.
 */
void symtable_free(SymTable *symtable)
{
    for (int i = 0; i < symtable->size; i++)
    {
        Symbol *symbol = symtable->table[i];
        while (symbol != NULL)
        {
            Symbol *temp = symbol;
            symbol = symbol->next;
            safe_free(temp->name); // Free symbol name
            safe_free(temp);       // Free symbol structure
        }
    }
    safe_free(symtable->table);
    symtable->table = NULL;
    symtable->size = 0;
    symtable->count = 0;
}

/**
 * Hash function to calculate the index for a given key.
 */
unsigned int symtable_hash(char *key, int size)
{
    if (key == NULL)
    {
        error_exit(ERR_INTERNAL, "NULL key passed to symtable_hash");
    }
    unsigned long hash = 5381;
    int c;
    while ((c = *key++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % size;
}

/**
 * Inserts a symbol into the symbol table.
 */
Symbol *symtable_insert(SymTable *symtable, char *key, Symbol *symbol)
{
    if (key == NULL)
    {
        error_exit(ERR_INTERNAL, "NULL key passed to symtable_insert");
    }
    if (symbol == NULL)
    {
        error_exit(ERR_INTERNAL, "NULL symbol passed to symtable_insert");
    }
    if (symbol->name == NULL)
    {
        error_exit(ERR_INTERNAL, "Symbol with NULL name passed to symtable_insert");
    }
    if ((float)symtable->count / symtable->size >= LOAD_FACTOR)
    {
        symtable_grow(symtable);
    }

    unsigned int index = symtable_hash(key, symtable->size);
    Symbol *current = symtable->table[index];
    while (current != NULL)
    {
        if (current->name == NULL)
        {
            error_exit(ERR_INTERNAL, "NULL symbol name encountered in symtable during insertion");
        }
        if (strcmp(current->name, key) == 0)
        {
            return NULL;
        }
        current = current->next;
    }
    symbol->next = symtable->table[index];
    symtable->table[index] = symbol;
    symtable->count++;

    return symbol;
}

/**
 * Searches for a symbol in the table by key.
 */
Symbol *symtable_search(SymTable *symtable, char *key)
{
    if (key == NULL)
    {
        error_exit(ERR_INTERNAL, "NULL key passed to symtable_search");
    }

    unsigned int index = symtable_hash(key, symtable->size);
    Symbol *current = symtable->table[index];
    while (current != NULL)
    {
        if (current->name == NULL)
        {
            error_exit(ERR_INTERNAL, "NULL symbol name encountered in symtable");
        }
        if (strcmp(current->name, key) == 0)
        {
            current->is_used = true;
            return current;
        }

        current = current->next;
    }

    return NULL;
}

/**
 * Checks if all symbols in the table have is_used set to true.
 */
void is_symtable_all_used(SymTable *symtable)
{
    bool is_all_used = true;
    for (int i = 0; i < symtable->size; i++)
    {
        Symbol *current = symtable->table[i];
        while (current != NULL)
        {
            if (!current->is_used && strncmp(current->name, "ifj.", 4) != 0)
            {
                is_all_used = false;
                break;
            }
            current = current->next;
        }
    }
    if (!is_all_used)
        error_exit(ERR_SEMANTIC_UNUSED, "Unused variable");
    return;
}

/**
 * Checks if the main function is defined correctly.
 */
void is_main_correct(SymTable *symtable)
{
    Symbol *main = symtable_search(symtable, "main");
    if (main == NULL)
        error_exit(ERR_SEMANTIC_UNDEF, "Function \"main\" is not defined");
    if (main->declaration_node->parameters != NULL)
        error_exit(ERR_SEMANTIC_PARAMS, "Function \"main\" must have no parameters");
    if (main->data_type != TYPE_VOID)
        error_exit(ERR_SEMANTIC_PARAMS, "Function \"main\" must have no parameters");

    return;
}

/**
 * Removes a symbol from the symbol table.
 */
void symtable_remove(SymTable *symtable, char *key)
{
    unsigned int index = symtable_hash(key, symtable->size);
    Symbol *current = symtable->table[index];
    Symbol *prev = NULL;
    while (current != NULL)
    {
        if (strcmp(current->name, key) == 0)
        {
            if (prev == NULL)
            {
                symtable->table[index] = current->next;
            }
            else
            {
                prev->next = current->next;
            }
            safe_free(current->name);
            safe_free(current);
            symtable->count--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

/**
 * Grows the symbol table when load factor exceeds threshold.
 */
static void symtable_grow(SymTable *symtable)
{
    int old_size = symtable->size;
    symtable->size *= 2;
    Symbol **old_table = symtable->table;

    // Allocate new, larger table
    symtable->table = (Symbol **)safe_malloc(sizeof(Symbol *) * symtable->size);
    // Initialize the new table
    for (int i = 0; i < symtable->size; i++)
    {
        symtable->table[i] = NULL;
    }

    // Rehash all existing symbols into the new table
    for (int i = 0; i < old_size; i++)
    {
        Symbol *symbol = old_table[i];
        while (symbol != NULL)
        {
            Symbol *next = symbol->next;
            unsigned int new_index = symtable_hash(symbol->name, symtable->size);
            symbol->next = symtable->table[new_index];
            symtable->table[new_index] = symbol;
            symbol = next;
        }
    }

    // Free the old table
    safe_free(old_table);
}
