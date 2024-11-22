#include "codegen.h"
#include "ast.h"
#include "symtable.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"


#undef DEBUG_PARSER
#ifdef DEBUG_PARSER
#define LOG(...) fprintf(__VA_ARGS__)
#else
#define LOG(...)
#endif
// Global variable for storing the output file
static FILE *output_file;

void codegen_generate_function(ASTNode *function_node);
void codegen_generate_block(FILE *output, ASTNode *block_node, const char *current_function);
void codegen_generate_statement(FILE *output, ASTNode *statement_node, const char *current_function);
void codegen_generate_expression(FILE *output, ASTNode *node, const char *current_function);
void codegen_generate_function_call(FILE *output, ASTNode *node, const char *current_function);
void codegen_generate_variable_declaration(FILE *output, ASTNode *declaration_node);
void codegen_generate_assignment(FILE *output, ASTNode *assignment_node);
void codegen_generate_return(FILE *output, ASTNode *return_node);
void codegen_generate_if(FILE *output, ASTNode *if_node);
void codegen_generate_while(FILE *output, ASTNode *while_node);

const char* remove_last_prefix(const char* name) {
    static char buffer[1024]; 
    if (!name) {
        return NULL;
    }
    const char* last_underscore = strrchr(name, '_');
    if (last_underscore && *(last_underscore + 1) != '\0') {
        size_t new_len = last_underscore - name;
        if (new_len >= sizeof(buffer)) {
            LOG(stderr, "Error: Buffer overflow in remove_last_prefix.\n");
            exit(EXIT_FAILURE);
        }
        strncpy(buffer, name, new_len); 
        buffer[new_len] = '\0';         
        return buffer;
    }
    return name;
}


/*void collect_functions(ASTNode *program_node) {
    ASTNode *current_function = program_node->body;

    while (current_function) {
        if (current_function->type == NODE_FUNCTION) {
            function_count++;
            functions = realloc(functions, function_count * sizeof(FunctionInfo));
            functions[function_count - 1].name = string_duplicate(current_function->name);
            functions[function_count - 1].param_count = current_function->param_count;

            if (current_function->param_count > 0) {
                functions[function_count - 1].parameters = malloc(current_function->param_count * sizeof(char *));
                for (int i = 0; i < current_function->param_count; i++) {
                    functions[function_count - 1].parameters[i] = strdup(current_function->parameters[i]->name);
                }
            } else {
                functions[function_count - 1].parameters = NULL;
            }
        }
        current_function = current_function->next;
    }
}*/

char *escape_ifj24_string(const char *input);
static int label_counter = 0;

int generate_unique_label() {
    return label_counter++;
}

// Initialize code generator
void codegen_init(const char *filename) {
    if (filename) {
        output_file = fopen(filename, "w");
        if (!output_file) {
            fprintf(stderr, "Error: Cannot open output file %s for writing.\n", filename);
            exit(EXIT_FAILURE);
        }
    } else {
        output_file = stdout; 
    }
    fprintf(output_file, ".IFJcode24\n"); // Header for output file
}


// Finalize code generation
void codegen_finalize() {
    if (output_file) {
        fclose(output_file);
        output_file = NULL;
    }
}

void codegen_generate_program(ASTNode *program_node) {
    if (!program_node || program_node->type != NODE_PROGRAM) {
        LOG(stderr, "DEBUG: Invalid program node.\n");
        return;
    }

    LOG(stderr, "DEBUG: Starting code generation for program.\n");

    ASTNode *current_function = program_node->body;

    while (current_function) {
        if (current_function->type == NODE_FUNCTION && strcmp(current_function->name, "main") == 0) {
            LOG(stderr, "DEBUG: Found main function. Generating its code.\n");
            codegen_generate_function(current_function);
            break;
        }
        current_function = current_function->next;
    }

    current_function = program_node->body;
    while (current_function) {
        if (current_function->type == NODE_FUNCTION && strcmp(current_function->name, "main") != 0) {
            LOG(stderr, "DEBUG: Found function '%s'. Generating its code.\n", current_function->name);
            codegen_generate_function(current_function);
        }
        current_function = current_function->next;
    }
    fprintf(output_file, "CALL main\n");
    fprintf(output_file, "EXIT int@0\n");
}

void codegen_generate_function(ASTNode *function) {
    fprintf(output_file, "LABEL %s\n", function->name);
    fprintf(output_file, "CREATEFRAME\n");
    fprintf(output_file, "PUSHFRAME\n");

    for (int i = 0; i < function->param_count; i++) {
        fprintf(output_file, "DEFVAR LF@%s\n", remove_last_prefix(function->parameters[i]->name));
        fprintf(output_file, "POPS LF@%s\n", remove_last_prefix(function->parameters[i]->name));
    }

    codegen_generate_block(output_file,function->body, function->name);

    if (strcmp(function->name, "main") == 0) {
        fprintf(output_file, "POPFRAME\n");
        fprintf(output_file, "EXIT int@0\n");
    } else {
        fprintf(output_file, "POPFRAME\n");
        fprintf(output_file, "RETURN\n");
    }
}

// Function to generate code for a block of statements
void codegen_generate_block(FILE *output, ASTNode *block_node, const char *current_function) {
    ASTNode *current = block_node->body;
    output = output;
    while (current) {
        codegen_generate_statement(output_file, current, current_function);
        current = current->next;
    }
}

void codegen_generate_expression(FILE *output, ASTNode *node, const char *current_function) {
    if (node == NULL) {
        LOG(stderr, "Node is NULL\n");
        return;
    }
    LOG(stderr, "Node type: %d\n", node->type);
    switch (node->type) {
        case NODE_LITERAL:
            if (node->data_type == TYPE_INT) {
                fprintf(output, "PUSHS int@%s\n", node->value);
            } else if (node->data_type == TYPE_FLOAT) {
                double float_value = atof(node->value);
                fprintf(output, "PUSHS float@%.13a\n", float_value);
            } else if (node->data_type == TYPE_U8) {
                char *escaped_value = escape_ifj24_string(node->value);
                fprintf(output, "PUSHS string@%s\n", escaped_value);
                free(escaped_value);
            } else if (node->data_type == TYPE_NULL) {
                fprintf(output, "PUSHS nil@nil\n");
            } else if (node->data_type == TYPE_BOOL) {
                fprintf(output, "PUSHS bool@%s\n", strcmp(node->value, "true") == 0 ? "true" : "false");
            }
            break;

        case NODE_IDENTIFIER:
        {
            if (strcmp(node->name, "nil") == 0) {
                fprintf(output, "PUSHS nil@nil\n"); 
                fprintf(output, "EQS\n");           
                fprintf(output, "NOTS\n");          
            } else if (strcmp(node->name, "true") == 0) {
                fprintf(output, "PUSHS bool@true\n");
            } else if (strcmp(node->name, "false") == 0) {
                fprintf(output, "PUSHS bool@false\n");
            } else {
                LOG(stderr, "175");
                fprintf(output, "PUSHS LF@%s\n", remove_last_prefix(node->name));
            }
        }
            break;

        case NODE_BINARY_OPERATION:
            codegen_generate_expression(output, node->left, node->name);
            codegen_generate_expression(output, node->right, node->name);

            if (strcmp(node->name, "+") == 0) {
                fprintf(output, "ADDS\n");
            } else if (strcmp(node->name, "-") == 0) {
                fprintf(output, "SUBS\n");
            } else if (strcmp(node->name, "*") == 0) {
                fprintf(output, "MULS\n");
            } else if (strcmp(node->name, "/") == 0) {
                fprintf(output, "DIVS\n");
            } else if (strcmp(node->name, "<") == 0) {
                fprintf(output, "LTS\n");
            } else if (strcmp(node->name, "<=") == 0) {
                fprintf(output, "LTS\n");
                fprintf(output, "PUSHS bool@true\n");
                fprintf(output, "ORS\n");
            } else if (strcmp(node->name, ">") == 0) {
                fprintf(output, "GTS\n");
            } else if (strcmp(node->name, ">=") == 0) {
                fprintf(output, "GTS\n");
                fprintf(output, "PUSHS bool@true\n");
                fprintf(output, "ORS\n");
            } else if (strcmp(node->name, "==") == 0) {
                codegen_generate_expression(output, node->left, current_function);
                codegen_generate_expression(output, node->right, current_function);
                fprintf(output, "EQS\n");
                fprintf(output, "DEFVAR LF@temp_condition\n");
                fprintf(output, "POPS LF@temp_condition\n");
                fprintf(output, "PUSHS LF@temp_condition\n");
            } else if (strcmp(node->name, "!=") == 0) {
                codegen_generate_expression(output, node->left, current_function);
                codegen_generate_expression(output, node->right, current_function);
                fprintf(output, "EQS\n");
                fprintf(output, "NOTS\n");
                fprintf(output, "DEFVAR LF@temp_condition\n");
                fprintf(output, "POPS LF@temp_condition\n");
                fprintf(output, "PUSHS LF@temp_condition\n");
            }
            break;

        case NODE_FUNCTION_CALL:
            codegen_generate_function_call(output, node, current_function);
            break;

        default:
            fprintf(stderr, "Unsupported expression type for code generation, type: %d, name: %s\n", node->type, node->name);
            exit(1);
    }
}

void codegen_generate_function_call(FILE *output, ASTNode *node, const char *current_function) {
    if (node == NULL || node->type != NODE_FUNCTION_CALL) {
        fprintf(stderr, "Invalid function call node for code generation\n");
        exit(1);
    }
    if (strcmp(node->name, "ifj.write") == 0) {
        ASTNode *arg = node->arguments[0];
        if (arg->type == NODE_LITERAL) {
            if (arg->data_type == TYPE_U8) {
                char *escaped_value = escape_ifj24_string(arg->value);
                fprintf(output, "WRITE string@%s\n", escaped_value);
                free(escaped_value);
            } else if (arg->data_type == TYPE_INT) {
                fprintf(output, "WRITE int@%s\n", arg->value);
            } else if (arg->data_type == TYPE_FLOAT) {
                double float_value = atof(arg->value);
                fprintf(output, "WRITE float@%.13a\n", float_value);
            }
        } else if (arg->type == NODE_IDENTIFIER) {
            fprintf(output, "WRITE LF@%s\n", remove_last_prefix(arg->name));
        }
    }
    else if (strcmp(node->name, "ifj.readi32") == 0) {
        fprintf(output, "DEFVAR LF@%%retval\n");
        fprintf(output, "READ LF@%%retval int\n");
    } else if (strcmp(node->name, "ifj.readf64") == 0) {
        fprintf(output, "DEFVAR LF@%%retval\n");
        fprintf(output, "READ LF@%%retval float\n");
    } else if (strcmp(node->name, "ifj.readstr") == 0) {
        fprintf(output, "DEFVAR LF@%%retval\n");
        fprintf(output, "READ LF@%%retval string\n");
        fprintf(output, "PUSHS LF@%%retval\n");
    } else if (!strcmp(node->name, "ifj.concat")) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        codegen_generate_expression(output, node->arguments[1], current_function);
        fprintf(output, "DEFVAR LF@%%retval\n");
        fprintf(output, "CONCAT LF@%%retval\n");
    } else if (!strcmp(node->name, "ifj.length")) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        fprintf(output, "DEFVAR LF@%%retval\n");
        fprintf(output, "STRLEN LF@%%retval\n");
    } else if (!strcmp(node->name, "ifj.i2f")) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        fprintf(output, "DEFVAR LF@%%retval\n"); 
        fprintf(output, "INT2FLOAT LF@%%retval LF@%%retval\n"); 
        fprintf(output, "PUSHS LF@%%retval\n"); 
    } else if (!strcmp(node->name, "ifj.f2i")) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        fprintf(output, "DEFVAR LF@%%retval\n");
        fprintf(output, "FLOAT2INT LF@%%retval LF@%%retval\n");
        fprintf(output, "PUSHS LF@%%retval\n");
    } else if (!strcmp(node->name, "ifj.substring")) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        codegen_generate_expression(output, node->arguments[1], current_function);
        codegen_generate_expression(output, node->arguments[2], current_function);
        fprintf(output, "DEFVAR LF@%%retval\n");
        fprintf(output, "SUBSTR LF@%%retval\n");
    } else if (!strcmp(node->name, "ifj.ord")) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        codegen_generate_expression(output, node->arguments[1], current_function);
        fprintf(output, "DEFVAR LF@%%retval\n");
        fprintf(output, "ORD LF@%%retval\n");
    } else if (!strcmp(node->name, "ifj.chr")) {
        codegen_generate_expression(output, node->arguments[0], current_function);
        fprintf(output, "DEFVAR LF@%%retval\n");
        fprintf(output, "INT2CHAR LF@%%retval\n");
    } else {
        for (int i = 0; i < node->arg_count; ++i) {
            codegen_generate_expression(output, node->arguments[i], current_function);
        }

        fprintf(output, "CALL %s\n", node->name);

        if (node->left) { 
            fprintf(output, "POPS LF@%s\n", node->left->name);
        }
    }
}

void codegen_generate_statement(FILE *output, ASTNode *node, const char *current_function) {
    if (node == NULL) {
        fprintf(stderr, "Invalid statement node for code generation\n");
        exit(1);
    }

    switch (node->type) {
        case NODE_VARIABLE_DECLARATION:
            fprintf(output, "DEFVAR LF@%s\n", remove_last_prefix(node->name));
            if (node->left != NULL) {
                if (node->left->type == NODE_FUNCTION_CALL &&
                    (strcmp(node->left->name, "ifj.readi32") == 0 ||
                     strcmp(node->left->name, "ifj.readf64") == 0 ||
                     strcmp(node->left->name, "ifj.readstr") == 0)) {
                    if (strcmp(node->left->name, "ifj.readi32") == 0) {
                        fprintf(output, "DEFVAR LF@%%retval\n");

                        fprintf(output, "READ LF@%%retval int\n");
                    } else if (strcmp(node->left->name, "ifj.readf64") == 0) {
                        fprintf(output, "READ LF@%%retval float\n");
                    } else if (strcmp(node->left->name, "ifj.readstr") == 0) {
                        fprintf(output, "DEFVAR LF@%%retval\n");
                        fprintf(output, "READ LF@%%retval string\n");
                    }
                    fprintf(output, "MOVE LF@%s LF@%%retval\n", remove_last_prefix(node->name));
                } else {
                    codegen_generate_expression(output, node->left, current_function);
                    fprintf(output, "POPS LF@%s\n", remove_last_prefix(node->name));
                }
            }
            break;

        case NODE_ASSIGNMENT:
            codegen_generate_expression(output, node->left, current_function);
            if (node->left->type == NODE_FUNCTION_CALL &&
                (strcmp(node->left->name, "ifj.readi32") == 0 ||
                 strcmp(node->left->name, "ifj.readf64") == 0 ||
                 strcmp(node->left->name, "ifj.readstr") == 0)) {
                if (strcmp(node->left->name, "ifj.readi32") == 0) {
                    fprintf(output, "READ LF@%%retval int\n");
                } else if (strcmp(node->left->name, "ifj.readf64") == 0) {
                    fprintf(output, "READ LF@%%retval float\n");
                } else if (strcmp(node->left->name, "ifj.readstr") == 0) {
                    fprintf(output, "READ LF@%%retval string\n");
                }
                fprintf(output, "MOVE LF@%s LF@%%retval\n", remove_last_prefix(node->name));
            } else {
                fprintf(output, "POPS LF@%s\n", remove_last_prefix(node->name));
            }
            break;

        case NODE_RETURN:
            // Generate code for return statement
            if (node->left != NULL) {
                codegen_generate_expression(output, node->left, node->name);
                //fprintf(output, "POPS LF@%%retval\n");
            }
            //fprintf(output, "RETURN\n");
            break;

        case NODE_IF:
            // Generate code for if statement
        {
            LOG(stderr, "DEBUG: Entering NODE_IF case\n");
            codegen_generate_if(output, node);
        }
            break;

        case NODE_WHILE:
            // Generate code for while loop
        {
            int label_num = generate_unique_label();
            fprintf(output, "LABEL $while_start_%d\n", label_num);
            codegen_generate_expression(output, node->condition, node->name);
            fprintf(output, "PUSHS bool@true\n");
            fprintf(output, "JUMPIFNEQS $while_end_%d\n", label_num);
            codegen_generate_block(output, node->body, node->name); // Generate code for while body
            fprintf(output, "JUMP $while_start_%d\n", label_num);
            fprintf(output, "LABEL $while_end_%d\n", label_num);
        }
            break;

        case NODE_FUNCTION_CALL:
        {
            codegen_generate_function_call(output, node, node->name);
        }
            break;

        default:
            fprintf(stderr, "Unsupported statement type for code generation\n");
            exit(1);
    }
}

void codegen_generate_variable_declaration(FILE *output, ASTNode *declaration_node) {
    if (!declaration_node || !declaration_node->name) {
        fprintf(stderr, "Error: Invalid variable declaration.\n");
        exit(1);
    }

    fprintf(output, "DEFVAR LF@%s\n", remove_last_prefix(declaration_node->name));
    if (declaration_node->left) {
        codegen_generate_expression(output, declaration_node->left, NULL);
        fprintf(output, "POPS LF@%s\n", remove_last_prefix(declaration_node->name));
    }
}

void codegen_generate_assignment(FILE *output, ASTNode *assignment_node) {
    codegen_generate_expression(output, assignment_node->left, assignment_node->name);
    fprintf(output, "POPS LF@%s\n", remove_last_prefix(assignment_node->name));
}

void codegen_generate_return(FILE *output, ASTNode *return_node) {
    if (return_node->left) { 
        codegen_generate_expression(output, return_node->left, return_node->name);
        fprintf(output, "POPS LF@%%retval\n");
    }
    fprintf(output, "RETURN\n");
}

void codegen_generate_if(FILE *output, ASTNode *if_node) {
    static int if_label_count = 0;
    int current_label = if_label_count++;
    if (if_node->condition->type == NODE_BINARY_OPERATION &&
        strcmp(if_node->condition->name, "==") == 0 &&
        if_node->condition->right->data_type == TYPE_NULL) {

        fprintf(output, "PUSHS nil@nil\n"); 
        fprintf(output, "PUSHS LF@%s\n", remove_last_prefix(if_node->condition->left->name)); 
        fprintf(output, "EQS\n"); 
        fprintf(output, "PUSHS bool@true\n");
        fprintf(output, "JUMPIFNEQS $else_%d\n", current_label);

    } else if (if_node->condition->data_type == TYPE_INT_NULLABLE || if_node->condition->data_type == TYPE_FLOAT_NULLABLE || if_node->condition->data_type == TYPE_U8_NULLABLE) {
        fprintf(output, "PUSHS nil@nil\n");
        fprintf(output, "PUSHS LF@%s\n", remove_last_prefix(if_node->condition->name));
        fprintf(output, "EQS\n"); 
        fprintf(output, "NOTS\n"); 
        fprintf(output, "PUSHS bool@true\n");
        fprintf(output, "JUMPIFNEQS $else_%d\n", current_label);

    } else if (if_node->condition->type == NODE_BINARY_OPERATION &&
               strcmp(if_node->condition->name, "!=") == 0 &&
               if_node->condition->right->data_type == TYPE_NULL) {

        fprintf(output, "PUSHS nil@nil\n");
        fprintf(output, "PUSHS LF@%s\n", remove_last_prefix(if_node->condition->left->name)); 
        fprintf(output, "EQS\n");
        fprintf(output, "NOTS\n"); 
        fprintf(output, "PUSHS bool@true\n");
        fprintf(output, "JUMPIFNEQS $else_%d\n", current_label);

    } else if (if_node->condition->type == NODE_IDENTIFIER) {
        fprintf(output, "PUSHS LF@%s\n", remove_last_prefix(if_node->condition->name));
        fprintf(output, "PUSHS nil@nil\n"); 
        fprintf(output, "EQS\n");           
        fprintf(output, "NOTS\n");          
        fprintf(output, "PUSHS bool@true\n");
        fprintf(output, "JUMPIFNEQS $else_%d\n", current_label);
    } else {
            codegen_generate_expression(output, if_node->condition, if_node->name);
            fprintf(output, "PUSHS bool@true\n");
            fprintf(output, "JUMPIFNEQS $else_%d\n", current_label);
    }
    codegen_generate_block(output, if_node->body, if_node->name);
    fprintf(output, "JUMP $endif_%d\n", current_label);
    fprintf(output, "LABEL $else_%d\n", current_label);
    if (if_node->left != NULL) {
        codegen_generate_block(output, if_node->left, if_node->name);
    }

    fprintf(output, "LABEL $endif_%d\n", current_label);
}

void codegen_generate_while(FILE *output, ASTNode *while_node) {
    static int while_label_count = 0;
    int current_label = while_label_count++;
    fprintf(output, "LABEL $while_start_%d\n", current_label);
    codegen_generate_expression(output, while_node->condition, while_node->name);
    fprintf(output, "PUSHS int@0\n");
    fprintf(output, "GTS\n");
    fprintf(output, "JUMPIFNEQS $while_end_%d\n", current_label);

    codegen_generate_block(output, while_node->body, while_node->name);
    fprintf(output, "JUMP $while_start_%d\n", current_label);
    fprintf(output, "LABEL $while_end_%d\n", current_label);
}

char *escape_ifj24_string(const char *input) {
    size_t length = strlen(input);
    size_t buffer_size = length * 4 + 1; // reserve space for escaping
    char *escaped_string = malloc(buffer_size);
    if (!escaped_string) {
        fprintf(stderr, "Memory allocation failed for escaped string.\n");
        exit(EXIT_FAILURE);
    }

    size_t index = 0;
    for (size_t i = 0; i < length; i++) {
        unsigned char c = input[i];

        // Escape non-printable characters and specific characters required by IFJcode24
        if (c <= 32 || c == 35 || c == 92 || c >= 127) { // ASCII 0-32, 35 (#), 92 (\), 127+ (non-printable)
            index += snprintf(escaped_string + index, buffer_size - index, "\\%03d", c);
        } else {
            escaped_string[index++] = c;
        }
    }

    escaped_string[index] = '\0';
    return escaped_string;
}

