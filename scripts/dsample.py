#!/usr/bin/env python

"""
Performs down-sampling of a specified class.

usage: %prog [options] file

-sSEED, --seed=SEED: seed to initialise the random number generator with
-cLABEL, --class=LABEL: label of the class to down-sample
-fNUM, --field=NUM: index of the field (column) containing the class label
-rRATIO, --ratio=RATIO: down-sampling ratio, i.e. the number of
                        instances of the specified class to retain for
                        each instance of another class

---
:Refinements:
-s: type='int', default=1234
-c: dest='classLabel', default='__'
-f: type='int', default=-1
-r: type='int', default=1
"""

import random
import sys

from itertools import imap


def main(options, args):
	newRatio = options.ratio
	seed = options.seed
	classLabel = options.classLabel
	fieldNum = options.field
	
	filename = args[0]
	
	random.seed(seed)
	
	positives = 0
	negatives = 0

	stream = open(filename)

	for instance in imap(str.split, stream):
		if instance[fieldNum] == classLabel:
			negatives += 1
		else:
			positives += 1


	origRatio = negatives / positives
	size = int(1.0 * newRatio / origRatio * negatives)
	print >> sys.stderr, "Original pos:neg ratio = 1:%s" % origRatio
	print >> sys.stderr, "Down-sampling to 1:%s" % newRatio

	stream.seek(0)

	for instance in imap(str.split, stream):
		if instance[fieldNum] == classLabel:
			if random.random() < 1.0 * size / negatives:
				print " ".join(instance)
				size -= 1
			negatives -= 1
		else:
			print " ".join(instance)

	stream.close()


if __name__ == "__main__":
	import util.cmdline
	main(*util.cmdline.parse())
