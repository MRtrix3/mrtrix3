#!/usr/bin/python
import numpy
from sys import argv

aucs = numpy.loadtxt(argv[1])
output_file = open(argv[3], 'w')
output_file.write("%f\n" % numpy.percentile(aucs, float(argv[2])))
output_file.close()
