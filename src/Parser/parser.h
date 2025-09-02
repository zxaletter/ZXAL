#ifndef PARSER_H
#define PARSER_H

#include "Lexer/lexer.h"
#include "Lexer/token.h"
#include "errors.h"
#include "types.h"
#include "node.h"

typedef struct CompilerContext CompilerContext;

#define INIT_SPECIAL_STATEMENTS_CAPACITY 100

typedef struct {
	Token* tokens;
	FileInfo* info;
} Parser;

bool validate_token(CompilerContext* ctx, Parser* parser, token_t target_type);
bool have_valid_statement(CompilerContext* ctx, Parser* parser, Node* stmt);

bool valid_function_return_type(token_t type);
bool match_token(Parser* parser, char expected);
token_t peek_token_type(Parser* parser);
Token peek_token(Parser* parser);
token_t peek_next_token_type(Parser* parser);
Token advance_parser(Parser* parser);
bool at_token_eof(Parser* parser);

node_t get_op_kind(Token* token);
TypeKind get_type(Token* token);
char* get_token_string(token_t type);

Node* create_node(CompilerContext* ctx, node_t type, Node* left, Node* right,
	Node* prev, Node* next, Node* params, struct Type* t);

Node* create_string_node(CompilerContext* ctx, node_t type, char* id, Node* left, Node* right,
	Node* prev, Node* next, Node* params, struct Type* t);

Node* create_char_node(CompilerContext* ctx, node_t type, char ch, Node* left, Node* right,
	Node* prev, Node* next, Node* params, struct Type* t);

Node* create_int_node(CompilerContext* ctx, node_t type, int val, Node* left, Node* right,
	Node* prev, Node* next, Node* params, struct Type* t);

Node* parse_factor(CompilerContext* ctx, Parser* parser);
Node* parse_unary(CompilerContext* ctx, Parser* parser);
Node* parse_multiplicative(CompilerContext* ctx, Parser* parser);
Node* parse_additive(CompilerContext* ctx, Parser* parser);
Node* parse_relational(CompilerContext* ctx, Parser* parser);
Node* parse_logical_and(CompilerContext* ctx, Parser* parser);
Node* parse_logical_or(CompilerContext* ctx, Parser* parser);

Node* parse_statement(CompilerContext* ctx,Parser* parser);
Node* parse_block(CompilerContext* ctx,Parser* parser);
Node* parse_parameters(CompilerContext* ctx, Parser* parser);
Node* parse_function(CompilerContext* ctx,Parser* parser);
Node* parse_enum_body(CompilerContext* ctx, Parser* parser);
Node* parse_enum(CompilerContext* ctx, Parser* parser);
Node* parse_struct(CompilerContext* ctx, Parser* parser);
Node* parse_switch(CompilerContext* ctx, Parser* parser);
Node* parse_args(CompilerContext* ctx, Parser* parser);
Node* parse_array_list(CompilerContext* ctx, Parser* parser, int* element_count);
Node* parse_let(CompilerContext* ctx, Parser* parser);
Node* parse(CompilerContext* ctx, Lexer* lexer);

void synchronize(Parser* parser, token_t* synchronizations, size_t length);
Token* copy_token(CompilerContext* ctx, Token* original_token);
#endif