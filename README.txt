filename: README.txt
author: James Mills
date: Fall 2016


Overview:
This collection of applications allows users to run a simple
file transfer service across one or more of Oregon State's 
Linux servers (i.e. Flip). The server-side application ftserver
listens on a Linux socket for incoming connection requests.
When an ftclient connects, the server opens up a second connection
with this client, parses and validates the incoming request
and reacts accordingly. This can mean one of either two things:
in case of an error or list request, this data is sent back to
the client user; in the case of a valid file request, the server
opens up yet another connection and transfers the file.


Compilation instructions:
1. gcc ftserver.c -o ftserver


Running instructions - get a list of files:
1. ./ftserver [port] 
2. python ftclient flip[suffix] [port] -l [new port]
	e.g. python ftclient flip3 24601 -l 24602 


Running instructions - get a file:
1. ./ftserver [port] 
2. python ftclient flip[suffix] [port] -g [filename] [new port]
	e.g. python ftclient flip3 24601 -g README.txt 24602

Note: files are downloaded in the client's current working
	  directory under ../downloads/


References:
1. For C:
	a. Beej's guide to networking using internet sockets
	b. Stack overflow - e.g. stackoverflow.com/questions/<number>:
		x. 238603 - fseek method for counting bytes in file
		x. 4204666 - get files in working dir
		x. 230062 - check whether file exists 
2. For python:
	a. Tutorialspoint.com 
	   https://www.tutorialspoint.com/python/python_networking.htm
	b. The Python documentation
	   https://docs.python.org/2/library/socket.html
	b. Stack overflow - e.g. stackoverflow.com/questions/<number>:
		x. 273192 - check whether file or dir exists
		x. 22747152 - strip null terminators from incoming c-strings 
3. Additionally, I used much of the code from my submission for
   project one. All references have been carried over.
