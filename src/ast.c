/**
 * @file ast.c
 *
 * Implementation of the Abstract Syntax Tree (AST) data structure.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */
#include "ast.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Create a program node representing the root of the AST.
 */
ASTNode *create_program_node()
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_PROGRAM;
    node->left = node->right = node->next = node->condition = NULL;
    node->body = NULL;
    node->name = NULL;
    node->value = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}

/**
 * Create a function node with a name, return type, parameters, and body.
 */
ASTNode *create_function_node(char *name, DataType return_type, ASTNode **parameters, int param_count, ASTNode *body)
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_FUNCTION;
    node->data_type = return_type;
    node->name = string_duplicate(name);
    node->parameters = parameters;
    node->param_count = param_count;
    node->body = body;
    node->left = node->right = node->next = node->condition = NULL;
    node->value = NULL;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}

/**
 * Create a variable declaration node with a name, type, and initializer.
 */
ASTNode *create_variable_declaration_node(char *name, DataType data_type, ASTNode *initializer)
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_VARIABLE_DECLARATION;
    node->data_type = data_type;
    node->name = string_duplicate(name);
    node->left = initializer;
    node->right = node->next = node->condition = node->body = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}

/**
 * Create an assignment node for assigning a value to a variable.
 */
ASTNode *create_assignment_node(char *name, ASTNode *value)
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_ASSIGNMENT;
    node->name = string_duplicate(name);
    node->left = value;
    node->right = node->next = node->condition = node->body = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}

/**
 * Create a binary operation node with an operator and operands.
 */
ASTNode *create_binary_operation_node(const char *operator_name, ASTNode *left, ASTNode *right)
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_BINARY_OPERATION;
    node->data_type = left->data_type;
    node->left = left;
    node->right = right;
    node->next = node->condition = node->body = NULL;
    node->name = string_duplicate(operator_name);
    node->value = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}

/**
 * Create a literal node representing a constant value.
 */
ASTNode *create_literal_node(DataType type, char *value)
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_LITERAL;
    node->data_type = type;
    node->value = string_duplicate(value);
    node->left = node->right = node->next = node->condition = node->body = NULL;
    node->name = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}

/**
 * Create an identifier node with a variable name.
 */
ASTNode *create_identifier_node(char *name)
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_IDENTIFIER;
    node->data_type = TYPE_UNKNOWN;
    node->name = string_duplicate(name);
    node->left = node->right = node->next = node->condition = node->body = NULL;
    node->value = NULL;
    node->data_type = TYPE_UNKNOWN;
    node->parameters = NULL;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}

/**
 * Create an if statement node with condition and branches.
 */
ASTNode *create_if_node(ASTNode *condition, ASTNode *true_block, ASTNode *false_block, ASTNode *var_without_null)
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_IF;
    node->data_type = true_block->data_type;
    node->condition = condition;
    node->body = true_block;
    node->left = false_block;
    node->right = node->next = NULL;
    node->name = NULL;
    node->value = NULL;
    node->parameters = (ASTNode **)safe_malloc(sizeof(ASTNode *));
    node->parameters[0] = var_without_null;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}

/**
 * Create a while loop node with a condition and body.
 */
ASTNode *create_while_node(ASTNode *condition, ASTNode *body)
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_WHILE;
    node->condition = condition;
    node->body = body;
    node->left = node->right = node->next = NULL;
    node->name = NULL;
    node->value = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}

/**
 * Create a return statement node with an optional value.
 */
ASTNode *create_return_node(ASTNode *value)
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_RETURN;
    node->left = value;
    node->right = node->next = node->condition = node->body = NULL;
    node->name = NULL;
    node->value = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    if (value != NULL)
    {
        node->data_type = value->data_type;
    }
    else
    {
        node->data_type = TYPE_VOID;
    }
    return node;
}

/**
 * Create a function call node with a name and arguments.
 */
ASTNode *create_function_call_node(char *name, ASTNode **arguments, int arg_count)
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_FUNCTION_CALL;
    node->name = string_duplicate(name);
    node->arguments = arguments;
    node->arg_count = arg_count;
    node->left = node->right = node->next = node->condition = node->body = NULL;
    node->value = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    return node;
}

/**
 * Create a block node containing a list of statements.
 */
ASTNode *create_block_node(ASTNode *statements, DataType return_type)
{
    ASTNode *node = (ASTNode *)safe_malloc(sizeof(ASTNode));
    node->type = NODE_BLOCK;
    node->data_type = return_type;
    node->body = statements;
    node->left = node->right = node->next = node->condition = NULL;
    node->name = NULL;
    node->value = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}
