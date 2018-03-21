#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_LENGHT 1000
#define MAX_SLEEP_TIME 5
int main(int argc, char** argv) {
    int c, aliveProcesses = 0;
    pid_t pid;
    char *directoryName,
        *command = (char*) malloc(BUFFER_LENGHT * sizeof(char)),
        *listingFile = "list.txt",
        *fileName = (char*) malloc(BUFFER_LENGHT * sizeof(char)),
        *outputFile = "all_sorted.txt",
        *processAlias = (char*) malloc(BUFFER_LENGHT * sizeof(char)),
        *completePath = (char*) malloc(BUFFER_LENGHT * sizeof(char));
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
            sprintf(completePath, "%s%s", directoryName, fileName);
            sprintf(processAlias, "sort_%s", fileName);
            sleep(sleepTime);
            // Sort the file
            execlp("sort", processAlias, "-n", "-o", completePath, completePath, (char*) 0);
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
    free(processAlias);
    free(completePath);

    // Collect the last process
    pid = wait((int*) 0);
    fprintf(stdout, "(Father PID %d) Child process %d collected\n", getpid(), pid);

    // Final sort
    sprintf(command, "cat %s* | sort -n > %s", directoryName, outputFile);
    system(command);

    free(command);

    return 0;
}