#!/bin/python
import numpy
from scipy.integrate import simps, trapz

def write_file (filename, data):
    f = open(filename, 'w')
    f.write("%f\n" % data)
    f.close()

# A function to read in a file, and compute the AUC for all FPR < 0.05
def compute_auc(effect, smooth, h, e, c, prefix):
    data = numpy.loadtxt(prefix + '_s' + str(smooth) + '_effect' + str(effect) + '_h' + str(h) + '_e' + str(e) + '_c' + str(c))
    FPR = data[::-1,1]
    TPR = data[::-1,0]                                   
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


# loop over all parameter combinations, compute AUC and average across ROIs

effect = '0.3'
smooth = '10'
h = '2'
e = '1'
c = '0.5'
for instance in ['1','2','3','4','5']:
    uncinate = compute_auc (effect,smooth, h, e, c, '/data/dave/cfe/experiment_2_sims/invivo/reproducibility/roc' + instance);
    write_file('/data/dave/cfe/experiment_2_sims/invivo/reproducibility/auc' + instance + '_effect' + str(effect) + '_s' + str(smooth) + '_h' + str(h) + '_e' + str(e) + '_c' + str(c), uncinate)



                      
                    
