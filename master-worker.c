#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

int item_to_produce = 0, curr_buf_size = 0;
int total_items, max_buf_size, num_workers, num_masters;

pthread_mutex_t mutex;
pthread_cond_t not_full;
pthread_cond_t not_empty;

int *buffer;

void print_produced(int num, int master) {
  printf("Produced %d by master %d\n", num, master);
}

void print_consumed(int num, int worker) {
  printf("Consumed %d by worker %d\n", num, worker);
}

void *generate_requests_loop(void *data) {
  int thread_id = *((int *)data);

  while (1) {
    pthread_mutex_lock(&mutex);
    while (curr_buf_size >= max_buf_size && item_to_produce < total_items) {
      pthread_cond_wait(&not_full, &mutex);
    }
    if (item_to_produce >= total_items) {
      pthread_cond_broadcast(&not_empty);
      pthread_mutex_unlock(&mutex);
      break;
    }
    buffer[curr_buf_size++] = item_to_produce;
    print_produced(item_to_produce, thread_id);
    item_to_produce++;
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

void *consume_requests_loop(void *data) {
  int thread_id = *((int *)data);
  int item;

  while (1) {
    pthread_mutex_lock(&mutex);
    while (curr_buf_size == 0 && item_to_produce < total_items) {
      pthread_cond_wait(&not_empty, &mutex);
    }
    if (curr_buf_size == 0 && item_to_produce >= total_items) {
      pthread_mutex_unlock(&mutex);
      break;
    }
    item = buffer[--curr_buf_size];
    print_consumed(item, thread_id);
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc < 5) {
    printf("./master-worker #total_items #max_buf_size #num_workers #masters\n");
    exit(1);
  }

  total_items = atoi(argv[1]);
  max_buf_size = atoi(argv[2]);
  num_workers = atoi(argv[3]);
  num_masters = atoi(argv[4]);

  buffer = (int *)malloc(sizeof(int) * max_buf_size);
  pthread_t *master_threads = (pthread_t *)malloc(sizeof(pthread_t) * num_masters);
  pthread_t *worker_threads = (pthread_t *)malloc(sizeof(pthread_t) * num_workers);
  int *master_thread_id = (int *)malloc(sizeof(int) * num_masters);
  int *worker_thread_id = (int *)malloc(sizeof(int) * num_workers);

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&not_full, NULL);
  pthread_cond_init(&not_empty, NULL);

  for (int i = 0; i < num_masters; i++) {
    master_thread_id[i] = i;
    pthread_create(&master_threads[i], NULL, generate_requests_loop, (void *)&master_thread_id[i]);
  }
  for (int i = 0; i < num_workers; i++) {
    worker_thread_id[i] = i;
    pthread_create(&worker_threads[i], NULL, consume_requests_loop, (void *)&worker_thread_id[i]);
  }

  for (int i = 0; i < num_masters; i++) {
    pthread_join(master_threads[i], NULL);
    printf("master %d joined\n", i);
  }
  for (int i = 0; i < num_workers; i++) {
    pthread_join(worker_threads[i], NULL);
    printf("worker %d joined\n", i);
  }

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&not_full);
  pthread_cond_destroy(&not_empty);

  free(buffer);
  free(master_threads);
  free(worker_threads);
  free(master_thread_id);
  free(worker_thread_id);

  return 0;
}
