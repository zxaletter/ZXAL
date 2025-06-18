// #ifndef TYPES_H
// #define TYPES_H

// #include "Parser/parser.h"
// #include "symbols.h"
// #include "auxiliaries.h"

// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>

// typedef enum data_t {
// 	TYPE_INTEGER,
// 	TYPE_CHAR,
// 	TYPE_BOOL,
// 	TYPE_VOID,
// 	TYPE_ARRAY,
// 	TYPE_POINTER,
// 	TYPE_FUNCTION,
// 	TYPE_STRUCT,
// 	TYPE_ENUM,
// 	TYPE_UNKNOWN
// } data_t;

// struct type {
// 	data_t kind;
// 	bool type_free;	
// 	struct type* subtype;
// 	Node* params;
// };

// Node* params_copy(Node* params);
// Node* create_param(Node* wrapped_param);
// struct type* create_type(data_t main_type, struct type* subtype, Node* params);
// struct type* type_copy(struct type* t);
// bool type_equals(struct type* a, struct type* b);
// void typecheck_params(Node* params);
// struct type* typecheck_expression(Node* expr);
// void typecheck_statement(Node* stmt);
// void typecheck_globals(Node* globals);
// void typecheck_tree(Node* root);	

// #endif