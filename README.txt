Client / Server Download Program
--------------------------------

Description:
	This program is a download server as well as the client. The host
	runs the server program and the client will be able to connect to it.
	Possible commands are DIR(LS), PWD, DOWNLOAD, and CD.


Compilation:
	gcc ckelly_server.c -o Server //server program
	gcc ckelly_client.c -o Client //client program
	(OR a Makefile is also included)


Run:
	./Server [<Port Number>]
	./Client <Hostname> [<Port Number>]


Design Overview:
       The server listens for incoming connections and when one is received, it
       creates a child process to handle it. This continuously receives commands
       from the client and processes them accordingly. It also validates that
       the command exists and the correct number of arguments was sent.
       
       The client attempts to connect to the server with the given hostname,
       then continously accepts user input to send to the server. It also checks
       that the commands are valid and have the correct number of args.

       PWD - No arguments. Prints working directory
       DIR <[Directory]> - Lists the current directory or the given directory
       CD <[Directory]> - Changes directory to home or the given directory
       DOWNLOAD [Filename] - Downloads the file specified


Protocol:
	If the client entered a valid command, it will send that command to the
	server. Then, the server will also check if it is valid and send an 
	OK message, indicating it validated.
	Then, the server will attempt to execute the command and send
	appropriate data. 
	If an error is encountered, the server sends an ERR message, then 
	waits for the client to ackowledge that it received it. After, the
	server sends the details of the error message to the client.

	For the download, after the server sends a piece of data, the client
	responds with an OK, indicating it was received. The reason for the 
	client responding this way is explained in "Notes" below.
	When downloading, the client checks if it received an END_OF_DATA
	symbol, which the server sends after the file is complete. If the
	symbol was not received, it continues accepting data from the server.

	ERR and OK messages are unique for each type of command. This made
	it much easier to bug test, since sometimes the server would be stuck
	but I wouldn't know where it was stuck. By printing the messages to the
	screen (for example ERR_DOWNLOAD, ERR_CD, OK_PWD), it made it much 
	easier to bug test and ensure the program was working correctly.


Known Bugs:
        Occasionally, when pwd is entered for the first time, nothing happens.
	The program will continue working normally, including pwd. I can't 
	figure out how to reproduce this- 95% of the time the program works fine.

	When download is entered with a pathname (i.e. download ../dir/file.txt)
	the client cannot download, since it tries to create a directory at
	"../". A way to fix this would be removing all "." and "/"s
	from the filename before the file is opened on the client side.....


Notes:
	When the server is sending data without a receive in between
	(DIR / DOWNLOAD), the client would sometimes not correctly process the
	data if an error was sent. For example, the server would send
	ERR_DOWNLOAD, which indicates there was an error and to stop.
	However, when the client would do strcmp(rec, "ERR_DOWNLOAD") it didn't
	return 0. From printing out the messages received, I determined this is
	because when the server does 2 sends in a row the client may not process
	them as 2 seperate sends. For example, the client was supposed to get
	ERR_DOWNLOAD, then the error message, but it seems the client got both
	simultaneously, causing the strcmp to return false. In order to fix this
	the client will send an 'OK' message to the server before it gets the
	error message, and the server will wait for the OK from the client, then
	send the appropriate error.
	(If my logic behind why the problem occurred is false, adding the
	additional send/recvs fixed the error anyway.
	I realized a better solution would be to send the ERR_DOWNLOAD message
	AND the strerror message in the same send, then using strncmp in the
	client to check if the first part of the message is ERR_DOWNLOAD, then
	printing the rest of the message, but changing it would take too much
	time and I want to turn this in early. Also, it would have been better
	to receive errors in a function instead of doing the same 4 lines every
	time, but again I wanted to turn it in early.)
