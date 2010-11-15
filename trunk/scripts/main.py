#!/usr/bin/env python

"""
Generate instances for the pairwise dependency prediction task.

usage: %prog [options] [file...]

-mDIST, --max-dist=DIST: maximum distance between head and dependent
-x, --exclude-non-scoring: do not generate instance for non-scoring
                           tokens

---
:Refinements:
-m: type='int', dest='maxDist'
-x: action='store_true', default=False, dest='skipNonScoring'
"""

import fileinput
import sys

from itertools import izip, imap, groupby, ifilter
from operator import itemgetter, attrgetter

from pylet.data.sentences import sentenceIterator

import cky
import csiparse2 as csiparse
import deptree

from common import *


def leftIncomplete(chart, s, t, sentence):
	r = chart[s, t, "l", False].r
	label = chart[s, t, "l", False].edgeLabel
	if r is not None:
		assert s > 0
		sentence[s - 1][DEPREL] = label
		sentence[s - 1][HEAD] = t
		rightComplete(chart, s, r, sentence)
		leftComplete(chart, r + 1, t, sentence)


def rightIncomplete(chart, s, t, sentence):
	r = chart[s, t, "r", False].r
	label = chart[s, t, "r", False].edgeLabel
	if r is not None:
		sentence[t - 1][DEPREL] = label
		sentence[t - 1][HEAD] = s
		rightComplete(chart, s, r, sentence)
		leftComplete(chart, r + 1, t, sentence)


def leftComplete(chart, s, t, sentence):
	r = chart[s, t, "l", True].r
	if r is not None:
		leftComplete(chart, s, r, sentence)
		leftIncomplete(chart, r, t, sentence)


def rightComplete(chart, s, t, sentence):
	r = chart[s, t, "r", True].r
	if r is not None:
		rightIncomplete(chart, s, r, sentence)
		rightComplete(chart, r, t, sentence)


def main(options, args):
	dirOutput, relsOutput, pairsOutput = args[:3]

	dirStream = open(dirOutput)
	relsStream = open(relsOutput)
	pairsStream = open(pairsOutput)

	dirIterator = csiparse.instanceIterator(dirStream)
	relsIterator = csiparse.instanceIterator(relsStream)
	pairsIterator = csiparse.instanceIterator(pairsStream)
	
	for sentence in sentenceIterator(fileinput.input(args[3:])):
		domains, constraints = csiparse.formulateWCSP(sentence,
													  dirIterator,
													  relsIterator,
													  pairsIterator,
													  options)

		parser = cky.CKYParser(len(sentence))
		for constraint in constraints:
			parser.addConstraint(constraint)

		chart = parser.parse()

		#item = chart[0, self.numTokens - 1, "r", True]

		#for token in sentence:
		#	token[DEPREL] = "__"
		#	token[HEAD] = "__"
		for token in sentence:
			if len(token) <= DEPREL:
				token.extend([None, None])  

		#print chart[0, len(sentence) - 1, "r", True].r + 1
		rightComplete(chart, 0, len(sentence) - 1 + 1, sentence)

		for token in sentence:
			print " ".join(map(str, token))
		print
		
		sys.stderr.write(".")


	dirStream.close()
	relsStream.close()
	pairsStream.close()


if __name__ == "__main__":
	from pylet.util import cmdline
	main(*cmdline.parse())
