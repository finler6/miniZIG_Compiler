#include "ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"


ASTNode* create_program_node() {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
    add_pointer_to_storage(node);

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


ASTNode* create_function_node(char* name, DataType return_type, ASTNode** parameters, int param_count, ASTNode* body) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
    add_pointer_to_storage(node);

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


ASTNode* create_variable_declaration_node(char* name, DataType data_type, ASTNode* initializer) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
    add_pointer_to_storage(node);

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


ASTNode* create_assignment_node(char* name, ASTNode* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
    add_pointer_to_storage(node);

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

ASTNode* create_binary_operation_node(const char* operator_name, ASTNode* left, ASTNode* right) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
    add_pointer_to_storage(node);

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


ASTNode* create_literal_node(DataType type, char* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
    add_pointer_to_storage(node);

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


ASTNode* create_identifier_node(char* name) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
        add_pointer_to_storage(node);

    node->type = NODE_IDENTIFIER;
    node->name = string_duplicate(name);
    node->left = node->right = node->next = node->condition = node->body = NULL;
    node->value = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}

ASTNode* create_if_node(ASTNode* condition, ASTNode* true_block, ASTNode* false_block, ASTNode *var_without_null) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
        add_pointer_to_storage(node);

    node->type = NODE_IF;
    node->data_type = true_block->data_type;
    node->condition = condition;
    node->body = true_block;
    node->left = false_block;  
    node->right = node->next = NULL;
    node->name = NULL;
    node->value = NULL;
    node->parameters = (ASTNode **)malloc(sizeof(ASTNode *));
    if(node->parameters == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode parameters failed");
    }
    add_pointer_to_storage(node->parameters);
    node->parameters[0] = var_without_null;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    return node;
}

ASTNode* create_while_node(ASTNode* condition, ASTNode* body) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
        add_pointer_to_storage(node);

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

ASTNode* create_return_node(ASTNode* value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
        add_pointer_to_storage(node);
    node->type = NODE_RETURN;
    node->left = value;
    node->right = node->next = node->condition = node->body = NULL;
    node->name = NULL;
    node->value = NULL;
    node->parameters = NULL;
    node->param_count = 0;
    node->arguments = NULL;
    node->arg_count = 0;
    if(value != NULL)
    {
    node->data_type = value->data_type;
    }
    else{
    node->data_type = TYPE_VOID;
    }
    return node;
}


ASTNode* create_function_call_node(char* name, ASTNode** arguments, int arg_count) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
        add_pointer_to_storage(node);
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

ASTNode* create_block_node(ASTNode* statements, DataType return_type) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
        if(node == NULL)
    {
        error_exit(ERR_INTERNAL, "Memory allocation for ASTNode failed");
    }
        add_pointer_to_storage(node);

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


void free_ast_node(ASTNode* node) {
    if (!node) return;

    if (node->left) free_ast_node(node->left);
    if (node->right) free_ast_node(node->right);
    if (node->next) free_ast_node(node->next);
    if (node->body) free_ast_node(node->body);
    if (node->condition) free_ast_node(node->condition);

    if (node->name) safe_free(node->name);
    if (node->value) safe_free(node->value);

    if (node->parameters) {
        for (int i = 0; i < node->param_count; ++i) {
            if (node->parameters[i]) free_ast_node(node->parameters[i]);
        }
        safe_free(node->parameters);
    }

    if (node->arguments) {
        for (int i = 0; i < node->arg_count; ++i) {
            if (node->arguments[i]) free_ast_node(node->arguments[i]);
        }
        safe_free(node->arguments);
    }

    safe_free(node);
}


void print_ast(ASTNode* node, int indent) {
    if (!node) return;


    for (int i = 0; i < indent; ++i) printf("  ");

    switch (node->type) {
        case NODE_PROGRAM:
            printf("Node type: PROGRAM\n");
            ASTNode *current_function = node->body;
            printf("  Functions:\n");
            while (current_function) {
                if (current_function->type == NODE_FUNCTION) {
                    for (int i = 0; i < indent + 2; ++i) printf("  ");
                    printf("- Function name: %s, return type: %d\n", current_function->name, current_function->data_type);
                }
                current_function = current_function->next;
            }
            break;

        case NODE_FUNCTION:
            printf("Node type: FUNCTION\n");
            for (int i = 0; i < indent + 1; ++i) printf("  ");
            printf("Name: %s, Return type: %d\n", node->name, node->data_type);
            break;

        case NODE_VARIABLE_DECLARATION:
            printf("Node type: VARIABLE_DECLARATION\n");
            for (int i = 0; i < indent + 1; ++i) printf("  ");
            printf("Name: %s, Data type: %d\n", node->name, node->data_type);
            break;

        case NODE_ASSIGNMENT:
            printf("Node type: ASSIGNMENT\n");
            for (int i = 0; i < indent + 1; ++i) printf("  ");
            printf("Variable: %s\n", node->name);
            break;

        case NODE_BINARY_OPERATION:
            printf("Node type: BINARY_OPERATION\n");
            for (int i = 0; i < indent + 1; ++i) printf("  ");
            printf("Data type: %d, Operator: %s\n", node->data_type, node->name);
            break;

        case NODE_LITERAL:
            printf("Node type: LITERAL\n");
            for (int i = 0; i < indent + 1; ++i) printf("  ");
            printf("Value: %s, Data type: %d\n", node->value, node->data_type);
            break;

        case NODE_IDENTIFIER:
            printf("Node type: IDENTIFIER\n");
            for (int i = 0; i < indent + 1; ++i) printf("  ");
            printf("Name: %s\n", node->name);
            break;

        case NODE_IF:
            printf("Node type: IF\n");
            //for (int i = 0; i < indent + 1; ++i) printf("  ");
            //printf("Condition type: %d\n", node->condition ? node->condition->data_type : -1);
            break;

        case NODE_WHILE:
            printf("Node type: WHILE\n");
            //for (int i = 0; i < indent + 1; ++i) printf("  ");
            //printf("Condition type: %d\n", node->condition ? node->condition->data_type : -1);
            break;

        case NODE_RETURN:
            printf("Node type: RETURN\n");
            break;

        case NODE_FUNCTION_CALL:
            printf("Node type: FUNCTION_CALL\n");
            for (int i = 0; i < indent + 1; ++i) printf("  ");
            printf("Name: %s, Arg count: %d\n", node->name, node->arg_count);
            if (node->arguments) {
                for (int i = 0; i < node->arg_count; ++i) {
                    for (int j = 0; j < indent + 2; ++j) printf("  ");
                    printf("Argument %d:\n", i + 1);
                    print_ast(node->arguments[i], indent + 3);
                }
            }
            break;

        case NODE_BLOCK:
            printf("Node type: BLOCK\n");
            break;

        default:
            printf("Node type: UNKNOWN\n");
            break;
    }
    if (node->left) {
        for (int i = 0; i < indent + 1; ++i) printf("  ");
        printf("Left:\n");
        print_ast(node->left, indent + 2);
    }

    if (node->right) {
        for (int i = 0; i < indent + 1; ++i) printf("  ");
        printf("Right:\n");
        print_ast(node->right, indent + 2);
    }

    if (node->condition) {
        for (int i = 0; i < indent + 1; ++i) printf("  ");
        printf("Condition:\n");
        print_ast(node->condition, indent + 2);
    }

    if (node->body) {
        for (int i = 0; i < indent + 1; ++i) printf("  ");
        printf("Body:\n");
        print_ast(node->body, indent + 2);
    }

    if (node->next) {
        for (int i = 0; i < indent; ++i) printf("  ");
        printf("Next:\n");
        print_ast(node->next, indent);
    }
}
