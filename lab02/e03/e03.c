#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_SLEEP_TIME	3
#define MIN_SLEEP_TIME	1
#define FILENAME_LENGHT 10

typedef struct thread_t {
	int thread_no;
	char* filename;
} thread_t;

int* generateArray(int size, int minNumber, int maxNumber);
void printArray(int *v, int size);
void writeToFile(int *v, int size, char* filename);
void* threadFunction(void* argument);

int g;
sem_t *processing, *dataProcessed, *mutexRead;

int main(int argc, char** argv) {
	int n1, n2, numberRequests = 0, numberTerminated = 0;
	int *v1, *v2;
	pthread_t tid1, tid2;

	// Check the number of parameters
	if(argc < 3) {
		fprintf(stderr, "Parameters error \n"
			"Correct usage is %s <n1> <n2> \n"
			"<n1>, <n2> vector dimensions \n", argv[0]);
		exit(1);
	}

	n1 = atoi(argv[1]);
	n2 = atoi(argv[2]);

	if(n1 < 0 || n2 < 0) {
		fprintf(stderr, "Parameter error \n"
			"<n1> and <n2> must be positive values \n");
		exit(2);
	}

	v1 = generateArray(n1, 10, 100);
	v2 = generateArray(n2, 21, 101);

	setbuf(stdout, NULL);
	srand((unsigned) time(NULL));

//	printArray(v1, n1);
//	printArray(v2, n2);

	writeToFile(v1, n1, "fv1.b");
	writeToFile(v2, n2, "fv2.b");

	processing = (sem_t*) malloc(sizeof(sem_t));
	mutexRead = (sem_t*) malloc(sizeof(sem_t));
	dataProcessed = (sem_t*) malloc(sizeof(sem_t));

	sem_init(processing, 0, 0);
	sem_init(mutexRead, 0, 1);
	sem_init(dataProcessed, 0, 0);

	thread_t t1, t2;
	t1.thread_no = 1;
	t1.filename = "fv1.b";

	t2.thread_no = 2;
	t2.filename = "fv2.b";

	pthread_create(&tid1, NULL, threadFunction, (void*) &t1);
	pthread_create(&tid2, NULL, threadFunction, (void*) &t2);

	while(numberTerminated < 2) {
		fprintf(stdout, "%02d. Main is here\n", numberRequests);
		sem_wait(processing);		// Wait for a request
		if(g == -1) {
			numberTerminated++;
			continue;
		}
		g *= 3;
		sem_post(dataProcessed);	// Request served. Response can be read
		numberRequests++;
	}

	sem_destroy(processing);
	sem_destroy(mutexRead);
	sem_destroy(dataProcessed);

	fprintf(stdout, "Server received %d requests\n", numberRequests);

	pthread_exit(0);
}

int* generateArray(int size, int minNumber, int maxNumber) {
	int* v = (int*) malloc(size * sizeof(int));
	int i;
	for(i = 0; i < size; i++)
		v[i] = rand() % (maxNumber - minNumber + 1) + minNumber;
	return v;
}

void printArray(int *v, int size) {
	int i;
	for(i = 0; i < size; i++)
		fprintf(stdout, "%d; ", v[i]);
	fprintf(stdout, "\n");
}

void writeToFile(int *v, int size, char* filename) {
	int fd;
	if((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0664)) < 0) {
		fprintf(stderr, "Error opening %s\n", filename);
		exit(2);
	}
	write(fd, v, size * sizeof(int));
	close(fd);
}

void* threadFunction(void* argument) {
	thread_t t = *(thread_t*) argument;
	int number, fd, exitWhile = 0;

	fprintf(stdout, "Thread #%02d started on file %s\n", t.thread_no, t.filename);

	if((fd = open(t.filename, O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening %s\n", t.filename);
		exit(2);
	}

	while(! exitWhile) {
		sem_wait(mutexRead);		// Wait before entering in the critical section
#ifdef SLEEP
		sleep(rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME + 1) + MIN_SLEEP_TIME);
#endif
		if(read(fd, &g, sizeof(int)) <= 0) {
			g = -1;		// Main process can recognize the termination
			break;
		}
		number = g;
		sem_post(processing);		// Wake up server
		sem_wait(dataProcessed);	// Wait for server response
		fprintf(stdout, "Thread #%02d Input %3d Output %3d\n", t.thread_no, number, g);
		sem_post(mutexRead);		// Unlock mutual exclusion
	}

	sem_post(processing);			// Server can elaborate the last (dummy) element
	// Do not wait for server response. It should discharge dummy requests
	fprintf(stdout, "Thread #%02d terminated\n", t.thread_no);
	sem_post(mutexRead);			// Unlock mutual exclusion

	close(fd);
	pthread_exit(NULL);
}