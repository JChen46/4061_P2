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

#define SERVER_ID "yeet"

/* -------------------------Main function for the client ----------------------*/
void main(int argc, char * argv[]) {

	int pipe_user_reading_from_server[2], pipe_user_writing_to_server[2];

	// You will need to get user name as a parameter, argv[1].
	//checks if user id is an argument
	char *user_id = argv[1];
	if (user_id == 0) {
		perror("user_id not specified");
		exit(1);
	}

	if(connect_to_server(SERVER_ID, argv[1], pipe_user_reading_from_server, pipe_user_writing_to_server) == -1) {
    perror("Couldn't connect to server.");
		exit(-1);
	}
	
	/* -------------- YOUR CODE STARTS HERE -----------------------------------*/
	printf("%s >> ", argv[1]);
	
	// poll pipe retrieved and print it to sdiout

	// Poll stdin (input from the terminal) and send it to server (child process) via pipe
	char buf[MAX_MSG];
	fgets(buf, MAX_MSG, stdin);

	

	write(pipe_user_writing_to_server[1], buf, MAX_MSG);

	//chris: poll pipe to server and stdin. if the pipe to the server closes, exit (you've been kicked).
		
	/* -------------- YOUR CODE ENDS HERE -----------------------------------*/
}

/*--------------------------End of main for the client --------------------------*/


