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

enum token_type {
    MOVE_RIGHT = '>',
    MOVE_LEFT = '<',
    INCREMENT = '+',
    DECREMENT = '-',
    OUTPUT = '.',
    INPUT = ',',
    JUMP_FORWARD = '[',
    JUMP_BACKWARD = ']',
};

struct token {
    enum token_type type;
    unsigned int index;
};

struct instructions {
    struct token *items;
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
    for (size_t i = 0; i < strlen(program); i++) {
        switch (program[i]) {
        case '>':
            da_append(intrs, ((struct token) { MOVE_RIGHT, i }));
            break;
        case '<':
            da_append(intrs, ((struct token) { MOVE_LEFT, i }));
            break;
        case '+':
            da_append(intrs, ((struct token) { INCREMENT, i }));
            break;
        case '-':
            da_append(intrs, ((struct token) { DECREMENT, i }));
            break;
        case '.':
            da_append(intrs, ((struct token) { OUTPUT, i }));
            break;
        case ',':
            da_append(intrs, ((struct token) { INPUT, i }));
            break;
        case '[':
            da_append(intrs, ((struct token) { JUMP_FORWARD, i }));
            break;
        case ']':
            da_append(intrs, ((struct token) { JUMP_BACKWARD, i }));
            break;
        default:
            break;
        }
    }
}

enum result_type {
    OK,
    ERROR_UNMATCHED_JF,
    ERROR_UNMATCHED_JB,
};

struct result {
    enum result_type type;
    unsigned int index;
};

struct brackets {
    unsigned int *items;
    size_t count;
    size_t capacity;
};

struct result matching_bracket(struct instructions *intrs) {
    struct brackets counter = { NULL, 0, 0 };

    for (size_t i = 0; i < intrs->count; i++) {
        struct token t = intrs->items[i];
        if (t.type == JUMP_FORWARD) {
            da_append(&counter, t.index);
        } else if (t.type == JUMP_BACKWARD) {
            if (counter.count == 0) {
                return (struct result){ ERROR_UNMATCHED_JB, t.index };
            }
            counter.count--;
        }
    }

    if (counter.count != 0) {
        return (struct result){ ERROR_UNMATCHED_JF, counter.items[0] };
    }

    if (counter.items != NULL) {
        free(counter.items);
    }

    return (struct result){ OK, 0 };
}

void display_error(struct result r, char *program, char *name) {
    if (r.type == OK) {
        return;
    }

    unsigned int line = 1;
    unsigned int col = 1;

    for (size_t i = 0; i < r.index; i++) {
        if (program[i] == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }
    }

    switch (r.type) {
    case ERROR_UNMATCHED_JF:
        fprintf(stderr, "%s:%d:%d: Unmatched Jump Forward\n", name, line, col);
        break;
    case ERROR_UNMATCHED_JB:
        fprintf(stderr, "%s:%d:%d: Unmatched Jump Backward\n", name, line, col);
        break;
    default:
        break;
    }
}

void generate_assembly_move_right(FILE *f, unsigned char count) {
    if (count == 1) {
        fprintf(f, "    inc qword [pointer]         ; increment pointer to current cell\n");
    } else {
        fprintf(f, "    add qword [pointer], %d\n", count);
        fprintf(f, "                                ; increment pointer to current cell\n");
    }
}

void generate_assembly_move_left(FILE *f, unsigned char count) {
    if (count == 1) {
        fprintf(f, "    dec qword [pointer]         ; decrement pointer to current cell\n");
    } else {
        fprintf(f, "    sub qword [pointer], %d\n", count);
        fprintf(f, "                                ; decrement pointer to current cell\n");
    }
}

void generate_assembly_increment(FILE *f, unsigned char count) {
    if (count == 1) {
        fprintf(f, "    mov rax, [pointer]          ; move pointer to current cell to rax\n");
        fprintf(f, "    inc byte [rax]              ; increment current cell\n");
    } else {
        fprintf(f, "    mov rax, [pointer]          ; move pointer to current cell to rax\n");
        fprintf(f, "    add byte [rax], %d\n", count);
        fprintf(f, "                                ; increment current cell\n");
    }
}

void generate_assembly_decrement(FILE *f, unsigned char count) {
    if (count == 1) {
        fprintf(f, "    mov rax, [pointer]          ; move pointer to current cell to rax\n");
        fprintf(f, "    dec byte [rax]              ; increment current cell\n");
    } else {
        fprintf(f, "    mov rax, [pointer]          ; move pointer to current cell to rax\n");
        fprintf(f, "    sub byte [rax], %d\n", count);
        fprintf(f, "                                ; increment current cell\n");
    }
}

void generate_assembly_output(FILE *f) {
    fprintf(f, "    mov rax, 1                  ; system call for write\n");
    fprintf(f, "    mov rdi, 1                  ; stdout\n");
    fprintf(f, "    mov rsi, [pointer]          ; pointer to current cell\n");
    fprintf(f, "    mov rdx, 1                  ; length\n");
    fprintf(f, "    syscall                     ; invoke operating system to read\n");
}

void generate_assembly_input(FILE *f) {
    fprintf(f, "    mov rax, 0                  ; system call for read\n");
    fprintf(f, "    mov rdi, 0                  ; stdin\n");
    fprintf(f, "    mov rsi, [pointer]          ; pointer to current cell\n");
    fprintf(f, "    mov rdx, 1                  ; length\n");
    fprintf(f, "    syscall                     ; invoke operating system to read\n");
}

void generate_assembly_jump_forward(FILE *f, unsigned int label) {
    fprintf(f, "    mov rax, [pointer]          ; move pointer to current cell to rax\n");
    fprintf(f, "    cmp byte [rax], 0           ; compare current cell to 0\n");
    fprintf(f, "    je .LB%d\n", label);
    fprintf(f, "                                ; jump to closing bracket if equal\n");
    fprintf(f, ".LF%d:\n", label);
}

void generate_assembly_jump_backward(FILE *f, unsigned int label) {
    fprintf(f, "    mov rax, [pointer]          ; move pointer to current cell to rax\n");
    fprintf(f, "    cmp byte [rax], 0           ; compare current cell to 0\n");
    fprintf(f, "    jne .LF%d\n", label);
    fprintf(f, "                                ; jump to opening bracket if not equal\n");
    fprintf(f, ".LB%d:\n", label);
}

void generate_assembly_block(FILE *f, struct instructions *intrs, size_t *index) {
    while (*index < intrs->count) {
        switch (intrs->items[*index].type) {
        case MOVE_RIGHT: {
            unsigned char count = 1;
            while (*index < intrs->count - 1 &&
                   intrs->items[*index + 1].type == MOVE_RIGHT) {
                count++;
                (*index)++;
            }
            generate_assembly_move_right(f, count);
            break;
        }
        case MOVE_LEFT: {
            unsigned char count = 1;
            while (*index < intrs->count - 1 &&
                   intrs->items[*index + 1].type == MOVE_LEFT) {
                count++;
                (*index)++;
            }
            generate_assembly_move_left(f, count);
            break;
        }
        case INCREMENT: {
            unsigned char count = 1;
            while (*index < intrs->count - 1 &&
                   intrs->items[*index + 1].type == INCREMENT) {
                count++;
                (*index)++;
            }
            generate_assembly_increment(f, count);
            break;
        }
        case DECREMENT: {
            unsigned char count = 1;
            while (*index < intrs->count - 1 &&
                   intrs->items[*index + 1].type == DECREMENT) {
                count++;
                (*index)++;
            }
            generate_assembly_decrement(f, count);
            break;
        }
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
    char *program = NULL;
    struct instructions *intrs = NULL;
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

    int result = 0;

    if (help) {
        argparse_print_help(parser);

        result = 0;
        goto cleanup;
    }

    program = read_input_program(file);

    intrs = instructions_new();
    instructions_parse(intrs, program);

    struct result r = matching_bracket(intrs);
    if (r.type != OK) {
        display_error(r, program, file);

        result = 1;
        goto cleanup;
    }

    generate_assembly(intrs, output);

cleanup:
    if (intrs != NULL) {
        instructions_free(intrs);
    }
    if (program != NULL) {
        free(program);
    }
    if (parser != NULL) {
        argparse_free(parser);
    }

    return result;
}
