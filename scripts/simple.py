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

from itertools import izip, imap, groupby, ifilter
from operator import itemgetter, attrgetter

from data.sentences import sentenceIterator

import csiparse
import deptree

from common import *


def main(options, args):
	pairsStream = open(args[0])
	pairsIterator = csiparse.instanceIterator(pairsStream)
	
	for sentence in sentenceIterator(fileinput.input(args[1:])):
		domains, constraints = csiparse.formulateWCSP(sentence,
													  None,
													  None,
													  pairsIterator,
													  options)

		depConstraints = [(i, list(deps)) for i, deps in groupby(
			ifilter(lambda c: isinstance(c, deptree.HasDependency),
					sorted(constraints, key=attrgetter("tokenIndex"))),
			attrgetter("tokenIndex"))]

		for i, (token, domain) in enumerate(izip(sentence, domains)):
			line = list(token)

			if depConstraints and depConstraints[0][0] == i:
				constraints = depConstraints.pop(0)[1]
			else:
				constraints = []

			if len(line) <= DEPREL:
				line.extend([None, None]) 

			try:
				rel = sorted(constraints, key=attrgetter("weight"))[-1]
				line[HEAD] = str(rel.headIndex + 1)
				line[DEPREL] = rel.relType
			except IndexError:
				line[HEAD] = "0"
				line[DEPREL] = "ROOT"

			print "\t".join(line)

		print

	pairsStream.close()


if __name__ == "__main__":
	import util.cmdline
	main(*util.cmdline.parse())
