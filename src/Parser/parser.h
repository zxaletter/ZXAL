#ifndef PARSER_H
#define PARSER_H

#include "Lexer/lexer.h"
#include "Semantics/symbols.h"
#include "auxiliaries.h"
#include "errorsandcontext.h"
#include "memallocator.h"

typedef struct {
	int size;
	int capacity;
	Token* end;
} Parser;


bool valid_function_return_type(token_t type);
bool match_token(Parser* parser, char expected);
token_t peek_token_type(Parser* parser);
Token peek_token(Parser* parser);
token_t peek_next_token_type(Parser* parser);
Token advance_parser(Parser* parser);
bool at_token_eof(Parser* parser);

node_t get_op_kind(Token* token);
data_t get_type(Token* token);
char* get_token_string(token_t type);

Node* create_node(Arena* arena, node_t type, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* create_string_node(Arena* arena, node_t type, char* id, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* create_char_node(Arena* arena, node_t type, char ch, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* create_int_node(Arena* arena, node_t type, int val, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* parse_factor(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_unary(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_multiplicative(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_additive(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_relational(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_logical_and(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_logical_or(Arena* arena, Parser* parser, FileInfo* info);

Node* parse_statement(Arena* arena,Parser* parser, FileInfo* info);
Node* parse_block(Arena* arena,Parser* parser, FileInfo* info);
Node* parse_parameters(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_function(Arena* arena,Parser* parser, FileInfo* info);
Node* parse_enum_body(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_enum(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_struct(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_switch(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_args(Arena* arena, Parser* parser, FileInfo* info);
Node* parse_array_list(Arena* arena, Parser* parser, FileInfo* info, int* element_count);
Node* parse_let(Arena* arena, Parser* parser, FileInfo* info);
Node* parse(Arena* arena, Token* tokens, FileInfo* info);
Parser initialize_parser(Token* tokens);

// void add_node_to_node_table(Node* node);
// void init_node_table();
// NodeTable create_node_table();
// void free_node_table();

void synchronize(Parser* parser, token_t* synchronizations);
Token* copy_token(Token* original_token);
void free_node(Node* node);
void free_globals(Node* node);
void free_ast(Node* root);
#endif