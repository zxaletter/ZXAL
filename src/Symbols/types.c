// #include "types.h"

// bool type_equals(struct type* a, struct type* b) {
// 	if (!a || !b) return false;

// 	if (a->kind == b->kind) {
// 		switch (a->kind) {
// 			case TYPE_BOOL:
// 			case TYPE_CHAR:
// 			case TYPE_INTEGER: { return true; } 
		

// 			case TYPE_ARRAY: {
// 				return type_equals(a->subtype, b->subtype);
// 			}
// 		}
// 	}
// 	return false;
// }

// void typecheck_expression(Node* expr) {
// 	if (!expr) return;
// }

// void typecheck_statement(Node* stmt) {
// 	if (!stmt) return;

// 	switch (stmt->type) {
// 		case  
// 	}
// }

// void typecheck_params(Node* params) {
// 	if (!params) return;
// }

// void typecheck_globals(Node* globals) {
// 	if (!globals) return;

// 	switch (globals->type) {
// 		case NODE_ASSIGNMENT: {
// 			if (node->left) {
// 				typecheck_expression(node->left);
// 			}

// 			if (node->right) {
// 				typecheck_expression(node->right);
// 			}

// 			break;
// 		}

// 		case NODE_NAME: {
// 			if (node->t && node->t->kind == TYPE_FUNCTION) {
// 				if (node->left) {
// 					Node* param = node->left;
// 					while (param) {
// 						Node* next = param->next;
// 						typecheck_param(param);
// 						param = next;
// 					}
// 				}

// 				if (node->right) {
// 					Node* stmt = node->right;
// 					while (stmt) {
// 						Node* next = stmt->next;
// 						typecheck_statement(stmt);
// 						stmt = next;
// 					}
// 				}

// 			} else {
// 				typecheck_expression(node);
// 			}
// 			break;
// 		}
// 	}
// }

// void typecheck_tree(Node* root) {
// 	if (!root) return;

// 	Node* node = root;
// 	while (node) {
// 		Node* next = node->next;
// 		typecheck_globals(node);
// 		node = next;
// 	}
// }