#!/bin/python
import numpy
from pylab import *
from scipy.integrate import simps, trapz

def write_file (filename, data):
    f = open(filename, 'w')
    f.write("%f\n" % data)
    f.close()

# A function to read in a file, and compute the AUC for all FPR < 0.05
def compute_auc(effect, smooth, h, e, c, prefix):
    data = numpy.loadtxt(prefix + str(smooth) + '_snr' + str(effect) + '_h' + str(h) + '_e' + str(e) + '_c' + str(c))
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
for smooth in ['0','5','10','20']:
    for h in ['0.5','1','2','3']:
        for e in ['0.5','1','2','3']:
            for c in ['0','0.25','0.5','0.75','1']:                       
                for effect in ['0.15','0.3','0.45']:
                  uncinate = compute_auc(effect,smooth, h, e, c, '../ROIs/uncinate/output/ROC_s');
                  write_file('/data/dave/cfe/experiment_2_sims/invivo/uncinate/AUC/' + str(effect) + '_' + str(smooth) + '_h' + str(h) + '_e' + str(e) + '_c' + str(c), uncinate)
                  arcuate = compute_auc(effect,smooth, h, e, c, '../ROIs/arcuate/output/ROC_s');
                  write_file('/data/dave/cfe/experiment_2_sims/invivo/arcuate/AUC/' + str(effect) + '_' + str(smooth) + '_h' + str(h) + '_e' + str(e) + '_c' + str(c), arcuate)
                  cingulum = compute_auc(effect,smooth, h, e, c, '../ROIs/cingulum/output/ROC_s');
                  write_file('/data/dave/cfe/experiment_2_sims/invivo/cingulum/AUC/' + str(effect) + '_' + str(smooth) + '_h' + str(h) + '_e' + str(e) + '_c' + str(c), cingulum)
                  posterior_cingulum = compute_auc(effect,smooth, h, e, c, '../ROIs/posterior_cingulum/output/ROC_s');
                  write_file('/data/dave/cfe/experiment_2_sims/invivo/posterior_cingulum/AUC/' + str(effect) + '_' + str(smooth) + '_h' + str(h) + '_e' + str(e) + '_c' + str(c), posterior_cingulum)
                  CC = compute_auc(effect,smooth, h, e, c, '../ROIs/CC/output/ROC_s');
                  write_file('/data/dave/cfe/experiment_2_sims/ROIs/CC/AUC/' + str(effect) + '_' + str(smooth) + '_h' + str(h) + '_e' + str(e) + '_c' + str(c), CC)
                  average = (uncinate + arcuate + cingulum + posterior_cingulum + CC ) / 5.0  
                  write_file ('/data/dave/cfe/experiment_2_sims/averageAUCs/' + str(effect) + '_' + str(smooth) + '_h' + str(h) + '_e' + str(e) + '_c' + str(c), average)


                      
                    
