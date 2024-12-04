/**
 * @file ast.c
 *
 * Header file for abstract syntax tree (AST) representation.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */
#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include "symtable.h"

// Enumeration of different types of AST nodes
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_VARIABLE_DECLARATION,
    NODE_ASSIGNMENT,
    NODE_BINARY_OPERATION,
    NODE_LITERAL,
    NODE_IDENTIFIER,
    NODE_IF,
    NODE_WHILE,
    NODE_RETURN,
    NODE_FUNCTION_CALL,
    NODE_BLOCK
} NodeType;

// Definition of an AST node structure
typedef struct ASTNode {
    NodeType type;  // Type of the AST node
    DataType data_type;  // Data type associated with the node
    struct ASTNode* left;  // Pointer to the left child node
    struct ASTNode* right; // Pointer to the right child node
    struct ASTNode* next;  // Pointer to the next node (in a sequence)
    struct ASTNode* condition; // Pointer to a condition node (used in if/while)
    struct ASTNode* body;  // Pointer to a body node (e.g., block of statements)

    char* name;  // Name of the variable/function
    char* value; // Value of the literal
    struct ASTNode** parameters; // Pointer to an array of parameters (for functions)
    int param_count; // Number of parameters

    struct ASTNode** arguments; // Pointer to an array of arguments (for function calls)
    int arg_count;  // Number of arguments
} ASTNode;

// Functions to create different types of AST nodes
ASTNode* create_program_node();
ASTNode* create_function_node(char* name, DataType return_type, ASTNode** parameters, int param_count, ASTNode* body);
ASTNode* create_variable_declaration_node(char* name, DataType data_type, ASTNode* initializer);
ASTNode* create_assignment_node(char* name, ASTNode* value);
ASTNode* create_binary_operation_node(const char* operator_name, ASTNode* left, ASTNode* right);
ASTNode* create_literal_node(DataType type, char* value);
ASTNode* create_identifier_node(char* name);
ASTNode* create_if_node(ASTNode* condition, ASTNode* true_block, ASTNode* false_block, ASTNode *var_without_null);
ASTNode* create_while_node(ASTNode* condition, ASTNode* body);
ASTNode* create_return_node(ASTNode* value);
ASTNode* create_function_call_node(char* name, ASTNode** arguments, int arg_count);
ASTNode* create_block_node(ASTNode* statements, DataType return_type);

#endif // AST_H