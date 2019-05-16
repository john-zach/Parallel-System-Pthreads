#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#define NUMTHREADS 2

typedef struct node {
        int              data;
        struct node      *next;
        pthread_mutex_t  lock;
} node_t;

typedef struct list {
        node_t           *head;
        unsigned int     length;
} list_t;

list_t *list = NULL;

void *thread_func(void *argument);

int insert(int value);
int del(int value);
int lookup(int value);

unsigned int in = 0;
unsigned int deletes = 0;
unsigned int look = 0;

unsigned int insert_fract = 0;
unsigned int del_fract = 0;
unsigned int look_fract = 0;

unsigned int range = 0;

pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;
list_t * new_list();
node_t * new_node(int value);

list_t* new_list() {
        list_t *tmp = (list_t *)malloc(sizeof(list_t));
        if(tmp == NULL)
                fprintf(stderr,  "Unable to allocate memory for new list.\n");

        tmp->head = NULL;
        tmp->length = 0;
        return tmp;
}

node_t* new_node(int value) {
        node_t *tmp = (node_t *)malloc(sizeof(node_t));
        if( tmp == NULL )
                fprintf(stderr, "Unable to allocate memory for new node.\n");

        tmp->data = value;
        tmp->next = NULL;
        pthread_mutex_init(&(tmp->lock), NULL);
        return tmp;
}

void list_print() {
        node_t *tmp = list->head;
        
        while(tmp != NULL) {
                printf("%d ", tmp->data);
                tmp = tmp->next;
        }
        printf("\n");
}

void* void_insert(void *argument) {
	while( in < insert_fract ) {
		printf("thread %ld does insert %d\n", pthread_self(), in);
		insert(rand() % range);
                //pthread_mutex_lock(&(count_lock));
		in++;		
                //pthread_mutex_unlock(&(count_lock));
	}
	pthread_exit(NULL);
}

void* void_del(void * argument) {
	while( deletes < del_fract ) {
		printf("thread %ld does delete\n", pthread_self());
		del( rand() % range);
		deletes++;
	}
	pthread_exit(NULL);
}

void* void_look(void * argument) {
	while( look < look_fract ) {
		printf("thread %ld does look up\n", pthread_self());
		lookup( rand() % range);
		look++;
	}
	pthread_exit(NULL);
}



int insert(int value) {
        node_t *curr;
        node_t *prev;
        node_t *temp;
        temp = new_node(value);
        //printf("------------------INSERT %d----------------------\n", value);

        if( list->head == NULL || list->head->data > value) {
                if(list->head == NULL) {
	        //printf("insert if list is null\n");
                temp->next = list->head;
                list->head = temp;
                return 1;
                } else {
                        pthread_mutex_lock(&(list->head->lock));
                        //printf("locked %d\n", list->head->data);
                        temp->next = list->head;
                        list->head = temp;
                        pthread_mutex_unlock(&(list->head->lock));
                        pthread_mutex_unlock(&(list->head->next->lock));
                        //printf("unlocked %d\n", list->head->data);
                        return 1;
                }
        }

        prev = list->head;
        curr = prev->next;

        if(curr != NULL) {
	       pthread_mutex_lock(&(curr->lock));
               //printf("locked22 %d\n", curr->data);
        }

        pthread_mutex_lock(&(prev->lock));
        //printf("locked %d\n", prev->data);

        while(curr != NULL && curr->data < value) {
	        pthread_mutex_unlock(&(prev->lock));
                //pthread_mutex_unlock(&(curr->lock));
		//printf("unlocked prev(%d)\n", prev->data);
                prev = curr;
		//printf("1, curr is %d \n", curr->data);
                curr = curr->next;
	 	//printf("2 prev(%d)\n", prev->data);
		if(curr != NULL){
	                //printf("locking current(%d)\n", curr->data);
	                pthread_mutex_lock(&(curr->lock));
		}

      }

        if(curr == NULL && prev->data != value) { /*reached end of list*/
                prev->next = temp;
                pthread_mutex_unlock(&(prev->lock));
                return 1;
        } else {
                if(curr->data == value || prev->data==value) { /*already exist*/
                        pthread_mutex_unlock(&(curr->lock));
                        pthread_mutex_unlock(&(prev->lock));
                        return 0;
                }

                temp->next = curr;
                prev->next = temp;

                pthread_mutex_unlock(&(curr->lock));
                pthread_mutex_unlock(&(prev->lock));
                return 1;
	}
}

int del(int value){
	node_t  *curr = list->head, *prev = NULL;

        pthread_mutex_lock(&(curr->lock));
        pthread_mutex_lock(&(prev->lock));//????????????????????????

        if( curr->data == value ){ /*its the first node that we want to delete*/
                list->head= list->head->next;
                free(curr);
                list->length--;
                pthread_mutex_unlock(&(curr->lock));
                pthread_mutex_unlock(&(prev->lock));
                return 1;

        }

	while(curr!=NULL && curr->data < value){

                pthread_mutex_unlock(&(prev->lock));
		prev= curr;
		curr= curr->next;
	        pthread_mutex_lock(&(curr->lock));
	}

        if(curr->data == value ) {
               // tmp = curr;
                prev->next = curr->next;
                //free(tmp);
                list->length--;
                pthread_mutex_unlock(&(curr->lock));
                pthread_mutex_unlock(&(prev->lock));
                return 1;
        } else {                                /* Not found or hit NULL(end of list)*/
                pthread_mutex_unlock(&(curr->lock));
                pthread_mutex_unlock(&(prev->lock));
                return 0;
        }
}

int lookup(int value) {
        return 0;
}

int main(int argc, char *argv[]) {
        node_t *tmp;
	pthread_t threads[NUMTHREADS];
        int i, rc;
        srand(getpid());
       
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

	printf("Inserts: %d, Deletes: %d, Look ups: %d\n", insert_fract, del_fract, look_fract);

        // for(i = 0; i < NUMTHREADS; i++) {
	// 	rc = pthread_create(&threads[i], NULL, thread_func, (void *) i);
	// 	if (rc){
        //   printf("ERROR; return code from pthread_create() is %d\n", rc);
        //   exit(-1);
        //         }
	// }

        pthread_create(&threads[0], &attr, void_insert, (void *) NULL);
        pthread_create(&threads[1], &attr, void_insert, (void *) NULL);
        // pthread_create(&threads[2], NULL, void_look, (void *) NULL);

	pthread_attr_destroy(&attr);

        for(i = 0; i < NUMTHREADS; i++) {
		pthread_join(threads[i], NULL);
	}

        //list_print();
        printf("length is %d", list->length);
        pthread_exit(NULL);
        return 0;
}