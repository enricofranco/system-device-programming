#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#define MAX_SLEEP_TIME 10
#define MIN_SLEEP_TIME 1
#define TRUE 1
#define FALSE 0
#define BUFFER_LENGHT 4096
#define ENDING_TIME 60

void child();
void parent();
void signalHandlerParent(int signo);
void signalHandlerChild(int signo);

int PRINT_LINE = TRUE;
int TERMINATE = FALSE;

int main(int argc, char** argv) {
	FILE* fp;
	pid_t pid;

	// Check the number of parameters
	if(argc < 2) {
		fprintf(stderr, "Parameters error \n"
			"Correct usage is %s filename \n"
			"filename: file to be read by the parent process \n", argv[0]);
		exit(1);
	}

	if((fp = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "An error occured while opening file %s\n", argv[1]);
		exit(2);
	}
	setbuf(stdout, NULL);

	pid = fork();
	if(pid == 0) { // Child process
		child();
	} else { // Parent process
		signal(SIGUSR1, signalHandlerParent);
		signal(SIGUSR2, signalHandlerParent);
		parent(fp);
		fclose(fp);
		pid = wait((int*) 0);
		fprintf(stdout, "(Father PID %d) Child process %d collected\n", getpid(), pid);
	}

	return 0;
}

void child() {
	int sleepTime;
	srand(time(NULL));
	signal(SIGUSR2, signalHandlerChild);
	if(fork() == 0) { // Child process
		sleep(ENDING_TIME);
	} else { // Parent process
		while(TERMINATE == FALSE) {
			sleepTime = rand() % MAX_SLEEP_TIME + MIN_SLEEP_TIME;
			sleep(sleepTime);
			kill(getppid(), SIGUSR1);
		}
	}
	kill(getppid(), SIGUSR2);
	return;
}

void parent(FILE* fp) {
	int i;
	char* line = (char*) malloc(BUFFER_LENGHT * sizeof(char));
	while(TERMINATE == FALSE) {
		i = 1;
		while(fgets(line, BUFFER_LENGHT, fp) != NULL && TERMINATE == FALSE) {
	    	if(PRINT_LINE == TRUE)
				fprintf(stdout, "#%03d %s", i, line);
			i++;
		}
		rewind(fp);
	}
	free(line);
	return;
}

void signalHandlerParent(int signo) {
	if(signo == SIGUSR1) {
		PRINT_LINE ^= TRUE; // XOR
	} else if(signo == SIGUSR2) {
		TERMINATE = TRUE;
	}
	signal(signo, signalHandlerParent);
	return;
}

void signalHandlerChild(int signo) {
	if(signo == SIGUSR2) {
		TERMINATE = TRUE;
	}
	signal(signo, signalHandlerParent);
	return;
}