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


typedef struct ASTNode {
    NodeType type;  
    DataType data_type;  
    struct ASTNode* left;  
    struct ASTNode* right; 
    struct ASTNode* next;  
    struct ASTNode* condition; 
    struct ASTNode* body;  

    char* name;  
    char* value;
    struct ASTNode** parameters; 
    int param_count; 

    struct ASTNode** arguments; 
    int arg_count;  
} ASTNode;

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
ASTNode *create_conversion_node(DataType target_type, ASTNode *operand);

void free_ast_node(ASTNode* node);

void print_ast(ASTNode* node, int indent);

#endif // AST_H
