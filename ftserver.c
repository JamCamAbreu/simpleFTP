/* *******************************************************
 * Author: James Cameron Abreu
 * Date: 06/01/2018
 * Description: Simple file transfer application - server
 *  This application allows for file transfer along with 
 *  a directory search from server to client.
 * *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <dirent.h> // for file directory search



/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *         GLOBAL CONSTANTS              
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
const int MAX_CLIENTS = 1;
const int MAX_COMMAND_INPUT = 512;
const int MAX_ARGUMENT_SIZE = 128;
const int MAX_ARGUMENTS = 3;

const char COM_DIR[] = "-l";
const int MAX_DIR_BUFFER = 1028;

const char COM_GET[] = "-g";

const int GET_ERR = 0;
const int GET_DIR = 1;
const int GET_FILE = 2;

const int FILE_BUFFER_SIZE = 4096; // 2^12



  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *  Error function used for reporting issues
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void error(const char *msg) { perror(msg); exit(0); } 


  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *     PUT DIR INTO STRING BUFFER        
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void getDir(char* buffer) {

  char nameBuff[256];
  int curLoc = 0;
  int curFileCharLoc = 0;
  char curChar;

  DIR *d;
  struct dirent* dir;
  d = opendir(".");

  if (d) {
    while ((dir = readdir(d)) != NULL) {
  
      // clear buffer:
      memset(nameBuff, '\0', sizeof(nameBuff)); 

      // reset location in current file name string:
      curFileCharLoc = 0;

      // copy name into buffer:
      sprintf(nameBuff, "%s", dir->d_name);

      // Copy over each char into buffer:
      curChar = nameBuff[0];
      while (curChar != '\0') {

        // put char in buffer, increment buffer loc
        buffer[curLoc] = curChar;
        curLoc++;

        // get char from cur filename, increment filename loc
        curFileCharLoc++;
        curChar = nameBuff[curFileCharLoc];
      }
      buffer[curLoc] = '\n';
      curLoc++;

      // rest of buffer is zeros because of memset :)
    }

    closedir(d);
  } // end if
}












/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *        MAIN PROGRAM                   
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
int main(int argc, char *argv[]) {


  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *      SETUP AND MORE GLOBAL VARIABLES          
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  // delim used to seperate arguments in client command:
  const char delim[2] = " ";
  char inputArgs[MAX_ARGUMENTS][MAX_ARGUMENT_SIZE];

  // important string buffers:
  char commandBuffer[MAX_COMMAND_INPUT];
  char fileDirBuffer[MAX_DIR_BUFFER];

  // peer name (updated after control connection):
  char peerName[256];
  struct sockaddr peer;
  socklen_t peer_len = sizeof(peer);
   
  // Check usage & args
  if (argc < 2) { fprintf(stderr,"USAGE: %s [port]\n", argv[0]); exit(0); } 


  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *           PRINT TITLE                    
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  // Get server name:
  char HOSTNAME[512];
  memset((char*)&HOSTNAME, '\0', sizeof(HOSTNAME)); 
  gethostname(HOSTNAME, sizeof(HOSTNAME));

  printf("---------------------------------------\n");
  printf("FTP server name: %s\n", HOSTNAME);
  printf("server port = %s\n", argv[1]);
  printf("---------------------------------------\n\n");


  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *      ESTABLISH CONTROL SOCKET            
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  int command_fd, portNumber, client_fd;
  struct sockaddr_in servaddr;

  portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
  command_fd = socket(AF_INET, SOCK_STREAM, 0);

  // Clear out the address struct
  memset((char*)&servaddr, '\0', sizeof(servaddr)); 

  // set addressing scheme to IP, set up socket:
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(portNumber);

  // bind socket:
  bind(command_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));



  // listen:
  printf("listening on port %s...\n", argv[1]);
  listen(command_fd, MAX_CLIENTS);


  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *        ACCEPT CONTROL CONNECTIONS     
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  while (1) {
    client_fd = accept(command_fd, (struct sockaddr*) NULL, NULL);
    

    // update peer name:
    memset(peerName, '\0', sizeof(peerName)); 
    getpeername(client_fd, &peer, &peer_len);
    getnameinfo(&peer, peer_len, peerName, sizeof(peerName), NULL, 0, 0);
    //printf("peername = %s\n", peerName);


    // Clear out input buffer:
    memset(&commandBuffer, '\0', sizeof(commandBuffer)); 

    // READ COMMAND FROM CLIENT:
    read(client_fd, commandBuffer, sizeof(commandBuffer));
    printf("Received command from client: '%s'\n", commandBuffer);


  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *            PARSE INPUT MESSAGE        
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    char *token;
    int count = 0;

    // clear out input argument strings:
    int i;
    for (i = 0; i < MAX_ARGUMENTS; i++)
      memset(inputArgs[i], '\0', sizeof(inputArgs[i])); 

    // Get first argument (token):
    token = strtok(commandBuffer, delim);
    count++;
    if (token != NULL)
      strcpy(inputArgs[0], token);

    // get remaining arguments (tokens), store in inputArgs:
    while (token != NULL) {
      token = strtok(NULL, delim);
      // don't copy null tokens:
      if (token != NULL)
        strcpy(inputArgs[count], token);
      count++;
    }


    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *    DEBUG, PRINT OUT INPUT ARGS        
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    /*
    printf("inputArgs:\n");
    for (i = 0; i < MAX_ARGUMENTS; i++) {
      printf("input:%d\t'%s'\n", i, inputArgs[i]);
    }
    printf("\n");
    // ---------------------------------------
    */


    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *        CHECK COMMAND TYPE             
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    // CHECK FOR COMMAND TYPE MATCH IN STRING:
    int commandType = GET_ERR; // default is error
    if (!strcmp(inputArgs[0], COM_DIR)) { commandType = GET_DIR; }
    else if (!strcmp(inputArgs[0], COM_GET)) { commandType = GET_FILE; }
    else { printf("ERROR: user sent unknown command '%s'\n", inputArgs[0]); }



    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *   IF FILE COMMAND, CHECK FILE EXISTS  
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    // If file read command, check if file exists:
    int fileExists = 1; // true in case command not for file
    char fName[strlen(inputArgs[1] + 1)];
    char messageBuff[256];
    memset(messageBuff, '\0', sizeof(messageBuff));

    if (commandType == GET_FILE) {
      memset(fName, '\0', sizeof(fName));
      strcpy(fName, inputArgs[1]);

      // File EXISTS:
      if (access(fName, F_OK) != -1) {
        printf("File with name '%s' requested on port %s\n", fName, inputArgs[2]);
        fileExists = 1;
        sprintf(messageBuff, "*");
      }

      // File does NOT exist!
      else {
        printf("File with name '%s' Requested but does not exist. ", fName);
        printf("Sending error to client.\n");
        sprintf(messageBuff, "File with name '%s' does not exist!", fName);
        fileExists = 0;
      }
    }

    else if (commandType == GET_DIR) {
      sprintf(messageBuff, "*");
    }


    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *           INVALID COMMAND             
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    // INVALID COMMAND:
    else {
      sprintf(messageBuff, "Error in command, see usage.\n");
    }


    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *       SEND ACK OR NAK TO CLIENT       
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    write(client_fd, messageBuff, sizeof(messageBuff));


    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *          EXECUTE COMMAND              
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    if (commandType && fileExists) {


      /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *     ESTABLISH DATA TCP CONNECTION     
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
      // port depends on commandType:
      int dataPort;
      if (commandType == GET_DIR)
        dataPort = atoi(inputArgs[1]); // client already checked int validity
      else if (commandType == GET_FILE)
        dataPort = atoi(inputArgs[2]); // client already checked int validity

      // Establish new data connection:
      int dataSock_fd;
      struct sockaddr_in dataAddress;
      struct hostent* dataHostInfo;
      
      // Set up the server address struct
      memset((char*)&dataAddress, '\0', sizeof(dataAddress)); // Clear out the address struct
      dataAddress.sin_family = AF_INET; // Create a network-capable socket
      dataAddress.sin_port = htons(dataPort); // Store the port number
      dataHostInfo = gethostbyname(peerName); // Convert the machine name into a special form of address
      if (dataHostInfo == NULL) { fprintf(stderr, "ERROR, no such host\n"); exit(0); }
      // Copy in the address
      memcpy((char*)&dataAddress.sin_addr.s_addr, (char*)dataHostInfo->h_addr, dataHostInfo->h_length); 

      // Set up the socket
      dataSock_fd = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
      if (dataSock_fd < 0) error("ERROR opening socket");
      
      // Connect to server (Connect socket to address)
      if (connect(dataSock_fd, (struct sockaddr*)&dataAddress, sizeof(dataAddress)) < 0) 
        error("ERROR connecting");



      /* ------------------------+
      // COMMAND: GET THE DIR    |
      ---------------------------+ */
      if (commandType == GET_DIR) {

        // clear the dir buffer:
        memset(fileDirBuffer, '\0', sizeof(fileDirBuffer)); 

        // Get the current directory contents, print to buffer:
        getDir(fileDirBuffer);
      
        // Write to the sock:
        write(dataSock_fd, fileDirBuffer, strlen(fileDirBuffer) + 1);
      }


      /* ------------------------+
      // COMMAND: GET FILE       |
      ---------------------------+ */
      else if (commandType == GET_FILE) {

        char bigBuffer[FILE_BUFFER_SIZE];
        FILE* fptr;
        int chars;

        if ((fptr = fopen(fName, "r")) == NULL)
          error("ERROR opening file");
        
        else {

          printf("Sending requested file...");

          memset(bigBuffer, '\0', sizeof(bigBuffer)); 
          fgets(bigBuffer, sizeof(bigBuffer) - 1, fptr);

          // Get file in CHUNKS (sizeof bigBuffer), until all file transfered:
          while (!feof(fptr)) {

            // send data to client:
            chars = send(dataSock_fd, bigBuffer, strlen(bigBuffer), 0);
            if (chars < 0) error("ERROR: Could not write to socket");
            if (chars < strlen(bigBuffer)) error("ERROR: message only sent in part!");

            memset(bigBuffer, '\0', sizeof(bigBuffer)); 

            // client blocks until confirm receipt:
            chars = recv(dataSock_fd, bigBuffer, sizeof(bigBuffer), 0);

            // get next chunk (if exists):
            fgets(bigBuffer, sizeof(bigBuffer) - 1, fptr);
          }

          // send final message:
          memset(bigBuffer, '\0', sizeof(bigBuffer)); 
          sprintf(bigBuffer, "%d", 0);
          send(dataSock_fd, bigBuffer, strlen(bigBuffer), 0);

          // close file
          fclose(fptr);
          printf("file send complete.\n");

        } // end if open file success

      } // end if command == GET_FILE


      /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *       CLOSE THE DATA SOCKET
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
      // Close the socket:
      close(dataSock_fd);
    }



    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *       CLOSE THE CONTROL SOCKET, WAIT
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    // close connection and wait for another:
    printf("Closing connection...");
    close(client_fd);
    printf("Connection closed.\n\n");
    printf("Awaiting new connections on port %s...\n", argv[1]);
  }



  printf("[PROGRAM CLEAN EXIT]\n");
  return 0;
}

