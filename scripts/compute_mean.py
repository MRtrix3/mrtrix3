#!/usr/bin/python
import numpy
from sys import argv

aucs = numpy.loadtxt(argv[1])
output_file = open(argv[2], 'w')
output_file.write("%f\n" % numpy.mean(aucs))
output_file.close()
