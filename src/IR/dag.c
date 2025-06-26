#include "dag.h"

DAGNodeTable* dag_node_table = NULL;
static int dag_id = 0;

#define MIN(a, b) ((a)<(b) ? (a) : (b))
#define MAX(a, b) ((a)>(b) ? (a) : (b))

void init_dag_node_table() {
	dag_node_table = create_dag_node_table();
	if (!dag_node_table) {
		printf("In 'init_dag_node_table, unable to create dag_node_table.\n");
		exit(EXIT_FAILURE);
	}
}

DAGNodeTable* create_dag_node_table() {
	DAGNodeTable* table = malloc(sizeof(DAGNodeTable));
	if (!table) {
		perror("In 'create_dag_node_table', unable to allocate space for table.\n");
		return NULL;
	}

	table->size = 0;
	table->capacity = DAG_NODES;
	table->entries = calloc(table->capacity, sizeof(DAGNodeEntry*));
	if (!table->entries) {
		perror("In 'create_dag_node_table', unable to allocate space and initialize dag_node_table.\n");
		free(table);
		return NULL;
	}
	return table;
}

DAGNodeEntry* create_dag_node_entry(char* key, DAGNode* node, DAGNodeEntry* link) {
	DAGNodeEntry* entry = malloc(sizeof(DAGNodeEntry));
	if (!entry) {
		perror("In 'create_dag_node_entry', unable to allocate space for entry.\n");
		return NULL;
	}

	entry->node = node;
	entry->link = link;
	entry->freed = false;
	entry->key = strdup(key);
	if (!entry->key) {
		printf("In 'create_dag_node_entry', unable to duplicate '%s'\n", key ? key : "N/A");
		free(entry);
		return NULL;
	}
	return entry;
}

unsigned int hash_string(char* key) {
	if (!key) return 0;

	unsigned int entry_index = 0;
	while (*key) {
		entry_index = (entry_index << 1) + *key++;
	}
	return entry_index % dag_node_table->capacity;
}

DAGNode* hash_get_dagnode(unsigned int entry_index, char* key) {
	DAGNodeEntry* entry = dag_node_table->entries[entry_index];
	while (entry) {
		if (strcmp(entry->key, key) == 0) { return entry->node; }
		entry = entry->link;
	}
	return NULL;
}

bool hash_bind_dagnode(DAGNode* node, unsigned int entry_index, char* key) {
	if (!node || !key) return false;

	if (dag_node_table->size >= dag_node_table->capacity) {
		int prev_capacity = dag_node_table->capacity;
		DAGNodeEntry** prev_entries = malloc(sizeof(DAGNodeEntry*) * prev_capacity);
		if (!prev_entries) {
			perror("Unable to allocate space for previous entries.\n");
			return false;
		}

		for (int i = 0; i < prev_capacity; i++) {
			prev_entries[i] = dag_node_table->entries[i];
		}

		dag_node_table->capacity *= 2;
		dag_node_table->entries = realloc(dag_node_table->entries, sizeof(DAGNodeEntry*) * dag_node_table->capacity);
		if (!dag_node_table->entries) {
			perror("In 'hash_bind_dagnode', unable to reallocate space for table entries.\n");
			free(prev_entries);
			return false;
		}

		for (int i = 0; i < dag_node_table->capacity; i++) {
			dag_node_table->entries[i] = NULL;
		}

		for (int i = 0; i < prev_capacity; i++) {
			DAGNodeEntry* current_entry_in_old_chain = prev_entries[i];
			while (current_entry_in_old_chain) {
				DAGNodeEntry* next_entry_in_old_chain = current_entry_in_old_chain->link;
				unsigned int updated_index = hash_string(current_entry_in_old_chain->key);
				if (dag_node_table->entries[updated_index]) {
					current_entry_in_old_chain->link = dag_node_table->entries[updated_index];
					dag_node_table->entries[updated_index] = current_entry_in_old_chain;
				} else {
					dag_node_table->entries[updated_index] = current_entry_in_old_chain;
					current_entry_in_old_chain->link = NULL;
				}
				current_entry_in_old_chain = next_entry_in_old_chain;
			}
		}
		free(prev_entries);
		entry_index = hash_string(key);
	}

	if (dag_node_table->entries[entry_index]) {
		DAGNodeEntry* current_entry = dag_node_table->entries[entry_index];
		DAGNodeEntry* new_entry = create_dag_node_entry(key, node, current_entry);
		dag_node_table->entries[entry_index] = new_entry;
		dag_node_table->size++;
	} else {
		dag_node_table->entries[entry_index] = create_dag_node_entry(key, node, NULL);
		dag_node_table->size++;
	}
	return true;
}

char* generate_hash_key(DAGNode* dagnode) {
	char* buffer = malloc(256);
	if (!buffer) {
		perror("In 'generate_hash_key', unable to allocate space for key.\n");
		return NULL;
	}

	switch (dagnode->kind) {
		case DAG_MUL:
		case DAG_ADD: {
			if (dagnode->left->dag_id != dagnode->right->dag_id) {
				int min = MIN(dagnode->left->dag_id, dagnode->right->dag_id);
				int max = MAX(dagnode->left->dag_id, dagnode->right->dag_id);
				snprintf(buffer, 256, "%d:%d:%d", dagnode->kind, min, max);
			} else {
				snprintf(buffer, 256, "%d:%d:%d", dagnode->kind, dagnode->left->dag_id, dagnode->right->dag_id);
			}
			break;
		}	

		case DAG_INCREMENT:
		case DAG_DECREMENT: {
			snprintf(buffer, 256, "%d:%d", dagnode->kind, dagnode->left->dag_id);
			break;
		}

		case DAG_INTEGER:
		case DAG_CHAR:
		case DAG_BOOL: 
		case DAG_NAME: {
			snprintf(buffer, 256, "%d:%d", dagnode->kind, dagnode->dag_id);
			break;
		}

		case DAG_NOT: {
			snprintf(buffer, 256, "%d:%d", dagnode->kind, dagnode->right->dag_id);
			break;
		}

		case DAG_LOGICAL_OR:
		case DAG_LOGICAL_AND:
		case DAG_EQUAL:
		case DAG_NOT_EQUAL:
		case DAG_LESS:
		case DAG_GREATER:
		case DAG_LESS_EQUAL:
		case DAG_GREATER_EQUAL:
		case DAG_SUB:
		case DAG_DIV: {
			snprintf(buffer, 256, "%d:%d:%d", dagnode->kind, dagnode->left->dag_id, dagnode->right->dag_id);
			
			break;
		}

		case DAG_SUBSCRIPT: {
			snprintf(buffer, 256, "%d:%d:%d", dagnode->kind, dagnode->left->dag_id, dagnode->right->dag_id);
			break;
		}
	}

	return buffer;
}

DAGNode* get_or_bind_dagnode(DAGNode* dagnode) {
	if (!dagnode) return NULL;

	char* key = NULL;
	switch (dagnode->kind) {
		case DAG_DEF:
		case DAG_AUG:
		case DAG_DECL: {
			key = generate_hash_key(dagnode->left);
			break;
		}

		default: {
			key = generate_hash_key(dagnode);
			break;
		}
	}

	if (!key) return NULL;

	unsigned int index = hash_string(key);
	DAGNode* existing_dagnode = hash_get_dagnode(index, key);
	if (existing_dagnode) {
		DAGNode* temp = dagnode;
		dagnode = existing_dagnode;
		free(key);
		free_dag_node(temp);
		return dagnode;
	}

	if (!hash_bind_dagnode(dagnode, index, key)) {
		printf("In 'get_or_bind_dagnode', unable to bind dagnode.\n");
		free(key);
		return NULL;
	}
	free(key);
	return dagnode;
}

DAGNode* copy_dag_node(DAGNode* original_dagnode) {
	if (!original_dagnode) return NULL;

	DAGNode* duplicate_dagnode = malloc(sizeof(DAGNode));
	if (!duplicate_dagnode) {
		perror("In 'copy_dag_node', unable to allocate space for duplicate dag node.\n");
		return NULL;
	}

	duplicate_dagnode->kind = original_dagnode->kind;
	duplicate_dagnode->value = original_dagnode->value;
	duplicate_dagnode->dag_id = original_dagnode->dag_id;
	duplicate_dagnode->type = original_dagnode->type;
	duplicate_dagnode->freed = false;
	duplicate_dagnode->left = NULL;
	duplicate_dagnode->right = NULL;
	duplicate_dagnode->prev = NULL;
	duplicate_dagnode->next = NULL;

	if (original_dagnode->left) {
		duplicate_dagnode->left = copy_dag_node(original_dagnode->left);
		if (!duplicate_dagnode->left) {
			free(duplicate_dagnode);
			return NULL;
		}
	}

	if (original_dagnode->right) {
		duplicate_dagnode->right = copy_dag_node(original_dagnode->right);
		if (!duplicate_dagnode->right) {
			if (duplicate_dagnode->left) free_dag_node(duplicate_dagnode->left);
			free(duplicate_dagnode);
			return NULL;
		}
	}

	if (original_dagnode->prev) {
		duplicate_dagnode->prev = copy_dag_node(original_dagnode->prev);
		if (!duplicate_dagnode->prev) {
			if (duplicate_dagnode->right) free_dag_node(duplicate_dagnode->right);
			if (duplicate_dagnode->left) free_dag_node(duplicate_dagnode->left);
			free(duplicate_dagnode);
			return NULL;
		}
	}

	if (original_dagnode->next) {
		duplicate_dagnode->next = copy_dag_node(original_dagnode->next);
		if (!duplicate_dagnode->next) {
			if (duplicate_dagnode->prev) free_dag_node(duplicate_dagnode->prev);
			if (duplicate_dagnode->right) free_dag_node(duplicate_dagnode->right);
			if (duplicate_dagnode->left) free_dag_node(duplicate_dagnode->left);
			free(duplicate_dagnode);
			return NULL;
		}
	}
	return duplicate_dagnode;
}

DAGNode* create_dag_node(dagnode_t kind, DAGNode* left, DAGNode* right, DAGNode* prev, DAGNode* next, Symbol* symbol, struct type* type) {
	DAGNode* dag_node = malloc(sizeof(DAGNode));
	if (!dag_node) {
		perror("Unable to create dag node.\n");
		return NULL;
	}

	dag_node->kind = kind;
	dag_node->left = left;
	dag_node->right = right;
	dag_node->prev = prev;
	dag_node->next = next;
	dag_node->freed = false;
	dag_node->type = type;
	dag_node->symbol = symbol;
	dag_node->dag_id = assign_dag_id();

	return dag_node;
}

int assign_dag_id() {
	return dag_id++;
}

DAGNode* create_int_dag_node(dagnode_t kind, int val, DAGNode* left, DAGNode* right, 
	DAGNode* prev, DAGNode* next, Symbol* symbol, struct type* type) {

	DAGNode* dag_node = create_dag_node(kind, left, right, prev, next, symbol, type);
	if (!dag_node) {
		printf("In 'create_int_dag_node', received NULL dag node from 'create_dag_node'\n");
		return NULL;
	}
	dag_node->value.val = val;
	return dag_node;
}

DAGNode* create_string_dag_node(dagnode_t kind, char* name, DAGNode* left, DAGNode* right, 
	DAGNode* prev, DAGNode* next, Symbol* symbol,struct type* type) {
	
	DAGNode* dag_node = create_dag_node(kind, left, right, prev, next, symbol, type);
	if (!dag_node) {
		printf("In 'create_string_dag_node', received NULL dag node from 'create_dag_node'.\n");
		return NULL;
	}

	dag_node->value.name = NULL;
	if (name) {
		dag_node->value.name = strdup(name);
		if (!dag_node->value.name) {
			printf("In 'create_string_dag_node', unable to duplicate '%s' name for dag_node->name.\n");
			free(dag_node);
			return NULL;
		}
	}
	return dag_node;
}

DAGNode* build_expression_dag_node(Node* node) {
	if (!node) return;
	printf("In build_expression_dag_node.\n");
	DAGNode* left = NULL;
	DAGNode* right = NULL;
	DAGNode* result = NULL;

	switch (node->type) {
		case NODE_INTEGER: {
			result = create_int_dag_node(DAG_INTEGER, node->value.val, NULL, NULL, NULL, NULL, NULL, node->t);
			break;
		}

		case NODE_CHAR: {
			result = create_int_dag_node(DAG_CHAR, node->value.c, NULL, NULL, NULL, NULL, NULL, node->t);
			break;
		}

		case NODE_BOOL: {
			result = create_int_dag_node(DAG_BOOL, node->value.val, NULL, NULL, NULL, NULL, NULL, node->t);
			break;
		}

		case NODE_NAME: {
			result = create_string_dag_node(DAG_NAME, node->value.name, NULL, NULL, NULL, NULL, node->symbol, node->t);
			break;
		}

		case NODE_ARG: {
			right = build_expression_dag_node(node->right);
			break;
		}

		case NODE_CALL: {
			Node* arg = node->right;
			DAGNode* head = NULL;
			DAGNode* current = NULL;

			while (arg) {
				right = build_expression_dag_node(arg);
				if (right) {
					if (!head) {
						head = right;
						current = right;
					} else {
						current->next = right;
						right->prev = current;
						current = right;
					}
				}
				arg = arg->next;
			}
			break;
		}

		case NODE_DEF:
		case NODE_DECL:
		case NODE_AUG: {
			left = build_expression_dag_node(node->left);
			result = create_dag_node(DAG_AUG, left, right, NULL, NULL, NULL, NULL);
			break;
		}

		case NODE_SUBSCRIPT: {
			left = build_expression_dag_node(node->left);
			right = build_expression_dag_node(node->right);
			result = create_dag_node(DAG_SUBSCRIPT, left, right, NULL, NULL, node->symbol, NULL);
			break;
		}

		case NODE_MODULO:
		case NODE_ADD_EQUAL:
		case NODE_SUB_EQUAL:
		case NODE_MUL_EQUAL:
		case NODE_DIV_EQUAL:
		case NODE_LESS:
		case NODE_GREATER:
		case NODE_LESS_EQUAL:
		case NODE_GREATER_EQUAL:
		case NODE_EQUAL:
		case NODE_NOT_EQUAL:
		case NODE_LOGICAL_AND:
		case NODE_LOGICAL_OR:
		case NODE_DIV:
		case NODE_MUL:
		case NODE_SUB:
		case NODE_ADD: {
			left = build_expression_dag_node(node->left);
			right = build_expression_dag_node(node->right);
			result = create_dag_node(node->type, left, right, NULL, NULL, NULL, NULL);
			break;
		}

		case NODE_NOT: {
			right = build_expression_dag_node(node->right);
			result = create_dag_node(DAG_NOT, left, right, NULL, NULL, NULL, NULL);
			break;
		}

		case NODE_DECREMENT:
		case NODE_INCREMENT: {
			left = build_expression_dag_node(node->left);
			result = create_dag_node(node->type, left, right, NULL, NULL, NULL, NULL);
			break;
		}
	}

	DAGNode* existing_dagnode = get_or_bind_dagnode(result);
	if (!existing_dagnode) return NULL;	
	if (existing_dagnode == result) return result; 
	return existing_dagnode;
}

DAGNode* build_statement_dag_node(Node* node) {
	if (!node) return;
	DAGNode* left_dag_node = NULL;
	DAGNode* right_dag_node = NULL;
	DAGNode* result = NULL;

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			left_dag_node = build_expression_dag_node(node->left);
			right_dag_node = build_expression_dag_node(node->right);
			result = create_dag_node(DAG_ASSIGNMENT, left_dag_node, right_dag_node, NULL, NULL, NULL, NULL);
			break;
		}

		case NODE_CALL: {
			result = build_expression_dag_node(node);
			break;
		}

		case NODE_INCREMENT:
		case NODE_DECREMENT: {
			result = build_expression_dag_node(node->left);
			break;
		}

		case NODE_BREAK: {
			result = create_dag_node(DAG_BREAK, NULL, NULL, NULL, NULL, NULL, NULL);
			break;
		} 

		case NODE_CONTINUE: {
			result = create_dag_node(DAG_CONTINUE, NULL, NULL, NULL, NULL, NULL, NULL);
			break;
		}

		case NODE_RETURN: {
			if (!node->right) {
				result = create_dag_node(DAG_RETURN, NULL, NULL, NULL, NULL, NULL, NULL);
			} else {
				right_dag_node = build_expression_dag_node(node->right);
				result = create_dag_node(DAG_RETURN, NULL, right_dag_node, NULL, NULL, NULL, NULL); 
			}
			break;
		}

		case NODE_IF: {
			left_dag_node = build_expression_dag_node(node->left);
			right_dag_node = build_expression_dag_node(node->right);
			result = create_dag_node(DAG_IF, left_dag_node, right_dag_node, NULL, NULL, NULL, NULL);
			break;
		}
		case NODE_ELSE_IF: {
			left_dag_node = build_expression_dag_node(node->left);
			right_dag_node = build_expression_dag_node(node->right);
			result = create_dag_node(DAG_ELSE_IF, left_dag_node, right_dag_node, NULL, NULL, NULL, NULL);
			break;

		}

		case NODE_ELSE: {
			right_dag_node = build_expression_dag_node(node->right);
			result = create_dag_node(DAG_ELSE, NULL, right_dag_node, NULL, NULL, NULL, NULL);
			break;
		}
		case NODE_WHILE: {
			left_dag_node = build_expression_dag_node(node->left);
			right_dag_node = build_expression_dag_node(node->right);
			result = create_dag_node(DAG_WHILE, left_dag_node, right_dag_node, NULL, NULL, NULL, NULL);
			break;
		}

		case NODE_FOR: {
			left_dag_node = build_statement_dag_node(node->left);
			Node* condition = node->left ? node->left->next : NULL;
			DAGNode* condition_dagnode = NULL;
			if (condition) {
				condition_dagnode = build_expression_dag_node(condition);
			}
			Node* update = condition ? condition->next : NULL;
			DAGNode* update_dagnode = NULL;
			if (update) { update_dagnode = build_expression_dag_node(update); }
			if (condition_dagnode) { left_dag_node->next = condition_dagnode; }
			if (update_dagnode) { condition_dagnode->next = update_dagnode; }

			DAGNode* right_dag_node = build_statement_dag_node(node->right);
			DAGNode* result = create_dag_node(DAG_FOR, left_dag_node, right_dag_node, NULL, NULL, NULL, NULL);
			break;
		}

		case NODE_BLOCK: {
			Node* stmt = node->right;
			DAGNode* head = NULL;
			DAGNode* current = NULL;
			while (stmt) {
				Node* next_stmt = stmt->next;
				right_dag_node = build_statement_dag_node(stmt);
				if (right_dag_node) {
					if (!head) {
						head = right_dag_node;
						current = right_dag_node;
					} else {
						current->next = right_dag_node;
						right_dag_node->prev = current;
						current = right_dag_node;
					}
				}
				stmt = next_stmt;
			}
			result = create_dag_node(DAG_BLOCK, NULL, head, NULL, NULL, NULL, NULL);
			break;
		}
	}
	return result;
}

DAGNode* build_global_dag_node(Node* global_node) {
	if (!global_node) return NULL;
	printf("In build_global_dag_node. Current node type is %d\n", global_node->type);
	switch (global_node->type) {
		case NODE_NAME: {
			if (global_node->t) {
				if (global_node->t->kind == TYPE_FUNCTION) {
					printf("Now im here\n");
					DAGNode* body = build_statement_dag_node(global_node->right);
					DAGNode* function_node = create_string_dag_node(DAG_NAME, global_node->value.name, NULL, body, NULL, NULL, global_node->symbol, global_node->t);
					printf("About to return function dagnode.\n");
					return function_node;
				}
			}
		}
		default: printf("unknown node.\n"); break;

	}
}

DAGNode* build_DAG(Node* root) {
	if (!root) return;

	init_dag_node_table();
	printf("initialized dag table\n");
	DAGNode* head = NULL;
	DAGNode* current = NULL;

	Node* global_node = root;
	printf("In 'build_DAG', root node has type: %d\n", root->type);
	while (global_node) {
		Node* next_global_node = global_node->next;
		DAGNode* dag_node = build_global_dag_node(global_node);
		
		if (dag_node) {
			printf("We have global dag node.\n");
			if (!head) {
				head = dag_node;
				current = dag_node;
			} else {
				current->next = dag_node;
				dag_node->prev = current;
				current = dag_node;
			}
		} else {
			printf("Global dag node is NULL\n");
		}

		global_node = next_global_node;
	}
	return head;
}

void free_dagnode_table() {
	if (!dag_node_table) return;
	for (int i = 0; i < dag_node_table->capacity; i++) {
		free_dag_entry(dag_node_table->entries[i]);
	}
	free(dag_node_table->entries);
	free(dag_node_table);
}

void free_dag_node(DAGNode* node) {
	if (!node || (node && node->freed)) return;

	node->freed = true;
	switch (node->kind) {
		case DAG_SUBSCRIPT:
		case DAG_LESS:
		case DAG_GREATER:
		case DAG_LESS_EQUAL:
		case DAG_GREATER_EQUAL:
		case DAG_EQUAL:
		case DAG_NOT_EQUAL:
		case DAG_LOGICAL_OR:
		case DAG_LOGICAL_AND:
		case DAG_ADD_EQUAL:
		case DAG_SUB_EQUAL:
		case DAG_MUL_EQUAL:
		case DAG_DIV_EQUAL:
		case DAG_MODULO:
		case DAG_ADD:
		case DAG_SUB:
		case DAG_MUL:
		case DAG_DIV:
		case DAG_ASSIGNMENT: {
			free_dag_node(node->left);
			free_dag_node(node->right);
			break;
		}

		case DAG_NAME: {
			if (node->value.name) {
				free(node->value.name); 
				node->value.name = NULL;
			}

			if (node->left) free_dag_node(node->left);
			if (node->right) free_dag_node(node->right);
			if (node->name) {
				free(node->name);
				node->name = NULL;
			}
			if (node->symbol) { free_symbol(node->symbol); }
			break;		
		}

		case DAG_DEF:
		case DAG_AUG:
		case DAG_DECL: {
			free_dag_node(node->left);
			if (node->name) free(node->name);
			break;
		}

		case DAG_INCREMENT:
		case DAG_DECREMENT: {
			free_dag_node(node->left);
			break;
		}

		case DAG_RETURN: {
			if (!node->right) break; 
			free_dag_node(node->right);
			break;
		}

		case DAG_NOT:
		case DAG_PARAM:	
		case DAG_ARG: {
			free_dag_node(node->right);
			if (node->symbol) { free_symbol(node->symbol); }
			break;
		}	

		case DAG_CALL: {
			if (node->left) free_dag_node(node->left);
			if (node->right) {
				DAGNode* arg = node->right;
				while (arg) {
					free_dag_node(arg);
					arg = arg->next;
				}
			}
			if (node->symbol) { free_symbol(node->symbol); }
			break;
		}

		case DAG_BLOCK: {
			DAGNode* stmt = node->right;
			while (stmt) {
				free_dag_node(stmt);
				stmt = stmt->next;
			}
			break;
		}

		case DAG_INTEGER:
		case DAG_BOOL:
		case DAG_CHAR: {
			if (node->name) free(node->name);
			break;
		}
		case DAG_CONTINUE:
		case DAG_BREAK: {
			break;
		}
	}
	free(node);
}

void free_dag_entry(DAGNodeEntry* entry) {
	if (!entry || (entry && entry->freed)) return;


	if (entry->key) { free(entry->key); }
	free_dag_node(entry->node);
	if (entry->link) {
		DAGNodeEntry* current_link = entry->link;
		while (current_link) {
			DAGNode* next_link = current_link->link;
			free_dag_entry(current_link);
			current_link = next_link; 
		}
	}
	entry->freed = true;
	free(entry);
}

void free_dag(DAGNode* root) {
	if (!root) return;

	free_dagnode_table();

	DAGNode* node = root;
	while (node) {
		DAGNode* next = node->next;
		free_dag_node(node);
		node = next;
	}

}