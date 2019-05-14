#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#define NUMTHREADS 3

typedef struct node {
	int          data;
	struct node  *next;
} node_t;

typedef struct list {
	node_t           *head;
	pthread_mutex_t  lock;
	unsigned int 	 length;
} list_t;


void *thread_func(void *argument);
int insert(int value);
int del(int value);
int lookup(int value);

int del_fract;
int insert_fract;
int look_fract;

unsigned int range;

unsigned int in = 0;
unsigned int deletes = 0;
unsigned int look = 0;

list_t *list = NULL;

/*typedef struct thread_data {
	list_t *list;
	int val;
} thread_data;
*/

list_t* new_list() {
	list_t *tmp = (list_t *)malloc(sizeof(list_t));
	if(tmp == NULL) 
		fprintf(stderr,  "Unable to allocate memory for new list.\n");
	
	tmp->head = NULL;
	tmp->length = 0;
	pthread_mutex_init(&(tmp->lock), NULL);
	
	return tmp; 
}

node_t* new_node(int value) {
	node_t *tmp = (node_t *)malloc(sizeof(node_t));
	if( tmp == NULL ) 
		fprintf(stderr,  "Unable to allocate memory for new node.\n");
	
	tmp->data = value;
	tmp->next = NULL;
	
	return tmp;
}

void list_print() {
	node_t *tmp = list->head;
	while(tmp != NULL) {
		printf("%d ", tmp->data);
		tmp = tmp->next;
	}
}

void list_free() {
	node_t *tmp;

	while(list->head != NULL) {
		tmp = list->head;
		list->head = list->head->next;
		free(tmp);
	}
	pthread_mutex_destroy(&(list->lock));
	free(list);
}

int insert(int value) {
	node_t *curr;
	node_t *prev;
	node_t *tmp;

	pthread_mutex_lock( &(list->lock));

	curr = list->head;
	prev = NULL;
	
	while(curr != NULL && curr->data < value) {
		prev = curr;
		curr = curr->next;
	}

	if(curr == NULL || curr->data > value) {
		tmp = new_node(value);
        tmp->next = curr;
        if(prev == NULL) {
            list->head = tmp;
			list->length++;
            pthread_mutex_unlock( &(list->lock));
            return 1;
        } else {
            prev->next = tmp;
            list->length++;
            pthread_mutex_unlock( &(list->lock));
            return 1;
		}
    } else {
        pthread_mutex_unlock( &(list->lock));
        return 0;
    }
}

int look_up(int value){
	node_t *curr;

	pthread_mutex_lock(&(list->lock));/* we lock this because while I search for given node, another function mght be trying to delete this node*/

	curr = list->head;

	while(curr != NULL && curr->data < value) {
		curr= curr-> next;
	}

	if( curr ==  NULL ) { /* hit end of list and didnt find it*/
		pthread_mutex_unlock(&(list->lock));
		return 0;
	}
	pthread_mutex_unlock(&(list->lock));
	return ( curr->data == value ) ? 1 : 0;
}

int del(int key) {
	node_t *curr, *prev;
	node_t *tmp;

	pthread_mutex_lock(&(list->lock));

	curr = list->head;
	prev = NULL;

	if( curr == NULL ) {
		pthread_mutex_unlock(&(list->lock)); 
		return 0;
	}

	if( curr->data == key ){ /*its the first node that we want to delete*/
		list->head= list->head->next;
		free(curr);
		list->length--;
		pthread_mutex_unlock(&(list->lock));
		return 1;
	}

	while( curr != NULL && curr->data < key) {
		prev = curr;
		curr = curr->next;
	}

	if( curr == NULL ) {
		pthread_mutex_unlock(&(list->lock)); 
		return 0;
	}

	if(curr->data == key ) {
		tmp = curr;
		prev->next = curr->next;
		free(tmp);
		list->length--;
		pthread_mutex_unlock(&(list->lock));
		return 1;
	}
}

void *thread_func(void *argument) {
	
	long value = (long)argument;
	
	while( in < insert_fract) {
		
		insert( rand() % insert_fract );

		//pthread_mutex_lock( &count_lock);
		//printf("thread %d inserts\n", pthread_self());
		in++;
		//pthread_mutex_unlock( &count_lock);
	}
	
	while( deletes < del_fract ) {
		
		del( rand() % insert_fract );
		
		//pthread_mutex_lock( &count_lock);
		//printf("thread %d deletes\n", pthread_self());
		deletes++;
		//pthread_mutex_unlock( &count_lock);
	
	}

	while( look < look_fract ) {

		look_up( rand() % insert_fract );
		
		//pthread_mutex_lock( &count_lock);
		//printf("thread %d looks\n", pthread_self());
		look++;
		//pthread_mutex_unlock( &count_lock);
	}

	pthread_exit(NULL);
}

void * void_insert(void *argument) {
	while( in < insert_fract ) {
		printf("thread %d does insert\n", pthread_self());
		insert( in);
		in++;		
	}
	pthread_exit(NULL);
}

void * void_del(void * argument) {
	while( deletes < del_fract ) {
		printf("thread %d does delete\n", pthread_self());
		del( deletes);
		deletes++;
	}
	pthread_exit(NULL);
}

void * void_look(void * argument) {
	while( look < look_fract ) {
		printf("thread %d does look up\n", pthread_self());
		look_up(look);
		look++;
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	srand(getpid());
	int i, rc;
	pthread_t threads[NUMTHREADS];
	node_t *tmp;
	
	pthread_attr_t attr;
	void* status;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if( argc < 4 ) {
		printf("Not enough arguments. Run with parameters: <max value of keys> <insert fraction> <delete fraction> <look_up fraction> \n");
		exit(1);
	}
	
	list = new_list();

	range = atoi(argv[1]);

	insert_fract = (int) (atof(argv[2]) * 10000);
	del_fract = (int) (atof(argv[3]) * 10000);
	look_fract = (int) (atof(argv[4]) * 10000);

	printf("Inserts: %d, Deletes:%d, Look ups:%d\n", insert_fract, del_fract, look_fract);
	
	// for(i = 0; i < NUMTHREADS; i++) {
	// 	rc = pthread_create(&threads[i], &attr, thread_func, (void *) NULL);
	// 	if (rc){
    //       printf("ERROR; return code from pthread_create() is %d\n", rc);
    //       exit(-1);
    //    }
	// }
		
	pthread_create(&threads[0], &attr, void_del, NULL);
	pthread_create(&threads[1], &attr, void_look, NULL);
	pthread_create(&threads[2], &attr, void_insert, NULL);
	
	
	pthread_attr_destroy(&attr);

	for(i = 0; i < NUMTHREADS; i++) {
		pthread_join(threads[i], &status);
	}

	printf("\nLength is %d\n", list->length);
	
	list_free();
	pthread_exit(NULL);
	return 1;
}
