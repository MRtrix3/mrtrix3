#!/usr/bin/python
import numpy
from sys import argv
from scipy.integrate import simps, trapz


# Compute the AUC for all FPR < 0.05
def compute_auc(TPR, FPR):
    i = 0
    while FPR[i] < 0.05:
        i = i + 1                           
    FPR_05 = FPR[0:i+1]
    TPR_05 = TPR[0:i+1]                     
    if FPR[i] != 0.05:
        # interpolate TPR at FPR 0.05
        y = TPR[i-1] + (TPR[i] - TPR[i-1])*((0.05 - FPR[i-1])/(FPR[i] - FPR[i-1]))
        TPR_05 = numpy.append(TPR_05, y)
        FPR_05 = numpy.append(FPR_05, 0.05)                           
    return trapz(TPR_05, x=FPR_05) * 20


if __name__ == "__main__":
    all_TPR = numpy.loadtxt(argv[1])
    average_TPR_FPR = numpy.loadtxt(argv[2])
    FPR = average_TPR_FPR[::-1,1]
    output_file = open(argv[3], 'w')
    print argv[3]

    temp = compute_auc(average_TPR_FPR[::-1,0], FPR)
    print(temp)
    for realisation in all_TPR.transpose():
        TPR = realisation[::-1]
        AUC = compute_auc(TPR, FPR)
        output_file.write("%f\n" % AUC)
    output_file.close()
                    
