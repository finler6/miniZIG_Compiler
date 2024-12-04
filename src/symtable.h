/**
 * @file symtable.h
 *
 * Header file for the symbol table module.
 * The symbol table is implemented as a hash table.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>

struct ASTNode;
// Symbol types
typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER
} SymbolType;

// Data types
typedef enum {
    TYPE_NULL,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_ALL,
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_U8,
    TYPE_UNKNOWN,

    TYPE_INT_NULLABLE,
    TYPE_FLOAT_NULLABLE,
    TYPE_U8_NULLABLE

} DataType;

// Symbol structure
typedef struct Symbol {
    char *name;
    SymbolType symbol_type;
    DataType data_type;
    char *parent_function;
    bool is_defined;
    bool is_used;
    bool is_constant;
    struct ASTNode *declaration_node;
    struct Symbol *next; // Linked list for collision resolution
} Symbol;

// Symbol table structure
typedef struct {
    Symbol **table;
    int size;        // Current size of the hash table
    int count;       // Number of symbols in the table
} SymTable;

// Function prototypes
void symtable_init(SymTable *symtable);
void load_builtin_functions(SymTable *symtable, struct ASTNode *import_node);
void insert_underscore(SymTable *symtable);
void symtable_free(SymTable *symtable);
Symbol *symtable_insert(SymTable *symtable, char *key, Symbol *symbol);
Symbol *symtable_search(SymTable *symtable, char *key);
void symtable_remove(SymTable *symtable, char *key);
void is_symtable_all_used(SymTable *symtable);
void is_main_correct(SymTable *symtable);
// Hash function
unsigned int symtable_hash(char *key, int size);

#endif // SYMTABLE_H
