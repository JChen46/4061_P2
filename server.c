#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "comm.h"
#include "util.h"

#define SERVER_ID "yeet"

/* -----------Functions that implement server functionality -------------------------*/

/*
 * Returns the empty slot on success, or -1 on failure
 */
int find_empty_slot(USER * user_list) {
	// iterate through the user_list and check m_status to see if any slot is EMPTY
	// return the index of the empty slot
    int i = 0;
	for(i=0;i<MAX_USER;i++) {
    	if(user_list[i].m_status == SLOT_EMPTY) {
			return i;
		}
	}
	return -1;
}

/*
 * list the existing users on the server shell
 */
int list_users(int idx, USER * user_list)
{
	// iterate through the user list
	// if you find any slot which is not empty, print that m_user_id
	// if every slot is empty, print "<no users>""
	// If the function is called by the server (that is, idx is -1), then printf the list
	// If the function is called by the user, then send the list to the user using write() and passing m_fd_to_user
	// return 0 on success
	int i, flag = 0;
	char buf[MAX_MSG] = {}, *s = NULL;

	/* construct a list of user names */
	s = buf;
	strncpy(s, "---connected user list---\n", strlen("---connected user list---\n"));
	s += strlen("---connected user list---\n");
	for (i = 0; i < MAX_USER; i++) {
		if (user_list[i].m_status == SLOT_EMPTY)
			continue;
		flag = 1;
		strncpy(s, user_list[i].m_user_id, strlen(user_list[i].m_user_id));
		s = s + strlen(user_list[i].m_user_id);
		strncpy(s, "\n", 1);
		s++;
	}
	if (flag == 0) {
		strcpy(buf, "<no users>\n");
	} else {
		s--;
		strncpy(s, "\n\n\0", 3);
	}

	if(idx < 0) {
		printf("%s", buf);
		printf("\n");
	} else {
		printf("\n");
		/* write to the given pipe fd */
		if (write(user_list[idx].m_fd_to_user, buf, strlen(buf) + 1) < 0)
			perror("writing to server shell");
	}

	return 0;
}

/*
 * add a new user
 */
int add_user(int idx, USER * user_list, int pid, char * user_id, int pipe_to_child, int pipe_to_parent) //server to child[1], child to server[0]
{
	// populate the user_list structure with the arguments passed to this function
	user_list[idx].m_pid = pid;
	strcpy(user_list[idx].m_user_id, user_id);
	user_list[idx].m_fd_to_user = pipe_to_child;
	user_list[idx].m_fd_to_server = pipe_to_parent;
	user_list[idx].m_status = SLOT_FULL;
	// return the index of user added
	return idx;
}

/*
 * Kill a user
 */
void kill_user(int idx, USER * user_list) {
	// kill a user (specified by idx) by using the systemcall kill()
	// then call waitpid on the user
	kill(user_list[idx].m_pid, SIGTERM);
	int status;
	waitpid(user_list[idx].m_pid, &status, 0);
	if (!WIFEXITED(status)) {
		perror("user died incorrectly");
	}
}

/*
 * Perform cleanup actions after the used has been killed
 */
void cleanup_user(int idx, USER * user_list)
{
	// m_pid should be set back to -1
	user_list[idx].m_pid = -1;
	// m_user_id should be set to zero, using memset()
	memset(user_list[idx].m_user_id, 0, MAX_USER_ID);
	// close all the fd
	close(user_list[idx].m_fd_to_user);
	close(user_list[idx].m_fd_to_server);
	// set the value of all fd back to -1
	user_list[idx].m_fd_to_user = -1;
	user_list[idx].m_fd_to_server = -1;
	// set the status back to empty
	user_list[idx].m_status = SLOT_EMPTY;
}

/*
 * Kills the user and performs cleanup
 */
void kick_user(int idx, USER * user_list) {
	// should kill_user()
	// then perform cleanup_user()
	kill_user(idx, user_list);
	cleanup_user(idx, user_list);
}

/*
 * broadcast message to all users
 */
int broadcast_msg(USER * user_list, char *inbuf, char *sender)
{
	//iterate over the user_list and if a slot is full, and the user is not the sender itself,
	//then send the message to that user
	//return zero on success
	int i;
	for (i = 0; i < MAX_USER; i++) {
		char buf[MAX_MSG + MAX_USER_ID + 2];
		if (user_list[i].m_status == SLOT_FULL && strcmp(user_list[i].m_user_id, sender)) {
			strcpy(buf, sender);
			strcat(buf, ": ");
			strcat(buf, inbuf);
			if (write(user_list[i].m_fd_to_user, buf, MAX_MSG) == -1) {
				fprintf(stderr, "failed to broadcast message to %s", user_list[i].m_user_id);
			}
		}
	}
	return 0;
}

//admin broadcasts to all users
int admin_broadcast(USER * user_list, char *inbuf)
{
	//server broadcasts message to all users
	//return zero on success
	int i;
	for (i = 0; i < MAX_USER; i++) {
		char buf[MAX_MSG + 8];
		if (user_list[i].m_status == SLOT_FULL) {
			strcpy(buf, "Notice: ");
			strcat(buf, inbuf);
			if (write(user_list[i].m_fd_to_user, buf, MAX_MSG) == -1) {
				fprintf(stderr, "failed to broadcast message to %s", user_list[i].m_user_id);
			}
		}
	}
	return 0;
}

/*
 * Cleanup user chat boxes
 */
void cleanup_users(USER * user_list)
{
	int i;
	// go over the user list and check for any empty slots
	for (i = 0; i < MAX_USER; i++) {
		if (user_list[i].m_status == SLOT_EMPTY) {
			// call cleanup user for each of those users.
			cleanup_user(i, user_list);
		}
	}
	printf("cleaned up user list");
}

/*
 * find user index for given user name
 */
int find_user_index(USER * user_list, char * user_id)
{
	// go over the  user list to return the index of the user which matches the argument user_id
	// return -1 if not found
	int i, user_idx = -1;

	if (user_id == NULL) {
		fprintf(stderr, "NULL name passed.\n");
		return user_idx;
	}
	for (i=0;i<MAX_USER;i++) {
		if (user_list[i].m_status == SLOT_EMPTY)
			continue;
		if (strcmp(user_list[i].m_user_id, user_id) == 0) {
			return i;
		}
	}

	return -1;
}

/*
 * given a command's input buffer, extract name
 */
int extract_name(char * buf, char * user_name)
{
	char inbuf[MAX_MSG];
    char * tokens[16];
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 2) {
        strcpy(user_name, tokens[1]);
        return 0;
    }

    return -1;
}

int extract_text(char *buf, char * text)
{
    char inbuf[MAX_MSG];
    char * tokens[16];
    char * s = NULL;
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 3) {
        //Find " "
        s = strchr(buf, ' ');
        s = strchr(s+1, ' ');

        strcpy(text, s+1);
        return 0;
    }

    return -1;
}

/*
 * send personal message
 */
void send_p2p_msg(int idx, USER * user_list, char *buf)
{
	char target_name[MAX_USER_ID];
	char p2p_message[MAX_MSG];
	char final_msg[MAX_MSG];
	char second_msg[MAX_MSG] = " whispers to you: ";
	// get the target user by name using extract_name() function
	if ((extract_name(buf, target_name)) == -1) {
		perror("Unable to extract username from buffer");
	}
	if (extract_text(buf, p2p_message) == -1) {
		perror("Unable to extract text");
	}
	strcpy(final_msg, user_list[idx].m_user_id);
	strcat(final_msg, strcat(second_msg, p2p_message)); //put p2p message into final_msg
	// find the user id using find_user_index()
	int user_id;
	// if user not found, write back to the original user "User not found", using the write()function on pipes.
	if ((user_id = find_user_index(user_list, target_name)) == -1) {
		write(user_list[idx].m_fd_to_user, "User not found", MAX_MSG);
	}
	// if the user is found then write the message that the user wants to send to that user.
	write(user_list[user_id].m_fd_to_user, final_msg, MAX_MSG);
}

//takes in the filename of the file being executed, and prints an error message stating the commands and their usage
void show_error_message(char *filename)
{
}


/*
 * Populates the user list initially
 */
void init_user_list(USER * user_list) {

	//iterate over the MAX_USER
	//memset() all m_user_id to zero
	//set all fd to -1
	//set the status to be EMPTY
	int i=0;
	for(i=0;i<MAX_USER;i++) {
		user_list[i].m_pid = -1;
		memset(user_list[i].m_user_id, '\0', MAX_USER_ID);
		user_list[i].m_fd_to_user = -1;
		user_list[i].m_fd_to_server = -1;
		user_list[i].m_status = SLOT_EMPTY;
	}
}

/* ---------------------End of the functions that implementServer functionality -----------------*/

/*
 * Function that each child process loops in
 */
int child_IPC(int server_to_child[2], int child_to_server[2], int child_to_user[2], int user_to_child[2]) {
	close(server_to_child[1]);
	close(child_to_server[0]);
	close(child_to_user[0]);
	close(user_to_child[1]);
	while (1) {
		char buf[MAX_MSG];
		if ((read(server_to_child[0], buf, MAX_MSG)) > 0) {
			if(write(child_to_user[1], buf, MAX_MSG) == -1) {
				perror("problem in child: writing to user");
			}
		}
		if ((read(user_to_child[0], buf, MAX_MSG)) > 0) {
			if(write(child_to_server[1], buf, MAX_MSG) == -1) {
				perror("problem in child: writing to server");
			}
		}

		usleep(100000);
	}
	return 0;
}

/* ---------------------Start of the Main function ----------------------------------------------*/
int main(int argc, char * argv[])
{
	int nbytes;
	setup_connection(SERVER_ID); // Specifies the connection point as argument.

	USER user_list[MAX_USER];
	init_user_list(user_list);   // Initialize user list

	char buf[MAX_MSG];
	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
	print_prompt("admin");

	//
	while (1) {
		/* ------------------------YOUR CODE FOR MAIN--------------------------------*/

		// Handling a new connection using get_connection
		int user_to_child[2];
		int child_to_user[2];
		char user_id[MAX_USER_ID];
		if (get_connection(user_id, child_to_user, user_to_child) != -1) {
			printf(" * %s connected\n", user_id);
			print_prompt("admin");
			fcntl(user_to_child[0], F_SETFL, fcntl(user_to_child[0], F_GETFL, 0) | O_NONBLOCK); //makes pipes nonblocking
			fcntl(child_to_user[0], F_SETFL, fcntl(child_to_user[0], F_GETFL, 0) | O_NONBLOCK);
			// check if user_id already exists in user_list
			int user_exists = 0, i;
			for (i = 0; i < MAX_USER; i++) {
				if (strcmp(user_list[i].m_user_id, user_id) == 0) {
					user_exists = 1;
					break;
				}
			}

			// search for empty slot, otherwise new_user_idx == -1
			int new_user_idx = -1;
			for (i = 0; i < MAX_USER; i++) {
				if (user_list[i].m_status == SLOT_EMPTY) {
					new_user_idx = i;
					break;
				}
			}
			if (new_user_idx == -1) {
				perror("too many users");
			}
			else if (user_exists) {
				perror("user already exists");
			}

			// no problems, proceed to fork.
			else {
				int child_to_server[2];
				int server_to_child[2];
				pipe(child_to_server);
				pipe(server_to_child);
				fcntl(child_to_server[0], F_SETFL, fcntl(child_to_server[0], F_GETFL, 0) | O_NONBLOCK); //makes pipes nonblocking
				fcntl(server_to_child[0], F_SETFL, fcntl(server_to_child[0], F_GETFL, 0) | O_NONBLOCK);

				int pid = fork();
				if (pid < 0) {
					//error checking
				}
				else if (pid > 0) {
					close(server_to_child[0]);
					close(child_to_server[1]);
					add_user(new_user_idx, user_list, pid, user_id, server_to_child[1], child_to_server[0]);
				}
				else {
					//child infinite loop
					if (!child_IPC(server_to_child, child_to_server, child_to_user, user_to_child)) {
						perror("child IPC has failed");
					}
				}
			}
		}

		// poll child processes and handle user commands
		int i;
		for (i = 0; i < MAX_USER; i++) {
			if (user_list[i].m_status == SLOT_EMPTY)
				continue;
			char buf[MAX_MSG];
			int oof = read(user_list[i].m_fd_to_server, buf, MAX_MSG);
			if (oof > 0) {
				printf("%s: %s",user_list[i].m_user_id, buf);
				fflush(stdout);
				if (strncmp(buf, "\\list", 5) == 0) {
					list_users(i, user_list);
				}
				else if (strncmp(buf, "\\exit", 5) == 0) {
					kick_user(i, user_list);
				}
				else if (strncmp(buf, "\\p2p", 4) == 0) {
					send_p2p_msg(i, user_list, buf);
				}
				else {
					broadcast_msg(user_list, buf, user_list[i].m_user_id);
				}
			}
		}
		// Poll stdin (input from the terminal) and handle admnistrative command
		int n;
		if ((n = read(0, buf, MAX_MSG)) == -1) {
			if (errno != EAGAIN) {
				perror("error reading user message");
			}
		}
		else {
			buf[n] = '\0';
			if (strncmp(buf, "\\list",5) == 0) {
				list_users(0, user_list);
			}
			else if (strncmp(buf, "\\exit", 5) == 0) {
				int i;
				for (i = 0; i < MAX_USER; i++) {
					kick_user(i, user_list);
				}
			}
			else if (strncmp(buf, "\\kick", 5) == 0) {
				int get_user;
				char kicked_name[MAX_USER_ID];
				if ((get_user = extract_name(buf, kicked_name)) == -1) {
					perror("unable to extract username");
				}
				int kicked_index = find_user_index(user_list, kicked_name);
				kick_user(kicked_index, user_list);
			}
			else {
				int i;
				admin_broadcast(user_list, buf);
			}
			print_prompt("admin");
		}


		
		/*buf[n] = '\0';
		if (write(server_to_child[1], buf, MAX_MSG) == -1) {
			perror("error sending server message to users");
		}
		print_prompt(user_id);*/
		

		usleep(100000);

		/* ------------------------YOUR CODE FOR MAIN--------------------------------*/
	}
}

/* --------------------End of the main function ----------------------------------------*/
