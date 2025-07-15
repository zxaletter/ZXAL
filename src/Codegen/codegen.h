// #ifndef CODEGEN_H
// #define CODEGEN_H

// #include "symbols.h"

// #define MAX_REGISTERS 10
// #define FUNCTION_REGS_COUNT 6

// typedef enum {
// 	REGISTER_USED,
// 	REGISTER_FREE
// } register_state;

// struct Register {
// 	register_state state;
// 	char* name;
// };

// struct RegisterTable {
// 	struct Register* register_table[MAX_REGISTERS];
// 	int capacity;
// };

// typedef enum {
// 	SECTION_TEXT,
// 	SECTION_DATA
// } section_t;

// typedef struct {
// 	FILE* file;
// 	size_t current_text_pos;
// 	size_t current_data_pos;
// } asm_writer;


// #define LABEL_STACK_CAPACITY 50
// typedef struct {
// 	int size;
// 	int capacity;
// 	int top;
// 	int* labels;
// } label_stack;

// label_stack* create_label_stack();
// void push_label(int label);
// bool is_label_stack_empty();
// int peek_label();
// void free_label_stack();

// void expression_codegen(Node* expr);
// void statement_codegen(Node* stmt);
// void second_pass_globals_codegen(Node* global_node);
// void first_pass_globals_codegen(Node* global_node);
// void codegen(Node* root, char* output);
// void write_asm_to_section(asm_writer* writer, char* text, section_t type);
// void init_asm_writer(char* output);


// int label_create();
// char* label_name(int label);
// char* scratch_name(struct RegisterTable* rt_struct, int i);
// int scratch_alloc(struct RegisterTable* rt_struct);
// void scratch_free(struct RegisterTable* rt_struct, int i);

// void free_register(struct Register* reg);
// void free_register_table();
// #endif