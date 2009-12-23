#!/usr/bin/python
#by Maarten van Gompel (adapted from code by Rogier Kraf), licensed under GPLv3
from socket import *
import re

class TadpoleClient:
	def __init__(self,host="localhost",port="12345", tadpole_encoding="iso-8859-1"):
		"""Create a client connecting to a Tadpole server"""
		self.BUFSIZE = 4096
		self.socket = socket(AF_INET,SOCK_STREAM)
		self.socket.connect( (host,port) )
		self.tadpole_encoding = tadpole_encoding

	def process(self,input_data, source_encoding="utf-8", return_unicode = True):
		"""Receives input_data in the form of a str unicode object, passes this to the server, with proper consideration for the encodings, and returns the Tadpole output as a list of tuples: (word,pos,lemma,morphology), each of these is a proper unicode object unless return_unicode is set to False, in which case raw strings in the tadpole encoding will be returned."""
		input_data = input_data.strip(' \t\n')

		targetbuffer = re.sub("[ -]","",input_data)
		buffer = ""		
		
		#print "SEND: ",input_data #DEBUG
		if not isinstance(input_data, unicode):
			input_data = unicode(input_data, source_encoding) #decode (or preferably do this in an earlier stage)
		self.socket.sendall(input_data.encode(self.tadpole_encoding) +'\n') #send to socket in desired encoding

		tp_output = []
		
		cnt = 0
		done = False
		while not done:	
			data = ""
			while not data or data[-1] != '\n':
				data += self.socket.recv(self.BUFSIZE)
			if return_unicode:
				data = unicode(data,self.tadpole_encoding)
		

			for line in data.strip(' \t\r\n').split('\n'):
				if line == "READY":
					#print "DONE" #DEBUG
					done = True
					break
				elif line:
					l = line.split('\t') # remove the linenumbers + \t here that tadpole seems to add now
					if len(l) > 1:
    						buffer += re.sub("[ -]","",l[1])
	    					line = '\t'.join(l[1:]) 
	    					tp_output.append(tuple(line.strip('\t').split('\t')))
		return tp_output

	def __del__(self):
		self.socket.close()
