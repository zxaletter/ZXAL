#ifndef PARSER_H
#define PARSER_H

#include "Lexer/lexer.h"
#include "Symbols/symbols.h"
#include "auxiliaries.h"
#include "errorsandcontext.h"

typedef struct {
	int size;
	int capacity;
	Token* end;

} Parser;

bool match_token(Parser* parser, char expected);
token_t peek_token_type(Parser* parser);
Token peek_token(Parser* parser);
token_t peek_next_token_type(Parser* parser);
Token advance_parser(Parser* parser);
bool at_token_eof(Parser* parser);

node_t get_op_kind(Token* token);
data_t get_type(Token* token);

char* get_token_string(token_t type);
void repor_error(Token* tok, FileInfo* info, error_t error);
void init_error_list();
ErrorList* create_error_list();
bool add_error(Error* err);
void create_error(error_t type, char* message, Token* token, FileInfo* info);
void display_error(Error* error);
void emit_errors();
void free_error(Error* error);
void free_error_list();

Node* create_node(node_t type, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* create_string_node(node_t type, char* id, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* create_char_node(node_t type, char ch, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* create_int_node(node_t type, int val, Node* left, Node* right,
	Node* prev, Node* next, struct type* t);

Node* parse_factor(Parser* parser, FileInfo* info);
Node* parse_unary(Parser* parser, FileInfo* info);
Node* parse_multiplicative(Parser* parser, FileInfo* info);
Node* parse_additive(Parser* parser, FileInfo* info);
Node* parse_relational(Parser* parser, FileInfo* info);
Node* parse_logical_and(Parser* parser, FileInfo* info);
Node* parse_logical_or(Parser* parser, FileInfo* info);

Node* parse_statement(Parser* parser, FileInfo* info);
Node* parse_block(Parser* parser, FileInfo* info);
Node* parse_parameters(Parser* parser, FileInfo* info);
Node* parse_function(Parser* parser, FileInfo* info);
Node* parse_enum_body(Parser* parser, FileInfo* info);
Node* parse_enum(Parser* parser, FileInfo* info);
Node* parse_struct(Parser* parser, FileInfo* info);
Node* parse_switch(Parser* parser, FileInfo* info);
Node* parse_args(Parser* parser, FileInfo* info);
Node* parse_array_list(Parser* parser, FileInfo* info, int* element_count);
Node* parse_let(Parser* parser, FileInfo* info);
Parser* initialize_parser(Token* tokens);
Node* parse(Token* tokens, FileInfo* info);

char* get_type_name(data_t type);
void print_expression_recursive(Node* expr, char* child_prefix, char* expr_connector, bool* is_last_stmt, int* level);
void print_statement_recursive(Node* stmt, char* child_prefix, char* stmt_connector, bool* is_last_stmt, int* level);
void print_ast(Node* root);

struct type* create_type(data_t main_type, struct type* subtype, Node* params);
void synchronize(Parser* parser, token_t* synchronizations);
Token* copy_token(Token* original_token);
void free_expression(Node* node);
void free_parameters(Node* params);
void free_statement(Node* head);
void free_globals(Node* node);
void free_ast(Node* root);
#endif