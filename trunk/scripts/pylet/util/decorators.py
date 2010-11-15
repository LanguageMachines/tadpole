####
#
# Pylet - Python Language Engineering Toolkit
# Written by Sander Canisius <S.V.M.Canisius@uvt.nl>
#

import warnings


class AbstractDecorator(object):

	def __new__(typ, *args, **kwargs):
		self = object.__new__(typ)

		def register(function):
			def wrapper(*funArgs, **funKwargs):
				return self(*funArgs, **funKwargs)

			wrapper.__name__ = function.__name__
			self.wrappedFunction = function
			self.__init__(*args, **kwargs)				  
			return wrapper
		
		return register


class deprecated(AbstractDecorator):

	def __init__(self, msg):
		self.msg = msg

	def __call__(self, *args, **kwargs):
		warnings.warn("[%s]: this function has been deprecated. %s" % \
					  (self.wrappedFunction.__name__, self.msg),
					  DeprecationWarning, stacklevel=3)
		return self.wrappedFunction(*args, **kwargs)
