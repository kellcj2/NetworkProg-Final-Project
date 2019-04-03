/*
  Author:        Christopher J. Kelly
  Major:         CS - Software Dev
  Creation Date: 11/13/2018
  Due Date:      12/5/2018
  Course:        CSC 328-20
  Professor:     Frye
  Assignment:    Final Project: Download Server
  Filename:      ckelly_server.c
  Language:      C - gcc 4.4.7
  Compilation:   gcc ckelly_server.c -o S
  Executable:    ./S [<Port Number>]
                 ./S 34567
  Purpose:       Server program for a download server. Continuously checks for
                 client connections and processes their commands.
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
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/wait.h>
#include <fcntl.h>

void catcher(int sig);
void childCatcher(int sig);
void startServer();
void connectToClients();
void handleClient();
int validateCommand(char *);
void pwd();
void cd(char *);
void dir(char *);
void download(char *);

#define END_OF_DATA "))!_!(("

int port = 25821;  //the default port to use
int newConnfd, sockfd; //global for signal handler
char startDirectory[256];

int main(int argc, char *argv[]) {
  if(argc > 2) {
    printf("Usage: %s [<Port>]\n", argv[0]);
    exit(-1);
  } //end if

  static struct sigaction act;
  act.sa_handler = catcher; //closes the socket if server ends
  sigfillset(&(act.sa_mask));
  sigaction(SIGINT, &act, NULL);
  
  if(argc == 2)
    port = atoi(argv[1]); //set port to command line arg
  
  if(getcwd(startDirectory, 256) == NULL) { //sets home directory
    perror("Error getting home directory");
    exit(-1);
  } //end if
  
  startServer();
  connectToClients();  
} //end main


/*
  Function Name: startServer
  Description:   Starts the server using the given port
  Parameters:    none
  Return Value:  void
*/
void startServer() {
  struct sockaddr_in server = {AF_INET, htons(port), INADDR_ANY};
  
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { //create socket
    perror("Socket failed");
    exit(-1);
  } //end if

  if(bind(sockfd, (struct sockaddr *)&server,
	  sizeof(struct sockaddr_in)) == -1) { //bind server to socket
    perror("Bind failed");
    exit(-1);
  } //end if
  
  if(listen(sockfd, 5) == -1) { //listen for connection
    perror("Listen failed");
    exit(-1);
  } //end if

  //printf("Server started on port %d\n", port);
} //end startServer


/*
  Function Name: connectToClients
  Description:   Accepts incoming clients and creates a child process
  Parameters:    none
  Return Value:  void
*/
void connectToClients() {
  for(;;) {
    signal(SIGCHLD, childCatcher); //waits for child to end if server closes
    while(((newConnfd = accept(sockfd, NULL, NULL)) == -1) && (errno == EINTR)) ;

    if(newConnfd == -1) {
      perror("Accept failed");
      exit(-1);
    } //end if
    int pid;

    if((pid = fork()) == -1) {
      perror("Fork failed");
      exit(-1);
    } //end if
    
    if(pid == 0) { //child - the client
      close(sockfd);
      handleClient();
      exit(0);      
    } //end child

    close(newConnfd); //close fd in parent
  } //end for

} //end connectToClients


/*
  Function Name: handleClient
  Description:   Called by child process to process a client
  Parameters:    none
  Return Value:  void
*/
void handleClient() {
  char input[256];
  int result;
  send(newConnfd, "HELLO", 5, 0); //initial HELLO

  while(recv(newConnfd, input, 256, 0) > 0) { //receive data from client

    if((result = validateCommand(input)) > 0) { //valid command
      strtok(input, " \n");            //command
      char *arg = strtok(NULL, " \n"); //argument for command
      
      if(result == 1) {
	send(newConnfd, "OK_PWD", 6, 0);
	pwd();
	    
      } else if(result == 2) {
	send(newConnfd, "OK_CD", 5, 0);
	cd(arg);
	    
      } else if(result == 3) {
	send(newConnfd, "OK_DIR", 6, 0);
	dir(arg);
	    
      } else if(result == 4) {
	send(newConnfd, "OK_DOWNLOAD", 11, 0);
	download(arg);

      } else if(result == 5) { //disconnecting
	send(newConnfd, "EXIT", 4, 0);
	close(newConnfd);
	exit(0);
	    
      } else { //shouldnt happen due to validateCommand
	send(newConnfd, "ERR_NF", 6, 0);
      } //end if-else (result)
	  
    } else { //validate input returned false
      send(newConnfd, "ERR_NF", 6, 0); //NF == not found
    } //end if-else (validateCommand)
    
    memset(input, 0, sizeof(input));
  } //end while recv

  close(newConnfd);
} //end handleClient


/*
  Function Name: validateCommand
  Description:   Validates that a valid command was sent to the server
  Parameters:    char *input - data sent to server
  Return Value:  int - 1: PWD, 2: CD, 3: DIR, 4: DOWNLOAD, 5: EXIT, -1: Error
*/
int validateCommand(char *input) {
  char *token, *arg2, *arg3, tmp[256];
  strcpy(tmp, input);
  token = strtok(tmp, " \n");
  arg2 = strtok(NULL, " \n");
  arg3 = strtok(NULL, " \n");

  int i;
  for(i=0; i < strlen(token); i++) //convert to uppercase. Check in both C & S
    token[i] = toupper(token[i]);

  if(strcmp(token, "PWD") == 0) {
    if(arg2 != NULL)
      return -1;
    else
      return 1;
    
  } else if(strcmp(token, "CD") == 0) {
    if(arg2 != NULL && arg3 != NULL)
      return -1;
    else
      return 2;

  } else if((strcmp(token, "DIR") == 0) || (strcmp(token, "LS") == 0)) {
    if(arg2 != NULL && arg3 != NULL)
      return -1;
    else
      return 3;
    
  } else if(strcmp(token, "DOWNLOAD") == 0) {
    if(arg2 == NULL || arg3 != NULL)
      return -1;
    else
      return 4;

  } else if(strcmp(token, "EXIT") == 0) {
    return 5;
    
  } else {
    return -1;//invalid command
  } //end if-else
  
} //end validateCommand


/*
  Function Name: pwd
  Description:   Validates 'print directory' command and sends data to client
  Parameters:    none
  Return Value:  void
*/
void pwd() {
  char buffer[PATH_MAX]; //PATH_MAX is max size a directory name can be
  if(getcwd(buffer, PATH_MAX) != NULL) {
    send(newConnfd, buffer, strlen(buffer), 0);
  } else {
    send(newConnfd, "ERR_PWD", 7, 0);
    recv(newConnfd, buffer, 2, 0);
    send(newConnfd, strerror(errno), strlen(strerror(errno)), 0);
  } //end else-if

} //end pwd


/*
  Function Name: cd
  Description:   Validate 'change directory' command & sends data to client
  Parameters:    char *input - argument to cd
  Return Value:  void
*/
void cd(char *input) {
  char buffer[PATH_MAX];
  
  if(input == NULL) { //change directory to home
    if(chdir(startDirectory) == 0) {
      send(newConnfd, "~", 1, 0);
    } else {
      send(newConnfd, "ERR_CD", 6, 0);
      recv(newConnfd, buffer, 2, 0);
      send(newConnfd, strerror(errno), strlen(strerror(errno)), 0);
    } //end if-else
    
  } else { //change directory to arg1
    if(chdir(input) == 0) {

      if(getcwd(buffer, PATH_MAX) != NULL) {
	send(newConnfd, buffer, strlen(buffer), 0); //send cwd to client

      } else {
	send(newConnfd, "ERR_CD", 6, 0);
	recv(newConnfd, buffer, 2, 0);
	send(newConnfd, strerror(errno), strlen(strerror(errno)), 0);
      } //end if-else getcwd

    } else {
      send(newConnfd, "ERR_CD", 6, 0);
      recv(newConnfd, buffer, 2, 0);
      send(newConnfd, strerror(errno), strlen(strerror(errno)), 0);
    } //end if-else chdir
    
  } //end if-else
  
} //end cd


/*
  Function Name: dir
  Description:   Validates 'Directory List' command & sends data to client
  Parameters:    char *input - argument to dir
  Return Value:  void
*/
void dir(char *input) {
  struct dirent *dirStrPtr;
  DIR *directoryPtr;
  char *dirname = (char *)calloc(NAME_MAX, sizeof(char));
  char *filename = (char *)calloc(256, sizeof(char));
  char *errmsg = (char *)calloc(256, sizeof(char));
  struct stat statStr;

  if(input == NULL) { //no args
    strcpy(dirname, ".");  //list current dir
  } else {
    strcpy(dirname, input); //list given dir
  } //end if-else

  if((directoryPtr = opendir(dirname)) == NULL) {
    send(newConnfd, "ERR_DIR", 7, 0);
    recv(newConnfd, filename, 2, 0);
    send(newConnfd, strerror(errno), strlen(strerror(errno)), 0);
    return;
  } //end if

  errno = 0;
  while ((dirStrPtr = readdir(directoryPtr)) != NULL)   {
    sprintf(filename, "%s/%s", dirname, dirStrPtr->d_name); //NAME_MAX file size

    if (stat(filename, &statStr) == -1)  { //error stat
      send(newConnfd, "ERR_DIR", 7, 0);
      recv(newConnfd, filename, 2, 0);
      send(newConnfd, strerror(errno), strlen(strerror(errno)), 0);
      return;
    } // end if stat

    if((statStr.st_mode & S_IFMT) == S_IFREG) { //'-' for directories
      sprintf(filename, "%s", dirStrPtr->d_name);      
    } else {
      sprintf(filename, "-%s", dirStrPtr->d_name);
    } //end if
    
    send(newConnfd, filename, strlen(filename), 0);
    recv(newConnfd, filename, 256, 0); //client ready for next file

    errno = 0;
    memset(filename, 0, 256);
  } //end while

  if ((dirStrPtr == NULL) && (errno != 0))  {
    send(newConnfd, "ERR_DIR", 7, 0);
    recv(newConnfd, filename, 2, 0);
    send(newConnfd, strerror(errno), strlen(strerror(errno)), 0);
    return;
  }  // end if error
  
  send(newConnfd, "END_DIR", 7, 0);
  closedir(directoryPtr);
} //end dir


/*
  Function Name: download
  Description:   Validates DOWNLOAD command and sends data to client
  Parameters:    char *input - argument to download
  Return Value:  void
*/
void download(char *input) {
  int fd;
  char buffer[256];
  ssize_t bytesRead;

  memset(buffer, 0, 256);
  recv(newConnfd, buffer, 256, 0);

  if(strcmp(buffer, "STOP") == 0) //client canceled at checking file existance
    return;

  if((fd = open(input, O_RDONLY)) == -1) {
    send(newConnfd, "ERR_DOWNLOAD", 12, 0); //can't open file
    recv(newConnfd, buffer, 2, 0);
    send(newConnfd, strerror(errno), strlen(strerror(errno)), 0);
    return;
  } else {
    send(newConnfd, "READY", 5, 0); //ready to start
  } //end if-else

  memset(buffer, 0, 256);
  recv(newConnfd, buffer, 256, 0);
  if(strcmp(buffer, "STOP") == 0) { //client had error opening file
    close(fd);
    return;
  } //end if

  memset(buffer, 0, 256);
  while((bytesRead = read(fd, &buffer, 256)) > 0) {
    send(newConnfd, buffer, strlen(buffer), 0);
    recv(newConnfd, buffer, 256, 0); //client ready for next segment
    if(strcmp(buffer, "STOP") == 0) { //client encountered an error
      close(fd);
      return;
    } //end if
    memset(buffer, 0, 256);
  } //end while

  if(bytesRead < 0) { //open() doesnt check if a directory, this will catch it
    send(newConnfd, "ERR_DOWNLOAD", 12, 0);
    recv(newConnfd, buffer, 2, 0);
    send(newConnfd, strerror(errno), strlen(strerror(errno)), 0);
    close(fd);
    return;
  } //end if
  
  send(newConnfd, END_OF_DATA, strlen(END_OF_DATA), 0); //end signal
  close(fd);
} //end download


/*
  Function Name: catcher
  Description:   Closes socket if close signal received
  Parameters:    int sig - close signal
  Return Value:  void
*/
void catcher(int sig) {
  printf("Server closing...\n");
  close(newConnfd);
  close(sockfd);
  exit(0);
} // end catcher


/*
  Function Name: childCatcher
  Description:   Ends child when it disconnects
  Parameters:    int sig - close signal
  Return Value:  void
*/
void childCatcher(int sig) {
  pid_t pid;
  int stat;

  while((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
    close(newConnfd);
    printf("\nServer: Child %d terminated\n", pid);
  } //end while

} //end childCatcher
