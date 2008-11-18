#!/usr/bin/env python

"""
Generate instances for the pairwise dependency prediction task.

usage: %prog [options] [file...]

-mDIST, --max-dist=DIST: maximum distance between head and dependent

---
:Refinements:
-m: type='int', dest='maxDist'
"""

import fileinput

from itertools import imap

from data.sentences import sentenceIterator

import common
import deptree


def parseDist(string):
	dist = dict((label.strip(), float(weight))
				for label, weight in map(str.split, string.strip().split(",")))
	normFactor = sum(dist.itervalues())
	for cls, weight in dist.iteritems():
		dist[cls] = weight / normFactor
		
	return dist


def parseInstanceLine(line):
	instance = line.split()
	startIndex = instance.index("{")
	endIndex = instance.index("}", startIndex)
	dist = parseDist(" ".join(instance[startIndex:endIndex]).strip("{}"))
	return instance[:startIndex], dist, float(instance[endIndex+1])


def instanceIterator(stream):
	return imap(parseInstanceLine, stream)


def formulateWCSP(sentence, dirInstances,
				  relInstances, pairInstances,
				  options):
	domains = [[] for token in sentence]
	constraints = []

	for dependent, head in common.pairIterator(sentence, options):
		dependentId = int(dependent[0]) - 1
		headId = int(head[0]) - 1

		instance, distribution, distance = pairInstances.next()

		cls = instance[-1]
		conf = distribution[cls]
		#if distance == 0:
		#	conf = 1000000
		#else:
		#	conf = 1.0 / distance

		#constraints.append(deptree.HasDependency(dependentId,
		#										 headId, cls,
		#										 conf))
		if cls != "__":
			constraints.append(deptree.HasDependency(dependentId,
													 headId, cls,
													 conf))
		if cls != "__":
			domains[dependentId].append((headId, cls))

	return domains, constraints


def main(options, args):
	dirOutput, relsOutput, pairsOutput = args[:3]

	dirStream = open(dirOutput)
	relsStream = open(relsOutput)
	pairsStream = open(pairsOutput)

	dirIterator = instanceIterator(dirStream)
	relsIterator = instanceIterator(relsStream)
	pairsIterator = instanceIterator(pairsStream)
	
	for sentence in sentenceIterator(fileinput.input(args[3:])):
		csp = formulateWCSP(sentence,
							dirIterator,
							relsIterator,
							pairsIterator,
							options)


	dirStream.close()
	relsStream.close()
	pairsStream.close()


if __name__ == "__main__":
	import util.cmdline
	main(*util.cmdline.parse())
