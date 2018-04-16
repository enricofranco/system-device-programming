#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_SLEEP_TIME	500	/* milliseconds */
#define MIN_SLEEP_TIME	0	/* milliseconds */

void* reader(void* argument);
void* writer(void* argument);
long long current_timestamp();

typedef struct arg_t {
	float *v1, *v2, **m;
	float *intermediateArray;
	int size;
	int index;
} arg_t;

int counter = 0;
pthread_mutex_t mutex;
float result;

float *newArray(int size);
float* newEmptyArray(int size);
float **newMatrix(int rows, int columns);
void printArray(float *v, int size);
void printMatrix(float **v, int rows, int columns);
void* concurrentProduct(void* argument);

int main(int argc, char** argv) {
	int k, i;
	long tid;
	pthread_t *tids;

	// Check the number of parameters
	if(argc < 2) {
		fprintf(stderr, "Parameters error \n"
			"Correct usage is %s <k> \n"
			"<k> number of rows/columns of the matrix\n", argv[0]);
		exit(1);
	}

	k = atoi(argv[1]);
	if(k <= 0) {
		fprintf(stderr, "Invalid number. <k> must be positive\n");
		exit(2);
	}

	setbuf(stdout, NULL);			// No buffering on stdout
	srand((unsigned) time(NULL));	// Random initialization

	float *v1 = newArray(k),
		*v2 = newArray(k),
		**mat = newMatrix(k, k),
		*intermediateArray = newEmptyArray(k);

	tids = (pthread_t*) malloc(2 * sizeof(pthread_t));
	arg_t* args = (arg_t*) malloc(k * sizeof(arg_t));

	printf("Array v1: ");
	printArray(v1, k);
	printf("Array v2: ");
	printArray(v2, k);
	printf("Matrix mat:\n");
	printMatrix(mat, k, k);

	for(i = 0; i < k; i++) {
		args[i].v1 = v1;
		args[i].v2 = v2;
		args[i].m = mat;
		args[i].index = i;
		args[i].intermediateArray = intermediateArray;
		args[i].size = k;
		pthread_create(&tids[i], NULL, concurrentProduct, (void*) &args[i]);
		printf("Thread %ld started on index %d\n", tids[i], i);
		tid++;
	}

	for(i = 0; i < k; i++) {
		pthread_join(tids[i], NULL);
		printf("Thread %ld collected\n", tids[i]);
	}

	printf("Array intermediate: ");
	printArray(intermediateArray, k);

	printf("Final result: %.4f\n", result);

	free(tids);
	free(v1);
	free(v2);
	free(intermediateArray);
	for(i = 0; i < k; i++)
		free(mat[i]);
	free(mat);

	pthread_exit(0);
}

float* newArray(int size) {
	float *v = (float*) malloc(size * sizeof(int));
	int i;
	for(i = 0; i < size; i++) {
		v[i] = (float) rand() / RAND_MAX - 0.5;
	}
	return v;
}

float* newEmptyArray(int size) {
	float *v = (float*) malloc(size * sizeof(int));
	int i;
	for(i = 0; i < size; i++) {
		v[i] = 0.0;
	}
	return v;
}

float** newMatrix(int rows, int columns) {
	float **v = (float**) malloc(rows * sizeof(float*));
	int i;
	for(i = 0; i < rows; i++)
		v[i] = newArray(columns);
	return v;
}

void printArray(float *v, int size) {
	int i;
	for(i = 0; i < size; i++)
		printf("%.4f ", v[i]);
	printf("\n");
}

float scalarProduct(float *v, float *w, int size) {
	float result = 0.0;
	int i;
	for(i = 0; i < size; i++)
		result += (v[i] * w[i]);
	return result;
}

void* concurrentProduct(void* argument) {
	arg_t* arg = (arg_t*) argument;
	int i = arg->index;
	arg->intermediateArray[i] = scalarProduct(arg->v2, arg->m[i], arg->size);

	int usecs = rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME + 1) + MIN_SLEEP_TIME;
	usleep(usecs * 1000);

	pthread_mutex_lock(&mutex);
	counter++;
	if(counter < arg->size) {
		pthread_mutex_unlock(&mutex);
		printf("Thread %d terminated\n", i);
		pthread_exit(NULL);
	}

	/* Here, last thread */
	result = scalarProduct(arg->v1, arg->intermediateArray, arg->size);

	pthread_mutex_unlock(&mutex);
	printf("Thread %d terminated\n", i);
	pthread_exit(NULL);
}

void printMatrix(float **v, int rows, int columns) {
	int i;
	for(i = 0; i < rows; i++)
		printArray(v[i], columns);
	printf("\n");
}