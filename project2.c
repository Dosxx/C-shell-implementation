#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LINE 80   /* command line argument maximum length */
#define EXIT "exit"   /* constant exit string */
#define CONCURRENT "&" /* concurrent execution is requested */
#define BUFFER_SIZE 25
#define READ_END 0
#define WRITE_END 1


/* the array to hold user commands */

char cmd_history[100][256];

static void extractCmd(char *user_input, char **arg_list, int *waitMode ) {
	/* takes a char pointer user input and set up the cmd and argument */
	int i;
	char *strings;

	for(i = 0; i < sizeof(*arg_list); i++){

		if(i == 0){
			strings = strtok(user_input, " ");
		}else{
			strings = strtok(NULL, " ");
		}

		if(strings == NULL){
			break;
		}
		else if (strcmp(strings, CONCURRENT) == 0) {
			/* set concurrency flag */
			*waitMode = 0;
			strings = strtok(NULL, " ");
		}
		arg_list[i] = strings;
	}

	arg_list[i] = NULL;
}

static int getCmdIndex(char *string, int Hcounter) {
	/* extract the index of the command requested */
	long index;
	char * ptr = string;
	while (*ptr) {
		if (isdigit(*ptr)) {
			index = strtol(ptr, &ptr, 10);
		} else {
			ptr++;
		}
	}
	if(index > Hcounter) {
		printf("No such command in history.\n");
		return (0);
	}
	
	/* retrieve the command at the index */
	else {
		strcpy(cmd_history[Hcounter], cmd_history[(index-1)]);
		strcpy(string, cmd_history[Hcounter]);
		return(1);
	}
}

int main(void){

	char *args[MAX_LINE/2 + 1];

	/* global variables */

	int should_run = 1;
	int should_wait = 1;
	char cmd[MAX_LINE];
	int len;
	pid_t pid,w;
	int wStatus;
	int historyCounter = 0;

	while(should_run){

		/* Display the shell prompt */

		printf("osh>");
		fflush(stdout);

		/* Get user input from command line */

		fgets(cmd, MAX_LINE, stdin);

		if(strcmp(cmd, "\n") != 0) {
			len = strlen(cmd);
			cmd[len - 1] = 0;
		}
		/* character return entered */
		else {
			do { 
				w = waitpid(-1, &wStatus, WNOHANG);
				if(w == -1)
					break;
				else if(w == 0) 
					printf("Process ID %d is still running\n", w );
				else
					if(WIFEXITED(wStatus)) {
						printf("Process ID %d Terminate normally with status [%d]\n",
								w, WEXITSTATUS(wStatus));
					}
			} while(w == 0);
			continue;
		}
		
		/* validate the input command */
		if(strcmp(cmd, "!!") == 0 ) {
			if(historyCounter > 0) {
				strcpy(cmd_history[(historyCounter)], cmd_history[(historyCounter-1)]);
				strcpy(cmd, cmd_history[historyCounter]);
			}
			else {
				printf("No commands in history.\n" );
				continue;
			}
		}
		else if(cmd[0] == '!'){
			/* extract the index of the command requested */
			if(!getCmdIndex(cmd, historyCounter))
				continue;
		}
		else
			/* save the entered command to history */
			strcpy(cmd_history[historyCounter], cmd);

		/* set up the arguments for execution */
		extractCmd(cmd, args, &should_wait);

		/* Terminate the loop when the first args is "exit" */
		if(strcmp(args[0], EXIT) == 0){
			should_run = 0;
			continue;
		}

		/* Step #1 Fork a child process to execute the user command */
		pid = fork();

		/* check for a sucessfull fork */
		if(pid < 0) {
			 perror("fork");
			 exit(EXIT_FAILURE);
		}

		/* child process */
		if(pid == 0) {
			/* Execute the history command */
			if (strcmp(args[0], "history") == 0 ) {
				int count = 0;
				for (int i = (historyCounter + 1); i > 0; i--) {
					count++;
					/* only the last 10 commands are printed */
					if(count <= 10)
						printf("%d %s\n",(i), cmd_history[i-1]);
				}
				/* Terminate the process when finished */
				exit(EXIT_SUCCESS);
			}
			/* Step #2 Execute the user command */
			else {
				if((execvp(args[0], args)) == -1) {
					int errnumb = errno;
					/* print error message if the command failled */
					perror(strerror(errnumb));
					exit(EXIT_FAILURE);
				}
			}
		}
		/* parent process */
		else if(pid > 0) 
		{
			/* Step #3 wait for child process if not in concurrent mode */
			if(should_wait) {
				while(1) {
					pid = wait(&wStatus);
					if(pid < 0)
						perror("wait error");
					else if(WIFEXITED(wStatus)) {
						break;
					}
				}
			}

			should_wait = 1; /* reset wait mode */
			historyCounter++; /* increament the history counter */
		}
	}
	return 0;
}