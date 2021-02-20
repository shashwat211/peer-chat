# Peer Chat
A Peer to Peer(P2P) Chat Application in C using TCP Protocols for connections within a defined closed set of users.

## Adding Users
The USERS.txt file defines the closed set of users that you want to chat to. The first line should be the total number of users followed by data of users (IPv4 Address and Port).

## Compilation of Source Code
Compile the source code using the following command:
```
gcc -o app app.c
```
## Running the application
Run the application using the following format:
```
./app <YOUR PORT>
```
### For example:
```
./app 3000
```
```
[+] Server Running! Start Conversation!
Welcome to Peer Chat!

These are the people available to chat:
amish
bhumika
aryan
sunny
You are shashwat. Start chatting!

Enter 'quit' or 'exit' for exiting application.

Use the following format for messaging:
receiver_name/message

```
