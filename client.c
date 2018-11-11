#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "comm.h"
#include "util.h"

#define SERVER_ID "yeet"

/* -------------------------Main function for the client ----------------------*/
void main(int argc, char * argv[]) {
	int server_to_user[2], user_to_server[2];

	// You will need to get user name as a parameter, argv[1].
	//checks if user id is an argument
	char *user_id = argv[1];
	if (user_id == 0) {
		perror("user_id not specified\n");
		exit(1);
	}

	if(connect_to_server(SERVER_ID, user_id, server_to_user, user_to_server) == -1) {
    perror("Couldn't connect to server.\n");
		exit(-1);
	}
	/* -------------- YOUR CODE STARTS HERE -----------------------------------*/
	close(server_to_user[1]);
	close(user_to_server[0]);
	fcntl(server_to_user[0], F_SETFL, fcntl(server_to_user[0], F_GETFL, 0) | O_NONBLOCK);
	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | O_NONBLOCK);
	print_prompt(user_id);
	while(1) {
		// poll pipe retrieved and print it to sdtout
		char buf[MAX_MSG];
		int readReturn;
		if ((readReturn = read(server_to_user[0], buf, MAX_MSG)) == -1) {
			if (errno != EAGAIN) {
				perror("error reading server message\n");
			}
		}
		else {
			if (readReturn == 0) {
				perror("you've been kicked\n");
				kill(getpid(), SIGINT);
			}
			printf("\n%s", buf);
			print_prompt(user_id);
		}

		// Poll stdin (input from the terminal) and send it to server (child process) via pipe
		int n;
		if ((n = read(0, buf, MAX_MSG)) == -1) {
			if (errno != EAGAIN) {
				perror("error reading user message\n");
			}
		}
		else {
			buf[n] = '\0';
			if (write(user_to_server[1], buf, MAX_MSG) == -1) {
				perror("error sending user message to server\n");
			}
			print_prompt(user_id);
		}
		usleep(100000);

		//chris: poll pipe to server and stdin. if the pipe to the server closes, exit (you've been kicked).
	}
		
	/* -------------- YOUR CODE ENDS HERE -----------------------------------*/
}

/*--------------------------End of main for the client --------------------------*/


