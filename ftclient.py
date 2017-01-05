# Filename: ftclient.py
# Author: James Mills | 932704566
# Date: Fall 2016
# Course/Project: CS372/P2

# our imports
from socket import *
from datetime import datetime # for handling timeout
import sys
import os

# constant values
BUFF_SIZE = 500 # bytes received
MAX_TIME = 10 # maximum time (sec) to wait for data
DIR = "downloads"


# checks to see whether filename exists in our 
# download directory, and modifies accordingly
# returns modified version if necessary
def setupFile( filename ):

	# checks to see whether our file already 
	# exists. credit to stack overflow 273192
	if os.path.exists( DIR + "/" + filename ):
		# do we have a good filename
		goodName = False
		filename = filename + ".dup"
		while goodName == False:
			if os.path.exists( DIR + "/" + filename ):
				filename = filename + ".dup"
			else:
				goodName = True
									
	return filename	


# handles a file transfer from server to client
def handleGetClient( conn, address, filename):
	print "Receiving file: "

	# set up our download folder if it doesn't already
	# exist. credit to stack overflow 273192
	if not os.path.exists( DIR ):
		os.makedirs( DIR )

	# set up our download file. this will modify the
	# new filename in case there are duplicates
	newfile = setupFile( filename )
	fp = open( DIR + "/" + newfile, 'w' )

	# first thing's first, figure out how many bytes
	# of data we should receive from the server
	msg = conn.recv( BUFF_SIZE )

	# convert to int and strip off null-terminator
	# for the last part, credit to stack overflow
	# question 22747152
	fileSize = int(msg.strip('\0'))

	# boolean value which is true while the connection
	# between our two hosts remains live
	bytesRecv = 0
	lastTime = datetime.now()
	timeout = False
	while bytesRecv < fileSize and timeout == False:
		msg = conn.recv( BUFF_SIZE ).strip('\0')

		# handle timeout conditions. i.e. if we received data
		# update time of last receive. otherwise, check to
		# see whether maximum time allowed has passed
		if len(msg) > 0:
			lastTime = datetime.now()
			bytesRecv += len(msg)
			print "bytes received: " + str(len(msg))
			fp.write(msg)
		else:
			elapsed = (datetime.now() - lastTime).total_seconds
			if elapsed > MAX_TIME:
				print "Error: a timeout has occured..."
				timeout = True		

	# close our file, we're done	
	fp.close()	


# receives a list of files in the server host's working 
# directory, one by one, till it sees the quit character
def handleListClient( conn, address):
	print "Files in directory: "

	# boolean value which is true while the connection
	# between our two hosts remains live
	connected = True
	while connected == True:
		msg = conn.recv( BUFF_SIZE )
		if "?q" in msg:
			connected = False
		else:
			print msg, 


# main server function: takes port number that we'll 
# run the server on, passes new connections to 
# another function, which handles inter-host comms
# this is a modified version of what I submitted
# for cs372 project 1
def runServer( args ):

	# get our host name
	servAddr = gethostname()

	# get our port
	action = args[3]
	if action == "-l":
		port = args[4]
	elif action == "-g":
		port = args[5]

	# establish new socket using the IPv4 address
	# family and TCP
	sock = socket( AF_INET, SOCK_STREAM )

	# bind the socket to localhost at the given port
	sock.bind( (servAddr, int(port)) )

	# listen for connections (max queued = 1)
	sock.listen(5)

	# loop till you receive the interupt
	conn, address = sock.accept()

	# now that we have a connection, run either the
	# list or the get handler accordingly
	if action == "-l":
		handleListClient( conn, address)

	elif action == "-g":
		handleGetClient( conn, address, args[4])

	# close up shop and go home
	conn.close()


# takes an established connection with another host
# and handles communications to and from that host
def connect( host, port, args ):
	sock = socket( AF_INET, SOCK_STREAM )
	sock.connect( (host, port) )

	# form and send command string
	command = ""
	for q in range(0, len(args)):
		if q > 2 and q < ( len(args) - 1):
			if args[q] == "-l":
				command = command + "LIST "
			elif args[q] == "-g": 
				command = command + "GET "
			else:
				command = command + args[q] + " "
		elif q == (len(args) - 1):
			command = command + args[q]
	sock.send( command )
	
	# boolean value which is true while the connection
	# between our two hosts remains live
	msg = sock.recv( BUFF_SIZE )
	print msg

	# check for go ahead
	if msg[:4] == "OKAY":
		# now start up a server to accept incoming data 
		# from the server	
		runServer( args )	
	
	# close up shop and go home	
	sock.close()


# validates command-line arguments for correct number
# and type. returns 0 for valid, -1 for invalid
def validateInput( args ):
	def printUse():
		print "Usage: python host port -l port"
		print "or...  python host port -g filename port\n"

	argc = len(args)
	host = args[1].lower()

	# check for minimum number of args
	if argc < 5:
		print "Error: not enough arguments"
		printUse()

	# make sure command is in the correct form	
	elif args[3][0] != "-":
		print "Error: third arg should be -l or -g operation"
		printUse()
		return -1

	# make sure it's one of the two valid commands
	elif args[3][1] != "l" and args[3][1] != "g":
		print "Error: third arg should be -l or -g operation"
		printUse()
		return -1

	# check for flip host in arguments
	elif host != "flip1" and host != "flip2" and host != "flip3":
		print "Error: first argument must be flip server prefix",
		print ", e.g. flip1"
		printUse()
		return -1

	# check for valid port in first arg
	elif args[2].isdigit() == False and args[2] > -1:
		print "Error: second argument must be valid port"
		printUse()
		return -1	

	# check for properly formed list command
	elif args[3][1] == "l":
		if argc > 5:
			print "Error: too many args"
			printUse()
			return -1	

		elif args[4].isdigit() == False and args[4] > -1:
			print "Error: fourth argument must be valid port"
			printUse()
			return -1	

		# make sure that command port and transfer port are
		# not one and the same
		elif args[2] == args[4]:
			print "Error: ports must be different"
			printUse()
			return -1


	# check for properly formed get command
	elif args[3][1] == "g":
		if argc < 6:
			print "Error: not enough args for -g command"
			printUse()
			return -1	

		elif args[5].isdigit() == False and args[5] > -1:
			print "Error: fifth argument in -g command must be port"
			printUse()
			return -1	

		elif args[4][-4:] != ".txt":
			print "Error: fourth argument in -g command must be txt file"
			printUse()
			return -1	

		# make sure that command port and transfer port are
		# not one and the same
		elif args[2] == args[5]:
			print "Error: ports must be different"
			printUse()
			return -1

	return 0



# our defacto main function
if __name__ == "__main__":
	if validateInput( sys.argv ) != 0:
		sys.exit(-1)

	# pull our initial connection data
	host = sys.argv[1]
	port = sys.argv[2]

	print "Connecting to host at " + \
	 	host + ".engr.oregonstate.edu" + ":" + str(port)
	connect( host, int(port), sys.argv )	
