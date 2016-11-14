####
#
# Pylet - Python Language Engineering Toolkit
# Written by Sander Canisius <S.V.M.Canisius@uvt.nl>
#

import sys
import traceback


class ChainedException(Exception):

	def __init__(self, message):
		self.message = message
		self.originalExceptionInfo = traceback.format_exc()

	def __str__(self):
		result = self.message
		if self.originalExceptionInfo:
			result += "\nCaused by:\n %s" % self.originalExceptionInfo
		return result
