#ifndef NODE_H
#define NODE_H

typedef enum {
	NODE_INTEGER,
	NODE_CHAR,
	NODE_BOOL,
	NODE_IF,
	NODE_ELSE_IF,
	NODE_ELSE,
	NODE_FOR, // 6
	NODE_WHILE, // 7
	NODE_BREAK,
	NODE_CONTINUE,
	NODE_STRUCT_DEF, // todo
	NODE_ENUM, // todo
	NODE_RETURN,
	NODE_CALL,
	NODE_SUBSCRIPT, // 14
	NODE_PARAM, // 15
	NODE_ARG, 
	NODE_ASSIGNMENT, // 17
	NODE_ADDR, // todo when we have pointer implementation

	NODE_BLOCK, // 19
	NODE_NAME, //20
	NODE_DECL,
	NODE_DEF,
	NODE_AUG,

	NODE_ADD, // 24
	NODE_SUB, // 25
	NODE_DIV,
	NODE_MUL,
	NODE_ADD_EQUAL,
	NODE_SUB_EQUAL,
	NODE_DIV_EQUAL, // 30
	NODE_MUL_EQUAL,
	NODE_MODULO,

	NODE_LESS,
	NODE_GREATER,
	NODE_LESS_EQUAL, // 35
	NODE_GREATER_EQUAL, // 36
	NODE_NOT,
	NODE_EQUAL,
	NODE_NOT_EQUAL,
	NODE_INCREMENT, // 40
	NODE_DECREMENT,
	NODE_LOGICAL_AND,
	NODE_LOGICAL_OR,
	NODE_ARRAY_LIST,

	NODE_UNARY_ADD,
	NODE_UNARY_SUB,
	NODE_UNKNOWN
} node_t;

typedef union {
	int val;
	char c;
	char* name;
} NodeValue;

typedef struct Node {
	node_t type;
	NodeValue value;

	struct Node* left;
	struct Node* right;
	struct Node* prev;
	struct Node* next;
	struct Node* params;
	
	void* t;
	void* symbol;
	int reg;
} Node;

#endif