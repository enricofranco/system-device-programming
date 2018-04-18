#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

typedef struct arg_t {
	int left, right;
} arg_t;

int *f, size;

int openFile(char *filename, int *len);
void createFile(char* filename);
void swap(int i, int j);
void* quicksort_thread(void* argument);
void quicksort(int left, int right);

int main(int argc, char** argv) {
	pthread_t tidLeft, tidRight;
	int fd, len, numberElements;
	arg_t argLeft, argRight;

	// Check the number of parameters
	if(argc < 3) {
		fprintf(stderr, "Parameters error \n"
			"Correct usage is %s <file> <size> \n", argv[0]);
		exit(1);
	}

	size = atoi(argv[2]);
	if(size < 0) {
		fprintf(stderr, "size cannot be negative\n");
		exit(2);
	}

	// No buffering on stdout
	setbuf(stdout, NULL);

	fd = openFile(argv[1], &len);

	f = mmap((caddr_t) 0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	numberElements =  len / sizeof(int);

	printf("File %s mapped in memory\n", argv[1]);
	printf("File contains %d lines\n", numberElements);

	int i;
	printf("Original file:\n");
	for(i = 0; i < numberElements; i++)
		printf("%d; ", f[i]);

	argLeft.left = 0;
	argLeft.right = numberElements/2;
	argRight.left = argLeft.right + 1;
	argRight.right = numberElements - 1;

	argLeft.left = 0;
	argLeft.right = numberElements - 1;

	pthread_create(&tidLeft, NULL, quicksort_thread, (void*) &argLeft);
	pthread_create(&tidRight, NULL, quicksort_thread, (void*) &argRight);

	pthread_join(tidLeft, NULL);
	pthread_join(tidRight, NULL);

	printf("\n\nSorted file:\n");
	for(i = 0; i < numberElements; i++)
		printf("%d; ", f[i]);
	printf("\n");

	close(fd);
	pthread_exit(0);
}

void* quicksort_thread(void* argument) {
	arg_t *arg = (arg_t*) argument;
	int i, j, x;
	if(arg->left >= arg->right) pthread_exit(NULL);
	x = f[arg->left];
	i = arg->left - 1;
	j = arg->right + 1;
	while (i < j) {
		while (f[--j] > x);
		while (f[++i] < x);
		if (i < j)
			swap (i, j);
	}

	pthread_t tidLeft, tidRight;
	arg_t *argLeft = (arg_t*) malloc(sizeof(arg_t)),
		*argRight = (arg_t*) malloc(sizeof(arg_t));
	argLeft->left = arg->left;
	argLeft->right = j;
	argRight->left = j + 1;
	argRight->right = arg->right;

	if(arg->right - arg->left >= size) {
		pthread_create(&tidLeft, NULL, quicksort_thread, (void*) argLeft);
		pthread_create(&tidRight, NULL, quicksort_thread, (void*) argRight);

		pthread_join(tidLeft, NULL);
		pthread_join(tidRight, NULL);
	} else {
		quicksort(argLeft->left, argLeft->right);
		quicksort(argRight->left, argRight->right);
	}

	free(argLeft);
	free(argRight);
	pthread_exit(NULL);
}

void quicksort(int left, int right) {
	int i, j, x;
	if(left >= right) return;
	x = f[left];
	i = left - 1;
	j = right + 1;
	while (i < j) {
		while (f[--j] > x);
		while (f[++i] < x);
		if (i < j)
			swap (i, j);
	}

	quicksort(left, j);
	quicksort(j + 1, right);
	return;
}

void swap(int i, int j) {
	int tmp = f[i];
	f[i] = f[j];
	f[j] = tmp;
}

int openFile(char *filename, int *len) {
	int fd;
	struct stat stat_buf;

	if((fd = open(filename, O_RDWR)) == -1) {
		fprintf(stderr, "Error opening file %s\n", filename);
		exit(2);
	}

	if(fstat(fd, &stat_buf) == -1) {
		fprintf(stderr, "Error getting info of file %s\n", filename);
		exit(3);
	}

	*len = stat_buf.st_size;

	return fd;
}