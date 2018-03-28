#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define MAX_SLEEP_TIME	2
#define MIN_SLEEP_TIME	0

void *threadFunction(void *argument);

typedef struct thread_t {
	pthread_t* threads;
	int level;
} thread_t;

int n;

int main(int argc, char** argv) {
	pthread_t tid1, tid2;

	// Check the number of parameters
	if(argc < 2) {
		fprintf(stderr, "Parameters error \n"
			"Correct usage is %s n \n"
			"<n> height of the tree to generate \n", argv[0]);
		exit(1);
	}

	n = atoi(argv[1]);
	if(n <= 0) {
		fprintf(stderr, "Parameter error \n"
			"<n> must be a positive value \n");
		exit(2);
	}
	setbuf(stdout, NULL);
	srand(time(NULL));

#ifdef DBG
	fprintf(stdout, "START -> Thread %ld. Level %d started\n", pthread_self(), 0);
#endif

	int level = 0;
	thread_t *t;
	t = (thread_t*) malloc(sizeof(thread_t));
	t->threads = (pthread_t*) malloc((level + 1) * sizeof(pthread_t));
	t->level = level + 1;
	t->threads[0] = pthread_self();

	pthread_create(&tid1, NULL, threadFunction, (void*) t);
	pthread_create(&tid2, NULL, threadFunction, (void*) t);

#ifdef DBG
	fprintf(stdout, "END   -> Thread %ld. Level %d terminated\n", pthread_self(), 0);
#endif

	pthread_exit(0);
}

void *threadFunction(void *argument) {
	thread_t* parent = (thread_t*) argument;
	int sleepTime = rand() % MAX_SLEEP_TIME + MIN_SLEEP_TIME;

#ifdef SLEEP
	sleep(sleepTime);
#endif

	int i, level = parent->level;

#ifdef DBG
	fprintf(stdout, "START -> Thread %ld. Level %d started\n", pthread_self(), level);
#endif

	if(level >= n) { // Termination
#ifdef DBG
		fprintf(stdout, "END   -> Thread level %d terminated\n", level);
#endif
		fprintf(stdout, "Thread tree: %ld ", pthread_self());
		for(i = 0; i < n; i++)
			fprintf(stdout, "%ld ", parent->threads[i]);
		fprintf(stdout, "\n");
		pthread_exit(0);
	}

	pthread_t tid1, tid2;
	thread_t *t;

	t = (thread_t*) malloc(sizeof(thread_t));
	t->threads = (pthread_t*) malloc((level + 1) * sizeof(pthread_t));
	t->level = level + 1;
	t->threads[0] = pthread_self();
	for(i = 0; i < parent->level; i++)
		t->threads[i+1] = parent->threads[i];

	t->level = level + 1;

	pthread_create(&tid1, NULL, threadFunction, (void*) t);
	pthread_create(&tid2, NULL, threadFunction, (void*) t);

#ifdef DBG
	fprintf(stdout, "END   -> Thread %ld. Level %d terminated\n", pthread_self(), level);
#endif

	pthread_exit(0);
}