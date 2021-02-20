#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define SIZE 1024

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

int main() {
	/* OPEN USER INFORMATION FILE */
	FILE* fp;
	fp = fopen("USERS.txt", "r");
	if (fp == NULL) {
		printf("[-]Error opening USERS.txt file.\n");
		exit(1);
	}
	
	/* READ USER DATA */
	int num_users = getTotalUsers(fp);
	User users[num_users];
	readUserData(fp, users, num_users);

}