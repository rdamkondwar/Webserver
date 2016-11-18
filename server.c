#include "cs537.h"
#include "request.h"

#include <pthread.h>

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

int *buffer;
int req_buffer_size;
int count = 0;
int producer_ptr = 0;
int consumer_ptr = 0;
pthread_cond_t cond_empty, cond_fill;

typedef struct {
  pthread_t thread_id;
  int       thread_num;
  char     *argv_string;
} thread_info;

thread_info *tinfo;
//TODO: Destroy mutex at end
pthread_mutex_t mutex;

void init_buffer(int buffer_size) {
  printf("debug: Init Buffer with size %d\n", buffer_size);
  req_buffer_size = buffer_size;
  int i = 0;
  buffer = (int *) malloc(buffer_size * sizeof(int));
  for (i = 0; i < buffer_size; i++) {
    buffer[i] = -1;
  }
  count = 0;

  if (pthread_mutex_init(&mutex, NULL) != 0) {
    printf("\n mutex init failed\n");
    exit(1);
  }

  if (pthread_cond_init(&cond_empty, NULL) != 0) {
    printf("cond init failed\n");
    exit(1);
  }
  if (pthread_cond_init(&cond_fill, NULL) != 0) {
    printf("cond init failed\n");
    exit(1);
  }
}

void insert_into_buffer(int val) {
  // printf("debug: inserting %d into buffer idx %d\n", val, producer_ptr);
  buffer[producer_ptr] = val;
  producer_ptr = (producer_ptr + 1) % req_buffer_size;
  count++;
}

void insert_new_req(int connfd) {
  pthread_mutex_lock(&mutex);
  while(count == req_buffer_size) {
    pthread_cond_wait(&cond_empty, &mutex);
  }
  insert_into_buffer(connfd);
  pthread_cond_signal(&cond_fill);    
  pthread_mutex_unlock(&mutex);
}

int get_req_from_buffer() {
  int val = -1;
  val = buffer[consumer_ptr];
  // printf("debug: consuming %d from buffer idx %d\n", val, consumer_ptr);
  buffer[consumer_ptr] = -1;
  consumer_ptr = (consumer_ptr + 1) % req_buffer_size;
  count--;
  
  return val;
}

int get_new_req() {
  int val = -1;
  pthread_mutex_lock(&mutex);
  while(count == 0) {
    pthread_cond_wait(&cond_fill, &mutex);
  }
  val = get_req_from_buffer();
  pthread_cond_signal(&cond_empty);
  pthread_mutex_unlock(&mutex);
  return val;
}

void* thread_consumer_start(void *arg) {
  //Infinite loop
  // thread_info *tinfo = (thread_info *)arg;
  while(1) {
    // printf("in thread consumer %lu\n", tinfo->thread_id);
    int connfd = get_new_req();
    // printf("Got new request %d\n", connfd);
    if (connfd > 0) {
      requestHandle(connfd);
      Close(connfd);
      // printf("Done request %d\n", connfd);
    }
    else {
       printf("No request found\n");
    }
    // sleep(1);
  }
}

void create_threads(int thread_count) {
  int i, ret;
  printf("debug: Creating threads %d\n", thread_count);
  tinfo = calloc(thread_count, sizeof(thread_info));
  if (tinfo == NULL) {
      //handle exit
      exit(1);
  }

  for (i = 0; i < thread_count; i++) {
    tinfo[i].thread_num = i + 1;
    tinfo[i].argv_string = NULL;

    ret = pthread_create(&tinfo[i].thread_id, NULL,
		       &thread_consumer_start, &tinfo[i]);
    if (0 != ret) {
      perror("pthread error");
      exit(1);
    }
  }
}

// CS537: Parse the new arguments too
void getargs(int *port, int argc, char *argv[], int *buffer, int *threads) {
    if (argc != 4) {
	fprintf(stderr, "Usage: %s <port> <threads_cnt> <buffer_size>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *buffer = atoi(argv[3]);
}

int main(int argc, char *argv[]) {
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    
    int buffer_size, thread_count;

    getargs(&port, argc, argv, &buffer_size, &thread_count);

    // Validate arguments -> port, buffer_size, thread_cnt
    // Setup buffer
    init_buffer(buffer_size);
    // Start N threads
    create_threads(thread_count);
    // Invoke requestHandle in thread_func

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// CS537: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. However, for SFF, you may have to do a little work
	// here (e.g., a stat() on the filename) ...
	//
	insert_new_req(connfd);
	// requestHandle(connfd);
    }
}
