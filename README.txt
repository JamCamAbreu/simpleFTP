Author: James Cameron Abreu
Date: 06/03/2018
Description: Instructions on how to compile and use the 'ftserver' and
'ftclient' programs.

Project Source: https://github.com/JamCamAbreu/simpleFTP


Files:
  - README.txt:     (This file)
  - ftserver.c      Server program, written in C. Needs to be compiled
                    (instructions below)

  - ftclient.py     Client program, written in Python



Server Compilation Instructions:
  - In the directory containing the above files, execute the gcc command:
  gcc ftserver.c -o ftserver 

  - An executable called 'ftserver' will be created (execution instructions
    below)



How to use the ftserver and ftclient programs:
  1. Compile the server code (instructions above) if you haven't already.

  2. Execute the server (see 'Server Terminal Usage' below)

  3. Execute the client program with the server host name (can also be
     localhost if on same machine), server port, and a command (see 'Client
     Terminal Usage' below)

  4. The client program will execute your command and display and information
     needed. If a file was requested, you should now see it in the client
     directory. The connection to the server is then closed.

  5. The server will continue to run and accept connections until teriminated
     (with SIGINT: CONTROL-C).


Server Terimal Usage:
  ./ftserver <PORT_NUMBER>


Client Terminal Usage:
  ./ftclient.py <SERVER_HOST> <SERVER_PORT> <COMMAND> <OPTION_1> [OPTION_2]

  Note: SERVER_HOST, SERVER_PORT, and COMMAND required. See commands below:

  Possible COMMANDs:
  -l  : have the server send a directory listing. 
        REQUIRES: arbitrary unused port in OPTION_1

  -g  : have the server send a file with name supplied in OPTION_1
        REQUIRES: arbitrary unused port in OPTION_2
