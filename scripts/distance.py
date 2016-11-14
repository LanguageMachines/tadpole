#!/usr/bin/env python

"""
Determines the distance range for dependencies making up a certain
amount of the total probability mass.

usage: %prog [options] [file...]

-pNUM, --prob-mass=NUM: the probability mass (default: 0.99)

---
:Refinements:
-p: type='float', default=0.99, dest='probMass'
"""

import fileinput

from itertools import izip

from data.sentences import sentenceIterator
from util.collections import DefaultDict


ID, FORM, LEMMA, CPOSTAG, POSTAG, \
FEATS, HEAD, DEPREL, PHEAD, PDEPREL = range(10)


def main(options, args):
	dist = DefaultDict(int)
	probMass = options.probMass
	
	for sentence in sentenceIterator(fileinput.input(args)):
		for i, token in enumerate(sentence):
			if token[HEAD] != "0":
				try:
					dist[int(token[ID]) - int(token[HEAD])] += 1
				except ValueError:
					print "Error in line:", fileinput.filelineno() - len(sentence) + i
					return

	totalCount = sum(dist.itervalues())
	mass = 0
	width = 0
	while mass < probMass:
		width += 1
		mass += 1.0 * (dist[width] + dist[-width]) / totalCount

	print width


if __name__ == "__main__":
	import util.cmdline
	main(*util.cmdline.parse())
