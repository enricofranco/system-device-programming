#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define MAX_LENGHT 30
#define MAX_RANDOM_NUMBER 1000
#define DBG_WRITE
#define DBG_READ

int main(int argc, char** argv) {
    int n, k, i, j, m;
    char *directoryName = "data/",
        *fileName = (char *) malloc(MAX_LENGHT * sizeof(char)),
		*command = (char *) malloc(MAX_LENGHT * sizeof(char));
    int fd; // File descriptor
    srand(time(NULL));

    // Check the number of parameters
    if(argc < 3) {
        fprintf(stderr, "Parameters error \n"
            "Correct usage is %s n k \n"
            "n: number of files to create \n"
            "k: maximum random number \n", argv[0]);
        exit(1);
    }

    // Parameters processing
    n = atoi(argv[1]);
    k = atoi(argv[2]);

    // Subdirectory creation
	sprintf(command, "mkdir %s", directoryName);
	if(system(command) != 0) {
		exit(2);
	}

    // Create n files
    for(i = 0; i < n; ++i) {
        int number;
        // Generate filename
        sprintf(fileName, "%sf%02d", directoryName, i);
        fd = open (fileName, O_WRONLY | O_CREAT, 0664);
        fprintf(stdout, "Writing file %s..\n", fileName);
        m = rand() % k + 1;
        // Write a random number of random numbers
        for(j = 0; j < m; ++j) {
            number = rand() % MAX_RANDOM_NUMBER + 1;
#ifdef DBG_WRITE
            fprintf(stdout, "%d\n", number);
#endif
            write(fd, &number, sizeof(int));
        }
        close(fd);
    }

#ifdef DBG_READ
    fprintf(stdout, "\n");
    // Read n files
    for(i = 0; i < n; ++i) {
        int number;
        // Generate filename
        sprintf(fileName, "%sf%02d", directoryName, i);
        fd = open (fileName, O_RDONLY);
        fprintf(stdout, "Reading file %s\n", fileName);
        while(read(fd, &number, sizeof(int)) > 0) {
            fprintf(stdout, "%d\n", number);
        }
        close(fd);
    }
#endif

	free(command);
    free(fileName);

    return 0;
}