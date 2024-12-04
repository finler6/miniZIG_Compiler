/**
 * @file main.c
 *
 * Main file of the IFJ2021 compiler project.
 *
 * IFJ Project 2024, Team 'xstepa77'
 *
 * @author <xlitvi02> Gleb Litvinchuk
 * @author <xstepa77> Pavel Stepanov
 * @author <xkovin00> Viktoriia Kovina
 * @author <xshmon00> Gleb Shmonin
 */

#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"
#include "parser.h"
#include "error.h"
#include "ast.h"
#include "codegen.h"
#include "utils.h"

/**
 * Main function of the compiler project.
 * Initializes scanner, parser, and code generator.
 */
int main(int argc, char *argv[]) {
    FILE *source_file = stdin; // Default source file is standard input
    const char *output_filename = NULL; // Default output filename is NULL

    // Check the number of arguments passed to the program
    if (argc > 3) {
        fprintf(stderr, "Usage: %s [source_file] [output_file]\n", argv[0]);
        return ERR_INTERNAL;
    }
    // If a source file is specified, open it
    if (argc > 1) {
        source_file = fopen(argv[1], "r");
        if (!source_file) {
            fprintf(stderr, "Error opening file: %s\n", argv[1]);
            return ERR_INTERNAL;
        }
    }

    // If an output file is specified, set the output filename
    if (argc > 2) {
        output_filename = argv[2];
    }

    // Initialize memory management for pointers (utils.c)
    init_pointers_storage(5);

    // Initialize scanner (scanner.c)
    Scanner scanner;
    scanner_init(source_file, &scanner);

    // Initialize parser (parser.c)
    parser_init(&scanner);

    // Parse the source file and generate an abstract syntax tree (AST) (ast.c)
    ASTNode* ast_root = parse_program(&scanner);

    // Initialize code generator (codegen.c)
    codegen_init(output_filename);

    // Generate code from the AST (codegen.c)
    codegen_generate_program(ast_root);

    // Finalize code generation (codegen.c)
    codegen_finalize();

    // Close the source file
    fclose(source_file);

    // Cleanup memory used for pointers (utils.c)
    cleanup_pointers_storage();

    return ERR_OK;  // Return success status
}
