#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mapreduce.h"

Mapper mapper;
Reducer reducer;
pthread_mutex_t map_lock;
Partitioner partitioner;
int reducer_num;
int file_num;
char** file_names;
struct tree* partition_array;
struct node* temp_node;
int map_count;

struct node {
	char* key;
	char* value;
	int count;
	struct node *left, *right, *parent;
};

typedef struct tree {
	struct node *node; //stores info of each node
	struct node *record; //stores the parent of node that is recently returned for finding the next node faster
	struct node *current_node;
	char* current_key;
	pthread_mutex_t lock;
}tree;

struct node *newNode(char* key, char* value, struct node* parent)
{
	struct node *temp = (struct node *)malloc(sizeof(struct node));
	temp->key = key;
	temp->value = value;
	temp->left = NULL;
	temp->right = NULL;
	temp->count = 1;
	temp->parent = parent;
	return temp;
}
void insert(char* key, char* value, struct node* node, struct node* parent_node, struct tree* tree, int direction) {
	/* If the tree is empty, return a new node */
	
	if (node == NULL) {
		struct node* temp = newNode(key, value, parent_node);
		pthread_mutex_lock(&tree->lock);
		if (tree->node == NULL) {
			tree->node = temp;
		}
		if (direction == -1) {
			parent_node->left = temp;
		}
		if (direction == 1) {
			parent_node->right = temp;
		}
		pthread_mutex_unlock(&tree->lock);
	}
	else {
		/* Otherwise, recur down the tree */
		if (strcmp(key, node->key) < 0)
			insert(key, value, node->left, node, tree, -1);
		else if (strcmp(key, node->key) > 0)
			insert(key, value, node->right, node, tree, 1);
		else if (strcmp(key, node->key) == 0) {
			if (strcmp(value, node->value) < 0)
				insert(key, value, node->left, node, tree, -1);
			else if (strcmp(value, node->value) > 0)
				insert(key, value, node->right, node, tree, 1);
			else if (strcmp(value, node->value) == 0) {
				node->count++;
			}
		}
	}
	pthread_mutex_unlock(&tree->lock);
}
struct node* findmin(struct node* node) {
	/* loop down to find the leftmost leaf */
	while (node->left != NULL)
	{
		node = node->left;
	}
	return node;
}
struct node* findnext(struct tree* tree) {
	struct node* return_val;
	if (tree->node == NULL) {
		return NULL;
	}
	return_val = findmin(tree->node); //find left-most node
	if (return_val == tree->node) {
		tree->record = return_val->right;//now record = the return node's parent
		tree->node = tree->node->right;
		tree->record = tree->node;
	}
	else {
		if (return_val->right == NULL) {
			tree->record = return_val->parent;
			tree->record->left = NULL;
		}
		else {
			tree->record = return_val->right;
			return_val->parent->left = tree->record;
			tree->record->parent = return_val->parent;
		}
	}
	return return_val;
}
char* get_next(char *key, int partition_number) {
	struct tree* tree = &partition_array[partition_number];
	struct node* temp = tree->current_node;

	if (temp->count == 0) {
		temp = findnext(tree);
		tree->current_node = temp;

		if (temp == NULL) {
			return NULL;
		}
		if (strcmp(temp->key, tree->current_key) == 0) {
			temp->count--;
			return temp->value;
		}
		else {
			tree->current_key = temp->key;
			return NULL;
		}
	}
	else {
		temp->count--;
		return temp->value;
	}
}


void* mapping() {
	for (;;) {
		pthread_mutex_lock(&map_lock);
		if (map_count >= file_num) {
			break;
		}
		map_count++;
		int temp = map_count;
		pthread_mutex_unlock(&map_lock);
		mapper(file_names[temp]);
	}
	return NULL;
}

void* reducing(void* partition_index) {
	int partition_number = *(int*)partition_index;
	struct tree* tree = &partition_array[partition_number];
	struct node* temp = findnext(tree);
	if (temp == NULL) {
		return NULL;
	}
	tree->current_node = temp;
	tree->current_key = temp->key;
	for (;;) {
		if (tree->current_node == NULL)
			return NULL;
		reducer(tree->current_key, get_next, partition_number);
	}
}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
	unsigned long hash = 5381;
	int c;
	while ((c = *key++) != '\0')
		hash = hash * 33 + c;
	return hash % num_partitions;
}
void MR_Emit(char *key, char *value) {
	tree* t = &partition_array[partitioner(key, reducer_num)];
	char* key_temp = malloc(strlen(key) + 1);
	if (key_temp == NULL) {
		perror("malloc failed\n");
		return;
	}
	strcpy(key_temp, key);
	char* value_temp = malloc(strlen(value) + 1);
	if (value_temp == NULL) {
		perror("malloc failed\n");
		return;
	}
	strcpy(value_temp, value);
	insert(key_temp, value_temp, t->node, NULL, t, 0);
	return;
}
void MR_Run(int argc, char *argv[],
	Mapper map, int num_mappers,
	Reducer reduce, int num_reducers,
	Partitioner partition) {
	mapper = map;
	reducer = reduce;
	partitioner = partition;
	reducer_num = num_reducers;
	file_num = argc - 1;
	file_names = argv;
	temp_node = NULL;
	map_count = 0;
	partition_array = (struct tree*)malloc(sizeof(struct tree) * num_reducers);
	if (partition_array == NULL) {
		perror("malloc failed\n");
	}
	for (int i = 0; i < num_reducers; i++) {
		partition_array[i].node = NULL;
		partition_array[i].record = NULL;
		partition_array[i].current_node = NULL;
		pthread_mutex_init(&partition_array[i].lock, NULL);
	}
	int num_thread;
	pthread_mutex_init(&map_lock, NULL);
	if (num_mappers < file_num) {
		num_thread = file_num;
	}
	else {
		num_thread = num_mappers;
	}
	pthread_t thread_array[num_thread];
	for (int i = 0; i < num_thread; i++) {
		pthread_create(&thread_array[i], NULL, mapping, NULL);
	}
	for (int i = 0; i < num_thread; i++) {
		pthread_join(thread_array[i], NULL);
	}

	pthread_t reduce_array[reducer_num];
	for (int i = 0; i < reducer_num; i++) {
		void* arg = malloc(sizeof(int*));
		*(int*)arg = i;
		pthread_create(&reduce_array[i], NULL, reducing, arg);
	}
	for (int i = 0; i < reducer_num; i++) {
		pthread_join(reduce_array[i], NULL);
	}
}
