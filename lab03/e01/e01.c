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
#include <sys/time.h>

#define MAX_SLEEP_TIME	10	/* milliseconds */
#define MIN_SLEEP_TIME	1	/* milliseconds */
#define ELEMENTS		100
#define BUFFER_SIZE		16

typedef struct bufferData {
	long long ms;
	unsigned short int bufferID;
} bufferData;

int p;
bufferData normalBuffer[BUFFER_SIZE], urgentBuffer[BUFFER_SIZE];
sem_t *normalEmpty, *normalFull, *urgentEmpty, *urgentFull;
sem_t *globalEmpty, *globalFull;
int normalIn, normalOut, urgentIn, urgentOut;

void init();
void clean();
void* producer(void* argument);
void* consumer(void* argument);
long long current_timestamp();

int main(int argc, char** argv) {
	pthread_t tidP, tidC;

	// Check the number of parameters
	if(argc < 2) {
		fprintf(stderr, "Parameters error \n"
			"Correct usage is %s <p> \n"
			"<p> probability to produce a normal data (percentage)\n", argv[0]);
		exit(1);
	}

	p = atoi(argv[1]);
	if(p < 0 || p > 100) {
		fprintf(stderr, "Invalid probability. <p> must be between 0 and 100\n");
		exit(2);
	}

	normalEmpty	= (sem_t*) malloc(sizeof(sem_t));
	normalFull	= (sem_t*) malloc(sizeof(sem_t));
	urgentEmpty	= (sem_t*) malloc(sizeof(sem_t));
	urgentFull	= (sem_t*) malloc(sizeof(sem_t));
	globalEmpty	= (sem_t*) malloc(sizeof(sem_t));
	globalFull	= (sem_t*) malloc(sizeof(sem_t));

	init();

	pthread_create(&tidP, NULL, producer, NULL);
	fprintf(stdout, "Producer started (TID: %ld) ...\n", tidP);
	pthread_create(&tidC, NULL, consumer, NULL);
	fprintf(stdout, "Consumer started (TID: %ld) ...\n", tidC);

	pthread_join(tidP, NULL);
	fprintf(stdout, "... Producer terminated (TID: %ld)\n", tidP);
	pthread_join(tidC, NULL);
	fprintf(stdout, "... Consumer terminated (TID: %ld)\n", tidC);

	clean();

	pthread_exit(0);
}

void init() {
	// Semaphore initialization
	sem_init(normalEmpty, 0, BUFFER_SIZE);
	sem_init(normalFull, 0, 0);
	sem_init(urgentEmpty, 0, BUFFER_SIZE);
	sem_init(urgentFull, 0, 0);
	sem_init(globalEmpty, 0, BUFFER_SIZE * 2);
	sem_init(globalFull, 0, 0);

	// Indexes initialization
	normalIn = 0;
	normalOut = 0;
	urgentIn = 0;
	urgentOut = 0;

	// No buffering on stdout
	setbuf(stdout, NULL);

	// Random initialization
	srand((unsigned) time(NULL));
}

void clean() {
	sem_destroy(normalEmpty);
	sem_destroy(normalFull);
	sem_destroy(urgentEmpty);
	sem_destroy(urgentFull);
	sem_destroy(globalEmpty);
	sem_destroy(globalFull);
}

void enqueue(bufferData* buffer, bufferData data, int in) {
	buffer[in] = data;
	in = (in + 1) % BUFFER_SIZE;
}

void dequeue(bufferData* buffer, bufferData* data, int out) {
	*data = buffer[out];
	out = (out + 1) % BUFFER_SIZE;
}

void printData(char* identifier, bufferData data) {
	fprintf(stdout, "(%s) %lld %u\n", identifier, data.ms, data.bufferID);
}

void produce(bufferData* buffer, bufferData data, int in, sem_t* empty, sem_t* full) {
	sem_wait(empty);
	enqueue(buffer, data, in);
	sem_post(full);
}

/*
*	Does not wait on the full semaphore
*/
void consume(bufferData* buffer, bufferData* data, int out, sem_t* empty, sem_t* full) {
	dequeue(buffer, data, out);
	sem_post(empty);
}

void* producer(void* argument) {
	int i, usecs;
	bufferData data;
	for(i = 0; i < ELEMENTS; i++) {
		usecs = rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME + 1) + MIN_SLEEP_TIME;
		usleep(usecs * 1000);

		// Produce data
		data.ms = current_timestamp();
		if(rand() % 101 < p) // Not urgent
			data.bufferID = 0;
		else // Urgent
			data.bufferID = 1;
		printData("P", data);

		sem_wait(globalEmpty);
		if(data.bufferID == 0) // Not urgent
			produce(normalBuffer, data, normalIn, normalEmpty, normalFull);
		else // Urgent
			produce(urgentBuffer, data, urgentIn, urgentEmpty, urgentFull);
		sem_post(globalFull);
	}

	pthread_exit(NULL);
}

void* consumer(void* argument) {
	int i, usecs;
	bufferData data;
	for(i = 0; i < ELEMENTS; i++) {
		usecs = rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME + 1) + MIN_SLEEP_TIME;
		usleep(usecs * 1000);

		sem_wait(globalFull);
		if(sem_trywait(urgentFull) == 0) {
			consume(urgentBuffer, &data, urgentOut, urgentEmpty, urgentFull);
		} else {
			sem_wait(normalFull);
			consume(normalBuffer, &data, normalOut, normalEmpty, normalFull);
		}
		sem_post(globalEmpty);
		printData("C", data);
	}
	pthread_exit(NULL);
}

long long current_timestamp() {
	struct timeval te;
	gettimeofday(&te, NULL); // get current time
	long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
	return milliseconds;
}