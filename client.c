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

	if(connect_to_server(SERVER_ID, argv[1], pipe_user_reading_from_server, pipe_user_writing_to_server) == -1) {
    perror("hohohohooho im gnot a gnelf, im gnot a gnoblin, im a gnome, and you've been gnomed!'");
		exit(-1);
	}

  printf("weird number here: %d", pipe_user_reading_from_server[0]);
	/* -------------- YOUR CODE STARTS HERE -----------------------------------*/

	
	// poll pipe retrieved and print it to sdiout

	// Poll stdin (input from the terminal) and send it to server (child process) via pipe

		
	/* -------------- YOUR CODE ENDS HERE -----------------------------------*/
}

/*--------------------------End of main for the client --------------------------*/


