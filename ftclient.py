#! /usr/bin/python

"""
Author: James Cameron Abreu
Date: 06/01/2018
Description: A simple File Transfer Application - Client
 This program uses two TCP connections to transfer data 
 from a server to a client. A server directory printout 
 is also possible. 
"""

from socket import *
import signal
import sys
import os.path


# +-----------------------------+
# |   HELPER FUNCTIONS          |
# +-----------------------------+
# test string for int:
def isInt(s):
  try:
    int(s)
    return True
  except ValueError:
    return False

#Debug function
def getType(s):
  if (isInt(s) == True):
    return '<int>'
  else:
    return '<string>'

# show usage and exit program with error 1
def errUsage():
  print('usage: ' + sys.argv[0] + ' <SERVER_HOST> <SERVER_PORT> <COMMAND> <OPTION_1> [OPTION_2]')
  sys.exit(1)



# +-----------------------------+
# |     GLOBAL CONSTANTS        |
# +-----------------------------+
COM_DIR = '-l'
COM_GET = '-g'
CONTINUE = '*'
MAX_REC_BUFFER = 4096


# +-----------------------------+
# |    ASSIGN INPUT NAMES       |
# +-----------------------------+
if (len(sys.argv) < 5):
  errUsage()

# Take inputs from argument list:
serverName = sys.argv[1]
serverPort = sys.argv[2]
command = sys.argv[3]
option_1 = sys.argv[4]
option_2 = "NA"
if (len(sys.argv) >= 6):
  option_2 = sys.argv[5]



# +-----------------------------+
# | DEBUG: PRINT ARGUMENTS      |
# +-----------------------------+
"""
print('------------DEBUG-------------')
print('<SERVER_HOST> = ' + serverName + ' ' + getType(serverName))
print('<SERVER_PORT> = ' + serverPort + ' ' + getType(serverPort))

print('<COMMAND> = ' + command + ' ' + getType(command))
print('<OPTION_1> = ' + option_1 + ' ' + getType(option_1))

if (len(sys.argv) >= 6):
  option_2 = sys.argv[5]
  print('[OPTION_2] = ' + option_2 + ' ' + getType(option_2))
print('------------------------------\n')
"""


# +-----------------------------+
# | ADDITIONAL INPUT CHECKS     |
# +-----------------------------+
# Check for bad port number:
if (isInt(sys.argv[2]) != True):
  print("ERROR: Invalid SERVER_PORT")
  errUsage()

# port for data socket must be numerical in file get request
if COM_GET in command:
  if (isInt(option_2) != True):
    print("ERROR: OPTION 2 MUST BE PORT NUMBER FOR FILE TRANSFER")
    errUsage()

# convert serverPort to int (already checked if possible):
serverPort = int(serverPort)

# combine command and options into one string:
message = command + ' ' + option_1 + ' ' + option_2






# +----------------------------+
# |   CHECK COMMAND TO RUN     |
# +----------------------------+
com = 0
if COM_DIR in command:
  com = 1
  #print "let's get the dir!"

elif COM_GET in command:
  com = 2
  #print "Let's get a file!"



# +----------------------------+
# |   CHECK FILE FOR WRITING   |
# +----------------------------+
f = "./" + option_1;
fName = f
if (com == 2):
  if (os.path.isfile(f)):
    print "File already exists!"
    print "Would you like to rename a copy?"
    response = raw_input("Enter 'y' for yes, or 'n' to overwrite file: ")
    if 'y' in response:
      fName = raw_input("Enter a new file name: ")



# +----------------------------+
# | BEGIN CONNECTION TO SERVER |
# |     (CONTROL SOCKET)       |
# |  AND SEND THE COMMAND      |
# +----------------------------+

clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((serverName, serverPort))

# send the command:
clientSocket.send(message.encode())


# listen for command response:
message = clientSocket.recv(MAX_REC_BUFFER).decode()

if CONTINUE not in message:
  com = 0 # prevent data transfer
  print(message) # message explains error



# +-----------------------------+
# | RECEIVE DATA FROM DATA SOCK |
# +-----------------------------+
# set up a welcoming socket for data:
if (com):

  # port depends on command type:
  if (com == 1):
    dataPort = int(option_1) # provided by user in command line
  elif (com == 2):
    dataPort = int(option_2) # provided by user in command line

  dataAddress = ('', dataPort)
  maxQueue = 1
  dataSocket = socket(AF_INET, SOCK_STREAM)
  dataSocket.bind(dataAddress)
  dataSocket.listen(maxQueue)
  d_Socket, d_Address = dataSocket.accept()
  print('Data connection established. Trasferring file...')

  # FIRST (and maybe last) chunk of data:
  sentence = d_Socket.recv(MAX_REC_BUFFER).decode()


  # +-----------------------------+
  # | COMMAND: PRINT DIRECTORY    |
  # +-----------------------------+
  # PRINT DIR:
  if (com == 1):
    print("Receiving directory from server:")
    print(sentence)


  # +-----------------------------+
  # | COMMAND: GET FILE FROM SERV |
  # +-----------------------------+
  # TRANSFER FILE AND WRITE TO LOCAL FILE:
  if (com == 2):
    # SET UP FILE FOR WRITING:
    newFile = open(fName, "w") # see 'fName' above in check file for writing section

    # Write to file:
    if (len(sentence) > 0):
      newFile.write(sentence)

    # keep printing till end of file transfer:
    if (com ==2):
      while (sentence != '0'):

        # send acknowledgement:
        receipt = "1"
        d_Socket.send(receipt.encode())

        # retrieve next part:
        sentence = d_Socket.recv(MAX_REC_BUFFER).decode()

        # Write to file:
        if (len(sentence) > 1):
          newFile.write(sentence)

      # Close the file:
      newFile.close()
      print('Trasfer success.')


  # +-----------------------------+
  # |     CLOSE DATA SOCKET       |
  # +-----------------------------+
  d_Socket.close()


# +-----------------------------+
# |   CLOSE COMMAND SOCKET      |
# +-----------------------------+
clientSocket.close()
