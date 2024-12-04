/*
 * Project Name: Implementation of the IFJ24 Compiler
 * Authors: 
 */

#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"
#include "parser.h"
#include "error.h"
#include "symtable.h"
#include "ast.h" 
#include "codegen.h" 
#include "utils.h"

int main(int argc, char *argv[]) {
    FILE *source_file = stdin; 
    const char *output_filename = NULL;

    if (argc > 3) {
        fprintf(stderr, "Usage: %s [source_file] [output_file]\n", argv[0]);
        return ERR_INTERNAL;
    }
    if (argc > 1) {
        source_file = fopen(argv[1], "r");
        if (!source_file) {
            fprintf(stderr, "Error opening file: %s\n", argv[1]);
            return ERR_INTERNAL;
        }
    }

    if (argc > 2) {
        output_filename = argv[2];
    }

    init_pointers_storage(5);

    Scanner scanner;
    scanner_init(source_file, &scanner);


    parser_init(&scanner);


    ASTNode* ast_root = parse_program(&scanner);


    //print_ast(ast_root, 0);


    codegen_init(output_filename);


    codegen_generate_program(ast_root);


    codegen_finalize();

    scanner_free(&scanner);


    //free_ast_node(ast_root);


    fclose(source_file);

    cleanup_pointers_storage();

    return ERR_OK;  
}