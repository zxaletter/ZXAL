#include "dag.h"

DAGNodeTable* dag_node_table = NULL;
static int dag_id = 0;

#define MIN(a, b) ((a)<(b) ? (a) : (b))
#define MAX(a, b) ((a)>(b) ? (a) : (b))

void init_dag_node_table(CompilerContext* ctx) {
	dag_node_table = create_dagnode_table(ctx);
	if (!dag_node_table) {
		printf("In 'init_dag_node_table, unable to create dag_node_table.\n");
		exit(EXIT_FAILURE);
	}
}

DAGNodeTable* create_dagnode_table(CompilerContext* ctx) {
	// DAGNodeTable* table = malloc(sizeof(DAGNodeTable));
	DAGNodeTable* table = arena_allocate(ctx->ir_arena, sizeof(DAGNodeTable));
	if (!table) {
		perror("In 'create_dagnode_table', unable to allocate space for table.\n");
		return NULL;
	}

	table->size = 0;
	table->capacity = DAG_NODES;
	// table->entries = calloc(table->capacity, sizeof(DAGNodeEntry*));
	table->entries = arena_allocate(ctx->ir_arena, sizeof(DAGNodeEntry*) * table->capacity);
	if (!table->entries) {
		perror("In 'create_dagnode_table', unable to allocate space and initialize dag_node_table.\n");
		// free(table);
		return NULL;
	}
	return table;
}

DAGNodeEntry* create_dagnode_entry(CompilerContext* ctx, char* key, DAGNode* node, DAGNodeEntry* link) {
	// DAGNodeEntry* entry = malloc(sizeof(DAGNodeEntry));
	DAGNodeEntry* entry = arena_allocate(ctx->ir_arena, sizeof(DAGNodeEntry));
	if (!entry) {
		perror("In 'create_dagnode_entry', unable to allocate space for entry.\n");
		return NULL;
	}

	entry->node = node;
	entry->link = link;
	entry->freed = false;
	// entry->key = strdup(key);
	entry->key = arena_allocate(ctx->ir_arena, strlen(key) + 1);
	if (!entry->key) {
		printf("In 'create_dagnode_entry', unable to duplicate '%s'\n", key ? key : "N/A");
		// free(entry);
		return NULL;
	}
	strcpy(entry->key, key);
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

bool hash_bind_dagnode(CompilerContext* ctx, DAGNode* node, unsigned int entry_index, char* key) {
	if (!node || !key) return false;

	if (dag_node_table->size >= dag_node_table->capacity) {
		int prev_capacity = dag_node_table->capacity;
		// DAGNodeEntry** prev_entries = malloc(sizeof(DAGNodeEntry*) * prev_capacity);
		DAGNodeEntry** prev_entries = arena_allocate(ctx->ir_arena, sizeof(DAGNodeEntry*) * prev_capacity);
		if (!prev_entries) {
			perror("Unable to allocate space for previous entries.\n");
			return false;
		}

		for (int i = 0; i < prev_capacity; i++) {
			prev_entries[i] = dag_node_table->entries[i];
		}

		dag_node_table->capacity *= 2;
		// dag_node_table->entries = realloc(dag_node_table->entries, sizeof(DAGNodeEntry*) * dag_node_table->capacity);
		void* new_entries = arena_reallocate(ctx->ir_arena, dag_node_table->entries, prev_capacity, dag_node_table->capacity);
		if (!new_entries) {
			perror("In 'hash_bind_dagnode', unable to reallocate space for table entries.\n");
			return false;
		}
		dag_node_table->entries = new_entries;

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
		// free(prev_entries);
		entry_index = hash_string(key);
	}

	if (dag_node_table->entries[entry_index]) {
		DAGNodeEntry* current_entry = dag_node_table->entries[entry_index];
		DAGNodeEntry* new_entry = create_dagnode_entry(ctx, key, node, current_entry);
		new_entry->link = current_entry;
		dag_node_table->entries[entry_index] = new_entry;
		dag_node_table->size++;
	} else {
		dag_node_table->entries[entry_index] = create_dagnode_entry(ctx, key, node, NULL);
		dag_node_table->size++;
	}
	return true;
}

char* generate_hash_key(CompilerContext* ctx, DAGNode* dagnode) {
	if (!dagnode) {
		printf("In 'generate_hash_key', received NULL dagnode\n");
		return NULL;
	}
	// char* buffer = malloc(256);
	char* buffer = arena_allocate(ctx->ir_arena, sizeof(char) * 256);
	if (!buffer) {
		perror("In 'generate_hash_key', unable to allocate space for key.\n");
		return NULL;
	}

	switch (dagnode->kind) {
		case DAG_ADD_EQUAL:
		case DAG_MUL_EQUAL:
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
			if (!dagnode->left) {
				printf("Dagnode with type: %d has NULL left child\n", dagnode->kind);
				return NULL;
			}
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
			if (!dagnode->right) {
				printf("Dagnode with type: %d has NULL right child\n", dagnode->kind);
				return NULL;
			}
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
		case DAG_DIV:
		case DAG_DIV_EQUAL:
		case DAG_SUB_EQUAL:
		case DAG_SUBSCRIPT: {				
			if (!dagnode->left) {
				printf("Dagnode with type: %d has NULL left child.\n", dagnode->kind);
				return NULL;
			}
			if (!dagnode->right) {
				printf("Dagnode with type: %d has NULL right child.\n", dagnode->kind);
				return NULL;
			}

			snprintf(buffer, 256, "%d:%d:%d", dagnode->kind, dagnode->left->dag_id, dagnode->right->dag_id);
			break;
		}

		case DAG_CALL: {
			if (dagnode->right) {
				snprintf(buffer, 256, "%d:%s:%d", dagnode->kind, dagnode->value.name, dagnode->right->dag_id);
			} else {
				snprintf(buffer, 256, "%d:%s:null", dagnode->kind, dagnode->value.name);
			}
			break;
		}

		case DAG_ARRAY_LIST: {
			if (dagnode->right) {
				snprintf(buffer, 256, "%d:%d", dagnode->kind, dagnode->right->dag_id);
			} else {
				snprintf(buffer, 256, "%d", dagnode->kind);
			}
			break;
		}
		default: return NULL; 

	}

	return buffer;
}

int assign_dag_id() {
	return dag_id++;
}

DAGNode* get_or_bind_dagnode(CompilerContext* ctx, DAGNode* dagnode) {
	if (!dagnode) return NULL;

	char* key = NULL;
	switch (dagnode->kind) {
		case DAG_DEF:
		case DAG_AUG:
		case DAG_DECL: {
			key = generate_hash_key(ctx, dagnode->left);
			break;
		}

		default: {
			printf("In 'get_or_bind_dagnode', obtaining key for dagnode of type %d\n", dagnode->kind);
			key = generate_hash_key(ctx, dagnode);
			break;
		}
	}

	if (!key) return dagnode;

	unsigned int index = hash_string(key);
	DAGNode* existing_dagnode = hash_get_dagnode(index, key);
	if (existing_dagnode) {
		printf("\033[31mRetrieved existing dagnode\033[0m\n");
		return existing_dagnode;
	}

	if (!hash_bind_dagnode(ctx, dagnode, index, key)) {
		printf("In 'get_or_bind_dagnode', unable to bind dagnode.\n");
		// free(key);
		return NULL;
	}
	// free(key);
	return dagnode;
}

DAGNode* copy_dagnode(CompilerContext* ctx, DAGNode* original_dagnode) {
	if (!original_dagnode) return NULL;

	// DAGNode* duplicate_dagnode = malloc(sizeof(DAGNode));
	DAGNode* duplicate_dagnode = arena_allocate(ctx->ir_arena, sizeof(DAGNode));
	if (!duplicate_dagnode) {
		perror("In 'copy_dagnode', unable to allocate space for duplicate dag node.\n");
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
		duplicate_dagnode->left = copy_dagnode(ctx, original_dagnode->left);
		if (!duplicate_dagnode->left) {
			// free(duplicate_dagnode);
			return NULL;
		}
	}

	if (original_dagnode->right) {
		duplicate_dagnode->right = copy_dagnode(ctx, original_dagnode->right);
		if (!duplicate_dagnode->right) {
			// if (duplicate_dagnode->left) free_dag_node(duplicate_dagnode->left);
			// free(duplicate_dagnode);
			return NULL;
		}
	}

	if (original_dagnode->prev) {
		duplicate_dagnode->prev = copy_dagnode(ctx, original_dagnode->prev);
		if (!duplicate_dagnode->prev) {
			// if (duplicate_dagnode->right) free_dag_node(duplicate_dagnode->right);
			// if (duplicate_dagnode->left) free_dag_node(duplicate_dagnode->left);
			// free(duplicate_dagnode);
			return NULL;
		}
	}

	if (original_dagnode->next) {
		duplicate_dagnode->next = copy_dagnode(ctx, original_dagnode->next);
		if (!duplicate_dagnode->next) {
			// if (duplicate_dagnode->prev) free_dag_node(duplicate_dagnode->prev);
			// if (duplicate_dagnode->right) free_dag_node(duplicate_dagnode->right);
			// if (duplicate_dagnode->left) free_dag_node(duplicate_dagnode->left);
			// free(duplicate_dagnode);
			return NULL;
		}
	}

	if (original_dagnode->params) {
		DAGNode* current_original_wrapped_param = original_dagnode->params;
		DAGNode* head = NULL;
		DAGNode* current = NULL;
		while (current_original_wrapped_param) {
			DAGNode* next_original_wrapped_param = current_original_wrapped_param->next;
			DAGNode* duplicate_inner_param = copy_dagnode(ctx, current_original_wrapped_param->right);
			if (!duplicate_inner_param) return NULL;
			DAGNode* duplicate_wrapped_param = create_dagnode(ctx, DAG_PARAM, NULL, duplicate_inner_param, NULL, NULL, NULL, NULL, NULL);
			if (duplicate_wrapped_param) {
				if (!head) {
					head = duplicate_wrapped_param;
					current = duplicate_wrapped_param;
				} else {
					current->next = duplicate_wrapped_param;
					duplicate_wrapped_param->prev = current;
					current = duplicate_wrapped_param;
				}
			} else {
				return NULL;
			}
			current_original_wrapped_param = next_original_wrapped_param;
		} 
		if (current) {
			current->next = NULL;
		}

		duplicate_dagnode->params = head;
	}
	return duplicate_dagnode;
}

DAGNode* create_dagnode(CompilerContext* ctx, dagnode_t kind, DAGNode* left, DAGNode* right, 
	DAGNode* prev, DAGNode* next, DAGNode* params, Symbol* symbol, struct type* type) {
	// DAGNode* dag_node = malloc(sizeof(DAGNode));
	DAGNode* dag_node = arena_allocate(ctx->ir_arena, sizeof(DAGNode));
	if (!dag_node) {
		perror("Unable to create dag node.\n");
		return NULL;
	}

	dag_node->kind = kind;
	dag_node->left = left;
	dag_node->right = right;
	dag_node->prev = prev;
	dag_node->next = next;
	dag_node->params = params;
	dag_node->freed = false;
	dag_node->type = type;
	dag_node->symbol = symbol;
	dag_node->dag_id = assign_dag_id();

	return dag_node;
}

DAGNode* create_int_dagnode(CompilerContext* ctx, dagnode_t kind, int val, DAGNode* left, DAGNode* right, 
	DAGNode* prev, DAGNode* next, DAGNode* params, Symbol* symbol, struct type* type) {

	DAGNode* dag_node = create_dagnode(ctx, kind, left, right, prev, next, params, symbol, type);
	if (!dag_node) {
		printf("In 'create_int_dagnode', received NULL dag node from 'create_dagnode'\n");
		return NULL;
	}
	dag_node->value.val = val;
	return dag_node;
}

DAGNode* create_string_dagnode(CompilerContext* ctx, dagnode_t kind, char* name, DAGNode* left, DAGNode* right, 
	DAGNode* prev, DAGNode* next, DAGNode* params, Symbol* symbol, struct type* type) {
	
	DAGNode* dag_node = create_dagnode(ctx, kind, left, right, prev, next, params, symbol, type);
	if (!dag_node) {
		printf("In 'create_string_dagnode', received NULL dag node from 'create_dagnode'.\n");
		return NULL;
	}

	dag_node->value.name = NULL;
	if (name) {
		// dag_node->value.name = strdup(name);
		dag_node->value.name = arena_allocate(ctx->ir_arena, strlen(name) + 1);
		if (!dag_node->value.name) {
			printf("In 'create_string_dagnode', unable to duplicate '%s' name for dag_node->name.\n");
			// free(dag_node);
			return NULL;
		}
		strcpy(dag_node->value.name, name);
	}
	return dag_node;
}

dagnode_t get_dagnode_type(node_t type) {
	switch (type) {
		case NODE_ADD: return DAG_ADD;
		case NODE_SUB: return DAG_SUB;
		case NODE_MUL: return DAG_MUL;
		case NODE_DIV: return DAG_DIV;
		case NODE_ADD_EQUAL: return DAG_ADD_EQUAL;
		case NODE_SUB_EQUAL: return DAG_SUB_EQUAL;
		case NODE_MUL_EQUAL: return DAG_MUL_EQUAL;
		case NODE_DIV_EQUAL: return DAG_DIV_EQUAL;
		case NODE_MODULO: return DAG_MODULO;
		case NODE_LESS: return DAG_LESS;
		case NODE_GREATER: return DAG_GREATER;
		case NODE_LESS_EQUAL: return DAG_LESS_EQUAL;
		case NODE_GREATER_EQUAL: return DAG_GREATER_EQUAL;
		case NODE_EQUAL: return DAG_EQUAL;
		case NODE_NOT_EQUAL: return DAG_NOT_EQUAL;
		case NODE_LOGICAL_OR: return DAG_LOGICAL_OR;
		case NODE_LOGICAL_AND: return DAG_LOGICAL_AND;
		case NODE_INCREMENT: return DAG_INCREMENT;
		case NODE_DECREMENT: return DAG_DECREMENT;
		case NODE_AUG: return DAG_AUG;
		case NODE_DEF: return DAG_DEF;
		case NODE_DECL: return DAG_DECL;
	}
}

DAGNode* build_expression_dagnode(CompilerContext* ctx, Node* node) {
	if (!node) return;

	DAGNode* left = NULL;
	DAGNode* right = NULL;
	DAGNode* result = NULL;

	switch (node->type) {
		case NODE_INTEGER: {
			result = create_int_dagnode(ctx, DAG_INTEGER, node->value.val, NULL, NULL, NULL, NULL, NULL, NULL, node->t);
			break;
		}

		case NODE_CHAR: {
			result = create_int_dagnode(ctx, DAG_CHAR, node->value.c, NULL, NULL, NULL, NULL, NULL, NULL, node->t);
			break;
		}

		case NODE_BOOL: {
			result = create_int_dagnode(ctx, DAG_BOOL, node->value.val, NULL, NULL, NULL, NULL, NULL, NULL, node->t);
			break;
		}

		case NODE_NAME: {
			if (node->left) left = build_expression_dagnode(ctx, node->left);
			if (node->right) right = build_expression_dagnode(ctx, node->right);
			result = create_string_dagnode(ctx, DAG_NAME, node->value.name, left, right, NULL, NULL, NULL, node->symbol, node->t);
			if (result) {
				printf("\033[31mResulting dag node has namse '%s'\033[0m\n", result->value.name);
			}
			break;
		}

		case NODE_ARG: {
			right = build_expression_dagnode(ctx, node->right);
			result = create_dagnode(ctx, DAG_ARG, NULL, right, NULL, NULL, NULL, NULL, NULL);
			break;
		}

		case NODE_CALL: {
			Node* arg = node->right;
			DAGNode* head = NULL;
			DAGNode* current = NULL;

			while (arg) {
				DAGNode* dagnode_arg = build_expression_dagnode(ctx, arg);
				if (dagnode_arg) {
					printf("\033[31mIn NODE_CALL case, dagnode_arg has type %d\033[0m\n", dagnode_arg->kind);
					if (!head) {
						head = dagnode_arg;
						current = dagnode_arg;
					} else {
						current->next = dagnode_arg;
						dagnode_arg->prev = current;
						current = dagnode_arg;
					}
				} else {
					return NULL;
				}
				arg = arg->next;
			}

			if (current) {
				current->next = NULL;
			}

			DAGNode* identifier_dagnode = build_expression_dagnode(ctx, node->left);
			result = create_dagnode(ctx, DAG_CALL, identifier_dagnode, head, NULL, NULL, NULL, node->symbol, node->t);
			break;
		}

		case NODE_DEF:
		case NODE_DECL:
		case NODE_AUG: {
			left = build_expression_dagnode(ctx, node->left);
			if (!left) return NULL;
			dagnode_t result_t = get_dagnode_type(node->type);
			result = create_dagnode(ctx, result_t, left, NULL, NULL, NULL, NULL, NULL, NULL);
			break;
		}

		case NODE_SUBSCRIPT: {
			left = build_expression_dagnode(ctx, node->left);
			right = build_expression_dagnode(ctx, node->right);
			if (!left || !right) {
				return NULL;
			}
			result = create_dagnode(ctx, DAG_SUBSCRIPT, left, right, NULL, NULL, NULL, node->symbol, node->t);
			break;
		}

		case NODE_ARRAY_LIST: {
			Node* elements = node->right;
			
			DAGNode* head = NULL;
			DAGNode* current = NULL;
			while (elements) {
				Node* next_element = elements->next;
				DAGNode* dagnode_element = build_expression_dagnode(ctx, elements);
				if (dagnode_element) {
					if (!head) {
						head = dagnode_element;
						current = dagnode_element;
					} else {
						current->next = dagnode_element;
						dagnode_element->prev = current;
						current = dagnode_element;
					}
				} else {
					return NULL;
				}
				elements = next_element;
			}

			if (current) {
				current->next = NULL;
			}
			
			result = create_dagnode(ctx, DAG_ARRAY_LIST, NULL, head, NULL, NULL, NULL, NULL, NULL);
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
			left = build_expression_dagnode(ctx, node->left);
			right = build_expression_dagnode(ctx, node->right);
		
			if (!left || !right) return NULL;
			dagnode_t result_t  = get_dagnode_type(node->type);
			result = create_dagnode(ctx, result_t, left, right, NULL, NULL, NULL, NULL, NULL);
			break;
		}

		case NODE_NOT: {
			right = build_expression_dagnode(ctx, node->right);
			if (!right) return NULL;
			result = create_dagnode(ctx, DAG_NOT, NULL, right, NULL, NULL, NULL, NULL, NULL);
			break;
		}

		case NODE_DECREMENT:
		case NODE_INCREMENT: {
			left = build_expression_dagnode(ctx, node->left);
			if (!left) return NULL;
			dagnode_t result_t = get_dagnode_type(node->type);
			result = create_dagnode(ctx, result_t, left, NULL, NULL, NULL, NULL, NULL, NULL);
			break;
		}
	}

	if (!result) return NULL;

	DAGNode* final_node = get_or_bind_dagnode(ctx, result);
	return final_node;
}

DAGNode* build_statement_dagnode(CompilerContext* ctx, Node* node) {
	if (!node) return;
	DAGNode* left = NULL;
	DAGNode* right = NULL;
	DAGNode* result = NULL;

	printf("In 'build_statement_dagnode'\n");

	switch (node->type) {
		case NODE_ASSIGNMENT: {
			printf("\033[32mIn 'build_statement_dagnode', processing NODE_ASSIGNMENT case\033[0m\n");
			left = build_expression_dagnode(ctx, node->left);
			right = build_expression_dagnode(ctx, node->right);
			if (!left || !right) return NULL;
			result = create_dagnode(ctx, DAG_ASSIGNMENT, left, right, NULL, NULL, NULL, NULL, NULL);
			printf("\033[32mIn 'build_statement_dagnode', leaving NODE_ASSIGNMENT case\033[0m\n");
			break;
		}

		case NODE_CALL: {
			printf("In 'build_statement_dagnode', processing NODE_CALL case\n");
			result = build_expression_dagnode(ctx, node);
			printf("In 'build_statement_dagnode', leaving NODE_CALL case\n");
			break;
		}

		case NODE_INCREMENT:
		case NODE_DECREMENT: {
			printf("In 'build_statement_dagnode', processing NODE INCREMENT/NODE DECREMENT case\n");
			left = build_expression_dagnode(ctx, node->left);
			if (!left) return NULL;
			dagnode_t result_t = get_dagnode_type(node->type);
			result = create_dagnode(ctx, result_t, left, NULL, NULL, NULL, NULL, NULL, NULL);
			printf("In 'build_statement_dagnode', leaving NODE INCREMENT/NODE DECREMENT case\n");
			break;
		}

		case NODE_BREAK: {
			printf("\033[32mIn 'build_statement_dagnode', processing NODE_BREAK case\033[0m\n");
			result = create_dagnode(ctx, DAG_BREAK, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			printf("\033[32mIn 'build_statement_dagnode', leaving NODE_BREAK case\033[0m\n");
			break;
		} 

		case NODE_CONTINUE: {
			printf("\033[32mIn 'build_statement_dagnode', processing NODE_CONTINUE case\033[0m\n");
			result = create_dagnode(ctx, DAG_CONTINUE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			printf("\033[32mIn 'build_statement_dagnode', leaving NODE_CONTINUE case\033[0m\n");
			break;
		}

		case NODE_RETURN: {
			printf("\033[32mIn 'build_statement_dagnode', processing NODE_RETURN case\033[0m\n");

			if (!node->right) {
				result = create_dagnode(ctx, DAG_RETURN, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			} else {
				right = build_expression_dagnode(ctx, node->right);
				if (!right) return NULL;
				result = create_dagnode(ctx, DAG_RETURN, NULL, right, NULL, NULL, NULL, NULL, NULL); 
				if (result) {
					printf("\033[31mCreated DAG_RETURN node\033[0m\n");
				} else {
					printf("\033[31mFailed to create DAG_RETURN node\033[0m\n");
					return NULL;
				}

			}
			printf("\033[32mIn 'build_statement_dagnode', leaving NODE_RETURN case\033[0m\n");
			break;
		}

		case NODE_IF: {
			printf("\033[32mIn 'build_statement_dagnode', processing NODE_IF case\033[0m\n");
			left = build_expression_dagnode(ctx, node->left);
			right = build_statement_dagnode(ctx, node->right);
			if (!left || !right) return NULL;
			result = create_dagnode(ctx, DAG_IF, left, right, NULL, NULL, NULL, NULL, NULL);
			printf("\033[32mIn 'build_statement_dagnode', leaving NODE_IF case\033[0m\n");
			break;
		}
		case NODE_ELSE_IF: {
			printf("\033[32mIn 'build_statement_dagnode', processing NODE_ELSE_IF case\033[0m\n");
			left = build_expression_dagnode(ctx, node->left);
			right = build_statement_dagnode(ctx, node->right);
			if (!left || !right) return NULL;
			result = create_dagnode(ctx, DAG_ELSE_IF, left, right, NULL, NULL, NULL, NULL, NULL);
			printf("\033[32mIn 'build_statement_dagnode', leaving NODE_ELSE_IF case\033[0m\n");
			break;
		}

		case NODE_ELSE: {
			printf("\033[32mIn 'build_statement_dagnode', processing NODE_ELSE case\033[0m\n");
			right = build_statement_dagnode(ctx, node->right);
			if (!right) return NULL;
			result = create_dagnode(ctx, DAG_ELSE, NULL, right, NULL, NULL, NULL, NULL, NULL);
			printf("\033[32mIn 'build_statement_dagnode', leaving NODE_ELSE case\033[0m\n");
			break;
		}
		case NODE_WHILE: {
			printf("\033[32mIn 'build_statement_dagnode', processing NODE_WHILE case\033[0m\n");
			left = build_expression_dagnode(ctx, node->left);
			right = build_statement_dagnode(ctx, node->right);
			if (!left || !right) return NULL;
			result = create_dagnode(ctx, DAG_WHILE, left, right, NULL, NULL, NULL, NULL, NULL);
			printf("\033[32mIn 'build_statement_dagnode', leaving NODE_WHILE case\033[0m\n");
			break;
		}

		case NODE_FOR: {
			printf("\033[32mIn 'build_statement_dagnode', processing NODE_FOR case\033[0m\n");
			// left = build_statement_dagnode(ctx, node->left);
			// Node* condition = node->left ? node->left->next : NULL;
			// DAGNode* condition_dagnode = NULL;
			// if (condition) {
			// 	condition_dagnode = build_expression_dagnode(ctx, condition);
			// }
			// Node* update = condition ? condition->next : NULL;
			// DAGNode* update_dagnode = NULL;
			// if (update) { update_dagnode = build_expression_dagnode(ctx, update); }
			// if (condition_dagnode) { left->next = condition_dagnode; }
			// if (update_dagnode) { condition_dagnode->next = update_dagnode; }

			// DAGNode* right = build_statement_dagnode(ctx, node->right);
			// DAGNode* result = create_dagnode(ctx, DAG_FOR, left, right, NULL, NULL, NULL, NULL);
			DAGNode* init_dagnode = build_statement_dagnode(ctx, node->left);

			Node* condition = node->left ? node->left->next : NULL;
			DAGNode* condition_dagnode = condition ? build_expression_dagnode(ctx, condition) : NULL;

			Node* update = condition ? condition->next : NULL;
			DAGNode* update_dagnode = update ? build_expression_dagnode(ctx, update) : NULL;

			DAGNode* body_dagnode = build_statement_dagnode(ctx, node->right);
			if (!body_dagnode) return NULL;

			if (init_dagnode) init_dagnode->next = condition_dagnode;
			if (condition_dagnode) condition_dagnode->next = update_dagnode;

			result = create_dagnode(ctx, DAG_FOR, init_dagnode, body_dagnode, NULL, NULL, NULL, NULL, NULL);
			printf("\033[32mIn 'build_statement_dagnode', leaving NODE_FOR case\033[0m\n");
			break;
		}

		case NODE_BLOCK: {
			printf("\033[32mIn 'build_statement_dagnode', processing NODE_BLOCK case\033[0m\n");
			Node* stmt = node->right;
			DAGNode* head = NULL;
			DAGNode* current = NULL;
			while (stmt) {
				Node* next_stmt = stmt->next;
				right = build_statement_dagnode(ctx, stmt);
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
				stmt = next_stmt;
			}
			if (current) {
				current->next = NULL;
			}

			result = create_dagnode(ctx, DAG_BLOCK, NULL, head, NULL, NULL, NULL, NULL, NULL);
			printf("\033[32mIn 'build_statement_dagnode', leaving NODE_BLOCK case\033[0m\n");
			break;
		}
	}
	return result;
}

DAGNode* build_params_dagnode(CompilerContext* ctx, Node* wrapped_param) {
	if (!wrapped_param) return NULL;

	DAGNode* head = NULL;
	DAGNode* current = NULL;

	Node* current_wrapped_param = wrapped_param;
	while (current_wrapped_param) {
		Node* next_wrapped_param = current_wrapped_param->next;
		DAGNode* inner_param = build_expression_dagnode(ctx, current_wrapped_param->right);
		if (!inner_param) return NULL;
		DAGNode* wrap_inner_param = create_dagnode(ctx, DAG_PARAM, NULL, inner_param, NULL, NULL, NULL, NULL, NULL);
		if (wrap_inner_param) {
			if (!head) {
				head = wrap_inner_param;
				current = wrap_inner_param;
			} else {
				current->next = wrap_inner_param;
				wrap_inner_param->prev = current;
				current = wrap_inner_param;
			}
		} else {
			return NULL;
		}
		current_wrapped_param = next_wrapped_param;
	}

	if (current) {
		current->next = NULL;
	}
	return head;
}

DAGNode* build_global_dagnode(CompilerContext* ctx, Node* global_node) {
	if (!global_node) return NULL;

	switch (global_node->type) {
		case NODE_NAME: {
			if (global_node->t) {
				if (global_node->t->kind == TYPE_FUNCTION) {
					printf("\033[31mIn build_global_dag_node, processing function '%s'\033[0m\n", global_node->value.name);
					DAGNode* params = build_params_dagnode(ctx, global_node->t->params); 
					DAGNode* body = build_statement_dagnode(ctx, global_node->right);
					if (!body) return NULL;

					DAGNode* function_node = create_string_dagnode(ctx, DAG_NAME, global_node->value.name, NULL, body, NULL, NULL, params, global_node->symbol, global_node->t);
					if (!function_node) return NULL;
					printf("\033[31mAbout to return function dagnode for function '%s'.\033[0m\n", global_node->value.name);
					return function_node;
				}
			}
		}
		default: printf("unknown node.\n"); break;

	}
}

DAGNode* build_DAG(CompilerContext* ctx, Node* root) {
	if (!root) return;

	init_dag_node_table(ctx);
	printf("initialized dag table\n");
	DAGNode* head = NULL;
	DAGNode* current = NULL;

	Node* global_node = root;
	printf("In 'build_DAG', root node has type: %d\n", root->type);
	while (global_node) {
		Node* next_global_node = global_node->next;
		DAGNode* dag_node = build_global_dagnode(ctx, global_node);
		
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
			return NULL;
		}

		global_node = next_global_node;
	}
	return head;
}

// void free_dagnode_table() {
// 	if (!dag_node_table) return;
// 	for (int i = 0; i < dag_node_table->capacity; i++) {
// 		free_dag_entry(dag_node_table->entries[i]);
// 	}
// 	free(dag_node_table->entries);
// 	free(dag_node_table);
// }

// void free_dag_node(DAGNode* node) {
// 	if (!node || (node && node->freed)) return;

// 	node->freed = true;
// 	switch (node->kind) {
// 		case DAG_SUBSCRIPT:
// 		case DAG_LESS:
// 		case DAG_GREATER:
// 		case DAG_LESS_EQUAL:
// 		case DAG_GREATER_EQUAL:
// 		case DAG_EQUAL:
// 		case DAG_NOT_EQUAL:
// 		case DAG_LOGICAL_OR:
// 		case DAG_LOGICAL_AND:
// 		case DAG_ADD_EQUAL:
// 		case DAG_SUB_EQUAL:
// 		case DAG_MUL_EQUAL:
// 		case DAG_DIV_EQUAL:
// 		case DAG_MODULO:
// 		case DAG_ADD:
// 		case DAG_SUB:
// 		case DAG_MUL:
// 		case DAG_DIV:
// 		case DAG_ASSIGNMENT: {
// 			free_dag_node(node->left);
// 			free_dag_node(node->right);
// 			break;
// 		}

// 		case DAG_NAME: {
// 			if (node->value.name) {
// 				free(node->value.name); 
// 				node->value.name = NULL;
// 			}

// 			if (node->left) free_dag_node(node->left);
// 			if (node->right) free_dag_node(node->right);
// 			if (node->name) {
// 				free(node->name);
// 				node->name = NULL;
// 			}
// 			if (node->symbol) { free_symbol(node->symbol); }
// 			break;		
// 		}

// 		case DAG_DEF:
// 		case DAG_AUG:
// 		case DAG_DECL: {
// 			free_dag_node(node->left);
// 			if (node->name) free(node->name);
// 			break;
// 		}

// 		case DAG_INCREMENT:
// 		case DAG_DECREMENT: {
// 			free_dag_node(node->left);
// 			break;
// 		}

// 		case DAG_RETURN: {
// 			if (!node->right) break; 
// 			free_dag_node(node->right);
// 			break;
// 		}

// 		case DAG_NOT:
// 		case DAG_PARAM:	
// 		case DAG_ARG: {
// 			free_dag_node(node->right);
// 			if (node->symbol) { free_symbol(node->symbol); }
// 			break;
// 		}	

// 		case DAG_CALL: {
// 			if (node->left) free_dag_node(node->left);
// 			if (node->right) {
// 				DAGNode* arg = node->right;
// 				while (arg) {
// 					free_dag_node(arg);
// 					arg = arg->next;
// 				}
// 			}
// 			if (node->symbol) { free_symbol(node->symbol); }
// 			break;
// 		}

// 		case DAG_BLOCK: {
// 			DAGNode* stmt = node->right;
// 			while (stmt) {
// 				free_dag_node(stmt);
// 				stmt = stmt->next;
// 			}
// 			break;
// 		}

// 		case DAG_INTEGER:
// 		case DAG_BOOL:
// 		case DAG_CHAR: {
// 			if (node->name) free(node->name);
// 			break;
// 		}
// 		case DAG_CONTINUE:
// 		case DAG_BREAK: {
// 			break;
// 		}
// 	}
// 	free(node);
// }

// void free_dag_entry(DAGNodeEntry* entry) {
// 	if (!entry || (entry && entry->freed)) return;


// 	if (entry->key) { free(entry->key); }
// 	free_dag_node(entry->node);
// 	if (entry->link) {
// 		DAGNodeEntry* current_link = entry->link;
// 		while (current_link) {
// 			DAGNode* next_link = current_link->link;
// 			free_dag_entry(current_link);
// 			current_link = next_link; 
// 		}
// 	}
// 	entry->freed = true;
// 	free(entry);
// }

// void free_dag(DAGNode* root) {
// 	if (!root) return;

// 	free_dagnode_table();

// 	DAGNode* node = root;
// 	while (node) {
// 		DAGNode* next = node->next;
// 		free_dag_node(node);
// 		node = next;
// 	}

// }


