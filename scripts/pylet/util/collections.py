####
#
# Pylet - Python Language Engineering Toolkit
# Written by Sander Canisius <S.V.M.Canisius@uvt.nl>
#

import copy
import random


def randomSubset(iterable):
	return set(filter(lambda elt: bool(random.randrange(0,2)), iterable))


class DefaultDict(dict):

	def __init__(self, defaultFactory, *args, **kwargs):
		dict.__init__(self, *args, **kwargs)
		self.defaultFactory = defaultFactory
		
	def __getitem__(self, key):
		if key in self:
			return self.get(key)
		else:
			return self.setdefault(key, self.defaultFactory())
