#define ARGPARSE_IMPLEMENTATION
#include "argparse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN 256
#define PROGRAM_LEN 8192

char *read_input_program(char *name) {
    FILE *f = NULL;
    char *program = calloc(PROGRAM_LEN, sizeof(char));
    unsigned int count = 0;
    unsigned int capacity = PROGRAM_LEN;

    if (name == NULL) {
        f = stdin;
    } else {
        f = fopen(name, "r");
    }

    char buffer[MAX_LEN];
    while (fgets(buffer, MAX_LEN, f)) {
        unsigned int len = strlen(buffer);

        if (capacity < count + len + 1) {
            capacity *= 2;
            program = realloc(program, capacity * sizeof(char));
        }

        strcat(program, buffer);
        count += len;
        program[count] = '\0';
    }

    if (name != NULL) {
        fclose(f);
    }

    return program;
}

// TODO: have a struct for tokens
enum {
    MOVE_RIGHT = '>',
    MOVE_LEFT = '<',
    INCREMENT = '+',
    DECREMENT = '-',
    OUTPUT = '.',
    INPUT = ',',
    JUMP_FORWARD = '[',
    JUMP_BACKWARD = ']',
};

struct instructions {
    unsigned char *items;
    size_t count;
    size_t capacity;
};

struct instructions *instructions_new() {
    struct instructions *intrs = malloc(sizeof(struct argparse_parser));

    intrs->items = NULL;
    intrs->count = 0;
    intrs->capacity = 0;

    return intrs;
}

void instructions_free(struct instructions *intrs) {
    free(intrs->items);
    free(intrs);
}

void instructions_parse(struct instructions *intrs, char *program) {
    for (char *c = program; *c != '\0'; c++) {
        switch (*c) {
        case '>':
            da_append(intrs, MOVE_RIGHT);
            break;
        case '<':
            da_append(intrs, MOVE_LEFT);
            break;
        case '+':
            da_append(intrs, INCREMENT);
            break;
        case '-':
            da_append(intrs, DECREMENT);
            break;
        case '.':
            da_append(intrs, OUTPUT);
            break;
        case ',':
            da_append(intrs, INPUT);
            break;
        case '[':
            da_append(intrs, JUMP_FORWARD);
            break;
        case ']':
            da_append(intrs, JUMP_BACKWARD);
            break;
        default:
            break;
        }
    }
}

unsigned int matching_bracket(struct instructions *intrs) {
    // TODO: add error messages
    unsigned int counter = 0;

    for (size_t i = 0; i < intrs->count; i++) {
        if (intrs->items[i] == JUMP_FORWARD) {
            counter++;
        } else if (intrs->items[i] == JUMP_BACKWARD) {
            if (counter == 0) {
                return 0;
            }
            counter--;
        }
    }

    if (counter != 0) {
        return 0;
    }

    return 1;
}

void generate_assembly_move_right(FILE *f) {
    fprintf(f, "    inc qword [pointer]         ; increment pointer to current cell\n");
}

void generate_assembly_move_left(FILE *f) {
    fprintf(f, "    dec qword [pointer]         ; decrement pointer to current cell\n");
}

void generate_assembly_increment(FILE *f) {
    fprintf(f, "    mov rax, [pointer]          ; move pointer to current cell to rax\n");
    fprintf(f, "    inc byte [rax]              ; increment current cell\n");
}

void generate_assembly_decrement(FILE *f) {
    fprintf(f, "    mov rax, [pointer]          ; move pointer to current cell to rax\n");
    fprintf(f, "    dec byte [rax]              ; increment current cell\n");
}

void generate_assembly_output(FILE *f) {
    //TODO: this
}

void generate_assembly_input(FILE *f) {
}

void generate_assembly_jump_forward(FILE *f, unsigned int label) {
    fprintf(f, "    mov rax, [pointer]          ; move pointer to current cell to rax\n");
    fprintf(f, "    cmp byte [rax], 0           ; compare current cell to 0\n");
    fprintf(f, "    je .LB%d                     ; jump to closing bracket if equal\n", label);
    fprintf(f, ".LF%d:\n", label);
}

void generate_assembly_jump_backward(FILE *f, unsigned int label) {
    fprintf(f, "    mov rax, [pointer]          ; move pointer to current cell to rax\n");
    fprintf(f, "    cmp byte [rax], 0           ; compare current cell to 0\n");
    fprintf(f, "    jne .LF%d                    ; jump to opening bracket if not equal\n", label);
    fprintf(f, ".LB%d:\n", label);
}

void generate_assembly_block(FILE *f, struct instructions *intrs, size_t *index) {
    while (*index < intrs->count) {
        switch (intrs->items[*index]) {
        case MOVE_RIGHT:
            generate_assembly_move_right(f);
            break;
        case MOVE_LEFT:
            generate_assembly_move_left(f);
            break;
        case INCREMENT:
            generate_assembly_increment(f);
            break;
        case DECREMENT:
            generate_assembly_decrement(f);
            break;
        case OUTPUT:
            generate_assembly_output(f);
            break;
        case INPUT:
            generate_assembly_input(f);
            break;
        case JUMP_FORWARD: {
            unsigned int label = *index;
            generate_assembly_jump_forward(f, label);
            (*index)++;
            generate_assembly_block(f, intrs, index);
            generate_assembly_jump_backward(f, label);
            break;
        }
        case JUMP_BACKWARD:
            return;
        default:
            break;
        }

        (*index)++;
    }
}

void generate_assembly(struct instructions *intrs, char *name) {
    FILE *f = NULL;

    if (name == NULL) {
        f = stdout;
    } else {
        f = fopen(name, "w");
    }

    fprintf(f, "global _start\n");
    fprintf(f, "\n");
    fprintf(f, "section .text\n");
    fprintf(f, "_start:\n");
    fprintf(f, "    mov rax, 12                 ; system call for brk\n");
    fprintf(f, "    mov rdi, 0                  ; no arguments\n");
    fprintf(f, "    syscall                     ; invoke operating system to do the brk\n");
    fprintf(f, "\n");
    fprintf(f, "    mov [tape], rax             ; save pointer to tape\n");
    fprintf(f, "    mov [pointer], rax          ; save pointer to current cell\n");
    fprintf(f, "\n");
    fprintf(f, "    add rax, 30000              ; move pointer to end of tape\n");
    fprintf(f, "    mov rdi, rax\n");
    fprintf(f, "    mov rax, 12                 ; system call for brk\n");
    fprintf(f, "    syscall                     ; invoke operating system to do the brk\n");

    fprintf(f, "\n");

    size_t index = 0;
    generate_assembly_block(f, intrs, &index);

    fprintf(f, "\n");

    fprintf(f, "    mov rax, 12                 ; system call for brk\n");
    fprintf(f, "    mov rdi, tape               ; pointer to start of tape\n");
    fprintf(f, "    syscall                     ; invoke operating system to do the brk\n");
    fprintf(f, "\n");
    fprintf(f, "    mov rax, 60                 ; system call for exit\n");
    fprintf(f, "    xor rdi, rdi                ; exit code 0\n");
    fprintf(f, "    syscall                     ; invoke operating system to exit\n");
    fprintf(f, "\n");
    fprintf(f, "section .data\n");
    fprintf(f, "tape: dq 0                      ; pointer to tape\n");
    fprintf(f, "pointer: dq 0                   ; pointer to current cell\n");

    if (name != NULL) {
        fclose(f);
    }
}

int main(int argc, char **argv) {
    struct argparse_parser *parser =
        argparse_new("bf compiler", "description", "0.0.1");
    argparse_add_argument(parser, 'h', "help", "print help",
                          ARGUMENT_TYPE_FLAG);
    argparse_add_argument(parser, 'f', "file", "file name",
                          ARGUMENT_TYPE_VALUE);
    argparse_add_argument(parser, 'o', "output", "file name",
                          ARGUMENT_TYPE_VALUE);

    argparse_parse(parser, argc, argv);

    unsigned int help = argparse_get_flag(parser, "help");
    char *file = argparse_get_value(parser, "file");
    char *output = argparse_get_value(parser, "output");

    if (help) {
        argparse_print_help(parser);

        argparse_free(parser);
        return 0;
    }

    char *program = read_input_program(file);

    struct instructions *intrs = instructions_new();
    instructions_parse(intrs, program);

    if (!matching_bracket(intrs)) {
        printf("Error: unmatched brackets\n");
        return 1;
    }

    generate_assembly(intrs, output);

    instructions_free(intrs);
    free(program);
    argparse_free(parser);
}

// TODO: think of optimizations
