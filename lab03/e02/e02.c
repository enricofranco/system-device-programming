#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_SLEEP_TIME	500	/* milliseconds */
#define MIN_SLEEP_TIME	0	/* milliseconds */
#define READING_TIME	500	/* milliseconds */
#define WRITING_TIME	500	/* milliseconds */

void* reader(void* argument);
void* writer(void* argument);
long long current_timestamp();

pthread_mutex_t meR, meW, w;
int nR = 0;

int main(int argc, char** argv) {
	int n, i;
	long tid;
	pthread_t *readersTid, *writersTid;

	// Check the number of parameters
	if(argc < 2) {
		fprintf(stderr, "Parameters error \n"
			"Correct usage is %s <n> \n"
			"<n> number of concurrent readers and writers\n", argv[0]);
		exit(1);
	}

	n = atoi(argv[1]);
	if(n <= 0) {
		fprintf(stderr, "Invalid number. <n> must be positive\n");
		exit(2);
	}

	setbuf(stdout, NULL);			// No buffering on stdout
	srand((unsigned) time(NULL));	// Random initialization
	readersTid = (pthread_t*) malloc(n * sizeof(pthread_t));
	writersTid = (pthread_t*) malloc(n * sizeof(pthread_t));

	pthread_mutex_init(&meR, NULL);
	pthread_mutex_init(&meW, NULL);
	pthread_mutex_init(&w, NULL);
	tid = 1;

	for(i = 0; i < n; i++) {
		pthread_create(&readersTid[i], NULL, reader, (void*) tid);
		tid++;
		pthread_create(&writersTid[i], NULL, writer, (void*) tid);
		tid++;
	}

	for(i = 0; i < n; i++) {
		pthread_join(readersTid[i], NULL);
		pthread_join(writersTid[i], NULL);
	}

	pthread_mutex_destroy(&meR);
	pthread_mutex_destroy(&meW);
	pthread_mutex_destroy(&w);
	free(readersTid);
	free(writersTid);

	pthread_exit(0);
}

void* reader(void* argument) {
	long tid = (long) argument;
	int usecs = rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME + 1) + MIN_SLEEP_TIME;
	usleep(usecs);
	fprintf(stdout, "Thread %02ld trying to read at time %lld\n", tid, current_timestamp());

	pthread_mutex_lock(&meR);
		nR++;
		if(nR == 1) {
			pthread_mutex_lock(&w);
		}
	pthread_mutex_unlock(&meR);

	// Read operation
	fprintf(stdout, "Thread %02ld reading at time %lld\n", tid, current_timestamp());
	usleep(READING_TIME * 1000);

	pthread_mutex_lock(&meR);
		nR--;
		if(nR == 0) {
			pthread_mutex_unlock(&w);
		}
	pthread_mutex_unlock(&meR);

	pthread_exit(NULL);
}

void* writer(void* argument) {
	long tid = (long) argument;
	int usecs = rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME + 1) + MIN_SLEEP_TIME;
	usleep(usecs);
	fprintf(stdout, "Thread %02ld trying to write at time %lld\n", tid, current_timestamp());

	pthread_mutex_lock(&meW);
		pthread_mutex_lock(&w);
		// condition

		// Write operation
		fprintf(stdout, "Thread %02ld writing at time %lld\n", tid, current_timestamp());
		usleep(WRITING_TIME * 1000);

		// condition
		pthread_mutex_unlock(&w);
	pthread_mutex_unlock(&meW);

	pthread_exit(NULL);
}

long long current_timestamp() {
	struct timeval te;
	gettimeofday(&te, NULL); // get current time
	long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
	return milliseconds;
}