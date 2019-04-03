/*
  Author:        Christopher J. Kelly
  Major:         CS - Software Dev
  Creation Date: 11/13/2018
  Due Date:      12/5/2018
  Course:        CSC 328-20
  Professor:     Frye
  Assignment:    Final Project: Download Server
  Filename:      ckelly_client.c
  Language:      C - gcc 4.4.7
  Compilation:   gcc ckelly_client.c -o C
  Executable:    ./C <Hostname> [<Port Number>]
                 ./C acad.kutztown.edu 34567
  Purpose:       Client program for a download server. Connects to the host and
                 sends validated commands to it.
*/
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>

int serverConnect(int);
void talkToServer(int);
int validateInput(char *);
void pwd(int);
void cd(int);
void dir(int);
void download(char *, int);

#define DEFAULT_PORT 25821
#define END_OF_DATA "))!_!(("

char *hostname, directory[PATH_MAX]; //will be displayed after every command

int main(int argc, char *argv[]) {
  if(argc != 2 && argc != 3) {
    printf("Usage: %s <Hostname> [<Port>]\n", argv[0]);
    exit(-1);
  }

  int port, sockfd;
  hostname = argv[1];
  strcpy(directory, "~");
  
  if(argc == 3) //use provided port
    port = atoi(argv[2]);
  else
    port = DEFAULT_PORT;
  
  sockfd = serverConnect(port);
  talkToServer(sockfd);

  return 0;
} //end main()


/*
  Function Name: serverConnect
  Description:   Connects to the given server
  Parameters:    int port - port # to connect to
  Return Value:  int - file descriptor of socket
*/
int serverConnect(int port) {
  struct sockaddr_in server;
  struct hostent *he;
  struct in_addr **addr_list;
  char *ipaddr;
  int sockfd;
  
  if((he = gethostbyname(hostname)) == NULL) {
    herror("Error resolving hostname");
    exit(-1);
  } //end if

  addr_list = (struct in_addr **) he->h_addr_list;
      
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr = *addr_list[0];

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { //create socket
    perror("Socket failed");
    close(sockfd);
    exit(-1);
  } //end if

  ipaddr = inet_ntoa(*addr_list[0]);
  printf("Connecting to %s [%s]...\n\n", hostname, ipaddr);
  if(connect(sockfd, (struct sockaddr *)&server,
	     sizeof(struct sockaddr_in)) == -1) { //create connection
    perror("Connection failed");
    close(sockfd);
    exit(-1);
  } //end if

  //recieve the inital HELLO message from the server
  char rec[5];
  if((recv(sockfd, rec, 5, 0) > 0)/* && (strcmp("HELLO", rec) == 0)*/) { 
    printf("Successful Connection. Enter a command.\n");
    printf("Commands: PWD, CD <[Directory]>, DIR <[Directory]> (or LS), ");
    printf("DOWNLOAD [File], EXIT\n\n");
    
  } else {
    printf("Server not responding correctly. Closing...\n");
    close(sockfd);
    exit(-1);
  } //end if-else

  return sockfd;
} //end serverConnect


/*
  Function Name: talkToServer
  Description:   Sends the user's input to the server and acts on the response
  Parameters:    int sockfd - socket fd for the server
  Return Value:  void
*/
void talkToServer(int sockfd) {
  char input[256], rec[256];
  memset(rec, 0, sizeof(rec));
  
  for(;;) {
    memset(rec, 0, sizeof(rec));
    if(validateInput(input)) { //if valid command
      send(sockfd, input, 256, 0); //send message
      
      if(recv(sockfd, rec, 256, 0) > 0) {
	strtok(input, " \n"); //splits 'input' into tokens
	char *token = strtok(NULL, " \n");
	//checks if server received a valid command
	//if server responds with "OK_***", it is ready to send data
	if(strcmp(rec, "EXIT") == 0) {
	  printf("\n----You have disconnected from the server----\n\n");
	  close(sockfd);
	  exit(-1);

	} else if(strcmp(rec, "ERR_NF") == 0) { //validateInput prevents this
	  printf("Invalid command was sent to the server\n");

	} else if(strcmp(rec, "OK_PWD") == 0) {
	  pwd(sockfd);
	  
	} else if(strcmp(rec, "OK_CD") == 0) {
	  cd(sockfd);
	  
       	} else if(strcmp(rec, "OK_DIR") == 0) {
	  dir(sockfd);
	  
	} else if(strcmp(rec, "OK_DOWNLOAD") == 0) {
	  download(token, sockfd);

	} //end if-else	strcmp

      } else { //recv failed
	printf("Server stopped responding. Closing...\n");
	close(sockfd);
	exit(-1);
      } //end if-else recv

    } //end if validateInput

  } //end for

} //end talkToServer()


/*
  Function Name: validateInput
  Description:   Reads user input from keyboard and validates the command
  Parameters:    char *input - string to write input to
  Return Value:  int - 0/1 if command is invalid/valid
*/
int validateInput(char *input) {
  char *token, *arg2, *arg3, tmp[256];
  printf("[%s %s]$ ", hostname, directory);
  fgets(input, 256, stdin); //read input from user
  
  if(strcmp(input, "\n") == 0 || *input == ' ') { //prevent crash if empty
    printf("Error: Invalid Command. For help, type HELP\n");
    return 0;
  } //end if

  strcpy(tmp, input); //otherwise strtok messes up 'input' too early
  token = strtok(tmp, " \n"); //gets first argument
  arg2 = strtok(NULL, " \n"); //gets 2nd (if it exists)
  arg3 = strtok(NULL, " \n");
  
  int i;
  for(i=0; i < strlen(token); i++) //convert to uppercase
    token[i] = toupper(token[i]);

  if(strcmp(token, "PWD") == 0) {
    if(arg2 != NULL) { //if an argument was given
      printf("Usage: PWD\n");
      return 0;
    } //end if

  } else if(strcmp(token, "CD") == 0) {
    if((arg2 != NULL) && (arg3 != NULL)) { //if more than 1 argument
      printf("Usage: CD [<Directory>]\n");
      return 0;
    } //end if

  } else if((strcmp(token, "DIR") == 0) || (strcmp(token, "LS") == 0)) {
    if(arg2 != NULL && arg3 != NULL) { //if more than 1 argument
      printf("Usage: DIR [<Directory>]\n");
      return 0;
    } //end if

  } else if(strcmp(token, "EXIT") == 0){
    printf("Disconnecting...\n");
    
  } else if(strcmp(token, "DOWNLOAD") == 0) {
    if(arg2 == NULL || arg3 != NULL) {       //if 0 args or more than 1 arg
      printf("Usage: DOWNLOAD <File>\n");
      return 0;
    } //end if

  } else if(strcmp(token, "HELP") == 0) {
    printf("Commands: PWD, CD <[Directory]>, DIR <[Directory]> (or LS), ");
    printf("DOWNLOAD [File], EXIT\n");
    return 0;
    
  } else {
    printf("Error: Invalid Command. For help, type HELP\n");
    return 0; //command not valid
  } //end if-else

  return 1;
} //end getInput()


/*
  Function Name: pwd
  Description:   Executes 'print directory' command and receives data from server
  Parameters:    int sockfd  - socket FD for server
  Return Value:  void
*/
void pwd(int sockfd) {
  char buffer[PATH_MAX]; //PATH_MAX is max size a directory name can be
  memset(buffer, 0, sizeof(buffer));
  recv(sockfd, buffer, PATH_MAX, 0);

  if(strcmp(buffer, "ERR_PWD") != 0) {
    printf("Current Directory: %s\n", buffer);
  } else {
    send(sockfd, "OK", 2, 0);
    memset(buffer, 0, 256);
    recv(sockfd, buffer, 256, 0);
    printf("Error printing directory: %s\n", buffer);
  } //end if-else
  
} //end pwd


/*
  Function Name: cd
  Description:   Executes 'change directory' command & receives data from server
  Parameters:    int sockfd  - socket FD for server
  Return Value:  void
*/
void cd(int sockfd) {
  char rec[PATH_MAX]; //PATH_MAX is max length a directory name can be
  memset(rec, 0, PATH_MAX);
  recv(sockfd, rec, PATH_MAX, 0);

  if(strcmp(rec, "ERR_CD") != 0) { //successful cd
    memset(directory, 0, PATH_MAX);
    strcpy(directory, rec); //indicate to client directory changed

  } else {
    send(sockfd, "OK", 2, 0);
    memset(rec, 0, 256);
    recv(sockfd, rec, 256, 0);
    printf("Error changing directory: %s\n", rec);
  } //end if-else

} //end cd


/*
  Function Name: dir
  Description:   Executes 'Directory List' command & receives data from server
  Parameters:    char *input - user's command
                 int sockfd  - socket FD for server
  Return Value:  void
*/
void dir(int sockfd) {
  char rec[NAME_MAX]; //NAME_MAX is the largest filename length

  for(;;) {
    memset(rec, 0, 256);
    recv(sockfd, rec, NAME_MAX, 0);

    if(strcmp(rec, "ERR_DIR") == 0) { //error reading directory
      send(sockfd, "OK", 2, 0);
      memset(rec, 0, 256);
      recv(sockfd, rec, 256, 0);
      printf("Error reading directory: %s\n", rec);
      return;

    } else if(strcmp(rec, "END_DIR") == 0) {
      return;

    } else {
      printf("%s\n", rec);
      send(sockfd, "OK", 2, 0); //ready to receive next file
    } //end if-else
  } //end for

} //end dir


/*
  Function Name: download
  Description:   Executes DOWNLOAD command and receives data from server
  Parameters:    char *input - user's filename
                 int sockfd  - socket file descriptor of server
  Return Value:  void
*/
void download(char *input, int sockfd) {
  char rec[256];
  int fd;

  memset(rec, 0, 256);
  if(access(input, F_OK) == 0) { //if file exists
    char answer[256];
    printf("File already exists. Overwrite? (y/n): ");
    fgets(answer, sizeof(answer), stdin);
    answer[0] = tolower(answer[0]);

    if(answer[0] == 'y') {
      send(sockfd, "OK", 2, 0); //ready to start download
    } else if(answer[0] == 'n') {
      printf("Canceling download\n");
      send(sockfd, "STOP", 4, 0);
      return;
    } else {
      printf("Error: Invalid Input. Canceling download\n");
      send(sockfd, "STOP", 4, 0);
      return;
    } //end if-else

  } else {
    send(sockfd, "OK", 2, 0); //file doesnt exist already
  } //end if-else access

  memset(rec, 0, 256);
  recv(sockfd, rec, 256, 0);
  if(strcmp(rec, "ERR_DOWNLOAD") == 0) { //checks if server can open file
    send(sockfd, "OK", 2, 0);
    memset(rec, 0, 256);
    recv(sockfd, rec, 256, 0);
    printf("Error downloading file: %s\n", rec);
    return;
  } //end if

  
  if((fd = open(input, O_WRONLY | O_CREAT, 0666)) == -1) { //open file
    perror("Error writing to file");
    send(sockfd, "STOP", 4, 0);
    return;
  } //end if

  printf("Downloading file %s...\n", input);
  send(sockfd, "OK", 2, 0);

  for(;;) {
    memset(rec, 0, 256);
    recv(sockfd, rec, 256, 0);

    if(strcmp(rec, END_OF_DATA) == 0) { //end signal received
      printf("Download of '%s' complete\n", input);
      close(fd);
      return;
    } else if(strcmp(rec, "ERR_DOWNLOAD") == 0) {
      send(sockfd, "OK", 2, 0);
      memset(rec, 0, 256);
      recv(sockfd, rec, 256, 0);
      printf("Error downloading file: %s\n", rec);
      if(remove(input) == -1) { //probably was a directory, remove it
	perror("Error removing file");
      }
      close(fd);
      return;
    } //end if-else

    if(write(fd, rec, strlen(rec)) == -1) {
      perror("Error writing to file");
      send(sockfd, "STOP", 4, 0);
      close(fd);
      return;
    } //end if

    send(sockfd, "OK", 2, 0); //ready for next segment
  } //end for

  close(fd);
  
} //end download
