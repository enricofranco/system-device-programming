#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_LENGHT 1000
#define MAX_SLEEP_TIME 5
#define ARGUMENTS_NUMBER 6
#define OPTION_LENGHT 5
int main(int argc, char** argv) {
    int c, i, aliveProcesses = 0;
    pid_t pid;
    char *directoryName,
        *command = (char*) malloc(BUFFER_LENGHT * sizeof(char)),
        *listingFile = "list.txt",
        *fileName = (char*) malloc(BUFFER_LENGHT * sizeof(char)),
        *outputFile = "all_sorted.txt",
        **arguments = (char**) malloc(ARGUMENTS_NUMBER * sizeof(char*));
    FILE *fp;
    srand(time(NULL));

    // Check the number of parameters
    if(argc < 3) {
        fprintf(stderr, "Parameters error \n"
            "Correct usage is %s C dir \n"
            "C: maximum number of process which can be created \n"
            "dir: directory to process \n", argv[0]);
        exit(1);
    }

    // Parameters processing
    c = atoi(argv[1]);
    directoryName = argv[2];

    // Array initialization
    arguments[0] = (char*) malloc(BUFFER_LENGHT * sizeof(char)); // Process alias
    arguments[1] = (char*) malloc(OPTION_LENGHT * sizeof(char)); // Option 1
    arguments[2] = (char*) malloc(OPTION_LENGHT * sizeof(char)); // Option 2
    arguments[3] = (char*) malloc(BUFFER_LENGHT * sizeof(char)); // Source file
    arguments[4] = (char*) malloc(BUFFER_LENGHT * sizeof(char)); // Destination file
    arguments[5] = NULL; // Terminator

    strcpy(arguments[1], "-n"); // Numeric sort
    strcpy(arguments[2], "-o"); // Overwrite

    // Command preparation
    // List all the files in the directory and save it on a file
    sprintf(command, "ls %s > %s", directoryName, listingFile);
    system(command);

    if((fp = fopen(listingFile, "r")) == NULL) {
        fprintf(stderr, "An error occured while opening file %s\n", listingFile);
        exit(2);
    }

    // Sort each file in a child process
    while(fscanf(fp, "%s", fileName) != EOF) {
        int sleepTime = rand() % MAX_SLEEP_TIME + 1;
        pid = fork();
        if(pid == 0) { // Child process
            fprintf(stdout, "(Child  PID %d) Sorting file %s...\n", getpid(), fileName);
            // Preparation of the argument array
            sprintf(arguments[0], "sort_%s", fileName); // Process alias
            sprintf(arguments[3], "%s%s", directoryName, fileName); // Source file
            sprintf(arguments[4], "%s%s", directoryName, fileName); // Destination file
            sleep(sleepTime);
            // Sort the file
            execv("/usr/bin/sort", arguments);
        } else { // Father process
            sleepTime = rand() % MAX_SLEEP_TIME + 1;
            sleep(sleepTime);
            aliveProcesses++;
            if(aliveProcesses == c) { // Maximum number of processes reached
                pid = wait((int*) 0); // Wait the first process which terminates
                fprintf(stdout, "(Father PID %d) Child process %d collected\n", getpid(), pid);
                aliveProcesses--;
            }
        }
        sleepTime = rand() % MAX_SLEEP_TIME;
        sleep(sleepTime);
    }
    fclose(fp);
	free(fileName);

    // Collect the last process
    pid = wait((int*) 0);
    fprintf(stdout, "(Father PID %d) Child process %d collected\n", getpid(), pid);

    // Final sort
    sprintf(command, "cat %s* | sort -n > %s", directoryName, outputFile);
    system(command);

    for(i = 0; i < ARGUMENTS_NUMBER; ++i) {
        free(arguments[i]);
    }
    free(arguments);
    free(command);

    return 0;
}