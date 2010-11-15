####
#
# Pylet - Python Language Engineering Toolkit
# Written by Sander Canisius <S.V.M.Canisius@uvt.nl>
#

import logging
import popen2


def execute(cmd, logger=logging.getLogger(), logLevel=logging.DEBUG):
	# TODO: rewrite using the subprocess module
	proc = popen2.Popen4(cmd)
	line = proc.fromchild.readline()
	while line:
		logger.log(logLevel, line.rstrip())
		line = proc.fromchild.readline()

	return proc.wait()
