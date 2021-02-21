#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#define SIZE 1024
#define TIMEOUT 0
#define STDIN 0

// Struct to define every user
typedef struct detail {
	char name[SIZE];
	char ip[SIZE];
	int port;
} User;

// Gets all user data into User type array
void readUserData(FILE* fp, User users[], int num_users) {
	char line[SIZE];
	for (int i = 0; i < num_users; i++) {
		fgets(line, SIZE, fp);
		sscanf(line, "%s %s %d", users[i].name, users[i].ip, &users[i].port);
	}
	return;
}

// Gets number of users from the first line of file
int getTotalUsers(FILE* fp) {
	char line[SIZE];
	int res;
	fgets(line, SIZE, fp);
	sscanf(line, "%d", &res);
	return res;
}

// Checks if port entered by user is in user list
char* welcomeAndIdentity(int svrport, User users[], int num_users) {
	printf("Welcome to Peer Chat!\n\nThese are the people available to chat:\n");
	int x = -1;
	for (int i = 0; i < num_users; i++) {
		if (svrport != users[i].port) printf("%s\n", users[i].name);
		else x = i;
	}
	if (x == -1) {
		fprintf(stderr, "[-] Entered port not found in users list.\n");
		exit(1);
	}
	printf("You are %s. Start chatting!\n", users[x].name);
	return users[x].name;
}

char** parseMessage(char* buff) {
	char** res = malloc( sizeof(char*) * 2 );
	res[0] = malloc(SIZE);
	res[1] = malloc(SIZE);
	int len = strlen(buff), x = 0;
	for (int i = 0; i < len; i++) {
		if (buff[i] == '/') {
			res[0][i] = '\0';
			x = i + 1;
		}
		else if (x == 0) res[0][i] = buff[i];
		else res[1][i - x] = buff[i];
	}
	buff[len - x] = '\0';
	return res;
}

// Free memory from memory parsing
void freeMemory(char** detail) {
	free(detail[0]);
	free(detail[1]);
	free(detail);
	return;
}

// Send message to person mentioned
void sendMessageToPerson(char* buff, char* identity, User users[], int num_users) {
	int sockfd, peer_port;
	struct sockaddr_in peeraddr;
	struct hostent* peer;
	char hostname[20];
	bool personExists = false;

	char** detail = parseMessage(buff);
	for (int i = 0; i < num_users; i++) {
		if (strcmp(detail[0], users[i].name) == 0) {
			peer_port = users[i].port;
			strcpy(hostname, users[i].ip);
			personExists = true;
			break;
		}
	}
	if (!personExists) {
		printf("[-] Such a person does not exist.\n[-] Invalid format.\nTry again!\n\n");
		return;
	}

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		fprintf(stderr, "[-] Error in opeing socket.\n");
		exit(1);
	}

	if ( (peer = gethostbyname(hostname)) == NULL ) {
		fprintf(stderr, "[-] Error: No host found for given peer.\n");
		return;
	}

	bzero(&peeraddr, sizeof(peeraddr));
	peeraddr.sin_family = AF_INET;
	bcopy((char*) peer -> h_addr, (char*) &peeraddr.sin_addr.s_addr, peer -> h_length);
	peeraddr.sin_port = htons(peer_port);

	if ( connect(sockfd, (struct sockaddr*) &peeraddr, sizeof(peeraddr)) < 0 ) {
		fprintf(stderr, "[-] Peer is offline.\n\n");
		return;
	}

	char message[2 * SIZE];
	strcpy(message, identity);
	strcat(message, "/");
	strcat(message, detail[1]);

	if ( write(sockfd, message, strlen(message)) < 0 ) {
		fprintf(stderr, "[-] Error writing to socket.\n");
		return;
	}
	freeMemory(detail);
	close(sockfd);
	return;
}

// Function to control chat
void startChat(int svrsock, User users[], int num_users, struct sockaddr_in serveraddr, socklen_t len, char* identity) {
	fd_set masterfds, readfds;
	struct timeval timeout;
	int maxfd, peersock;
	size_t size = SIZE;
	bool exit_flag = false;
	char *buff = malloc(SIZE);
	char** detail;

	FD_ZERO(&masterfds);
	FD_SET(svrsock, &masterfds);
	FD_SET(STDIN, &masterfds);
	maxfd = svrsock;

	while (true) {
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;
		memcpy(&readfds, &masterfds, sizeof(readfds));
		int result = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
 
		if (result == 0 && exit_flag) {
			/* After TIMEOUT amount of time after entering 
			quit/exit as input, the program enters here */
			printf("Do you want to exit application? [Y/N]: ");
			
			getline(&buff, &size, stdin);
			if (!strcmp(buff, "Y\n") || !strcmp(buff, "y\n")) break;
			if (strlen(buff) != 2 || (strcmp(buff, "N\n") && strcmp(buff, "n\n"))) 
				printf("[-] Invalid Character Input. Try quitting again.\n");
			
			exit_flag = false;
			printf("\n");
		} else if (result < 0 && errno != EINTR) {
			/* select() error */
			fprintf(stderr, "[-] Error in select() function\n");
			exit(1);
		} else if (result > 0) {
			/* If Server Socket is ready, accept connection */
			if ( FD_ISSET(svrsock, &readfds) ) {
				peersock = accept(svrsock, (struct sockaddr*) &serveraddr, &len);
				if ( peersock < 0 ) {
					fprintf(stderr, "[-] Error in accept().\n");
				} else {
					FD_SET(peersock, &masterfds);
					maxfd = (maxfd < peersock)? peersock: maxfd;
				}
				// Remove Server Socket from readfds
				FD_CLR(svrsock, &readfds);
			}
			/* If stdin is ready, send message to person, if person exists, else continue */
			if ( FD_ISSET(STDIN, &readfds) ) {
				getline(&buff, &size, stdin);
				printf("\n");
				// If user wants to exit, allow and continue
				if ( !strcmp(buff, "exit\n") || !strcmp(buff, "quit\n") ) {
					exit_flag = true;
					continue;
				}
				sendMessageToPerson(buff, identity, users, num_users);
				continue;
			}
			/* Check for all the other sockets which might be ready to get read from */
			for (int i = 1; i <= maxfd; i++) {
				if ( FD_ISSET(i, &readfds) ) {
					int res = recv(i, buff, SIZE, 0);
					if (res > 0) {
						buff[res] = '\0';
						detail = parseMessage(buff);
						printf("%s: %s\n", detail[0], detail[1]);
					} else if (res == 0) { // EOF
						// Close and clear socket
						close(i);
						FD_CLR(i, &masterfds);
					} else {
						fprintf(stderr, "[-] Error in recv().\n");
						exit(1);
					}
				}
			}
		}
	}
	freeMemory(detail);
	return;
}

int main(int argc, char** argv) {
	/* CHECK NUMBER OF ARGUMENTS */
	if (argc != 2) {
		fprintf(stderr, "USAGE: %s <YOUR PORT>\n", argv[0]);
		exit(1);
	}

	/* OPEN USER INFORMATION FILE */
	FILE* fp;
	fp = fopen("USERS.txt", "r");
	if (fp == NULL) {
		fprintf(stderr, "[-] Error opening USERS.txt file.\n");
		exit(1);
	}
	
	/* READ USER DATA */
	int num_users = getTotalUsers(fp);
	User users[num_users];
	readUserData(fp, users, num_users);
	fclose(fp);

	/*	START APPLICATION SERVER */
	int svrport = atoi(argv[1]);
	int svrsock = socket(AF_INET, SOCK_STREAM, 0);
	if (svrsock < 0) {
		fprintf(stderr, "[-] Error opening server socket.\n");
		exit(1);
	}
	
	struct sockaddr_in serveraddr;
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET; // IPv4 Address Family
	serveraddr.sin_port = htons((unsigned short) svrport); // Set port to bind
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from all IPs of machine
	socklen_t len = sizeof(serveraddr);

	// Bind socket to port
	if ( bind(svrsock, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0 ) {
		fprintf(stderr, "[-] Unsuccessful binding!\n");
		exit(1);
	}

	// Listen for connection requests (MAX = 5)
	if ( listen(svrsock, 5) < 0 ) {
		fprintf(stderr, "[-] Error in listening!\n");
		exit(1);
	}

	printf("\n[+] Server Running! Start Conversation!\n");

	// Check if the port mentioned in arguments exists in user list
	char* identity = welcomeAndIdentity(svrport, users, num_users);

	printf("\nEnter 'quit' or 'exit' for exiting application.\n");

	printf("\nUse the following format for messaging:\nreceiver_name/message\n\n");

	/* START CHAT */
	startChat(svrsock, users, num_users, serveraddr, len, identity);

	return 0;
}