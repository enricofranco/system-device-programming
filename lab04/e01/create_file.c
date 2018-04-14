#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_NUMBER	5000
#define MIN_NUMBER	1

int main(int argc, char**argv) {
	int i, fd;
	int elements;

	// Check the number of parameters
	if(argc < 3) {
		fprintf(stderr, "Parameters error \n"
		"Correct usage is %s <file> <number of elements>\n", argv[0]);
		exit(1);
	}

	elements = atoi(argv[2]);
	if(elements <= 0) {
		fprintf(stderr, "<number of elements> must be positive\n");
		exit(1);
	}

	// Random initialization
	srand((unsigned) time(NULL));

	if((fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0664)) == -1) {
		fprintf(stderr, "Error writing file %s\n", argv[1]);
		exit(2);
	}

	for(i = 0; i < elements; i++) {
		int n = rand() % (MAX_NUMBER - MIN_NUMBER + 1) + MIN_NUMBER;
		write(fd, &n, sizeof(int));
	}

	close(fd);
	return 0;
}