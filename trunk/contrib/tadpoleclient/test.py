#!/usr/bin/python
# -*- coding: utf-8 -*-
from sys import stderr
from tadpoleclient import TadpoleClient

#generating some example sentences to pass to the server:
sentences = [u"Dhr. M. A. de Vries heeft vandaag één nieuwe koelkast gekocht.", "De cliënt houdt een appèl in de oude ruïne"] #sentences with diacritics, the client will take care of proper conversion
sentences += ['Dit is zin ' + str(i) + "." for i in range(0,101)] #100 sentences


#Create a client instance connecting to the specified server (make sure to run the server first: $ Tadpole -S 12345 )
tadpoleclient = TadpoleClient('localhost',12345) 

#pass all sentences to the server one by one:
for i, sentence in enumerate(sentences):
	print "INPUT",i,":", sentence
	tp_output = tadpoleclient.process(sentence) #pass sentence to the server and return the response as a list of tuples: [(word, pos, lemma, morph) , ... ], one element for each word
	if tp_output:
			print "OUTPUT",i,":", tp_output
	else:
			print >>stderr, "ERROR *** NO OUTPUT! (sentence ",i,")" #shouldn't happen :)

del tadpoleclient #closes the connection (not really explicitly needed here as the object would be destroyed automatically at the end anyhow)

