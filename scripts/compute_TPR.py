#!/usr/bin/python
import numpy
from sys import argv


# Compute the TPR at FPR = 0.05
def compute_tpr_05(TPR, FPR):
    i = 0
    while FPR[i] < 0.05:
        i = i + 1                           
    FPR_05 = FPR[0:i+1]
    TPR_05 = TPR[0:i+1]                     
    if FPR[i] != 0.05:
        # interpolate TPR at FPR 0.05
        y = TPR[i-1] + (TPR[i] - TPR[i-1])*((0.05 - FPR[i-1])/(FPR[i] - FPR[i-1]))
        TPR_05 = numpy.append(TPR_05, y)
    return TPR_05[-1]


if __name__ == "__main__":
    all_TPR = numpy.loadtxt(argv[1])
    average_TPR_FPR = numpy.loadtxt(argv[2])
    FPR = average_TPR_FPR[::-1,1]
    output_file = open(argv[3], 'w')
    print argv[3]
    for realisation in all_TPR.transpose():
        TPR = realisation[::-1]
        TPR_05 = compute_tpr_05(TPR, FPR)
        output_file.write("%f\n" % TPR_05)
    output_file.close()
                    
