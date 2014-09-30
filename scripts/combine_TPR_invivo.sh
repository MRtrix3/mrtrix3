#!/bin/bash

output=$1
echo "ROI, effect, smoothing, E, H, C, IQR25, AUC, IQR75" > $output

for ROI in arcuate CC;
do
  for effect in 0.2 0.3 0.45;
  do
    for smooth in 10;
    do
      for E in 0.5 1 2 3;
      do
        for H in 0.5 1 2 3;
        do
          for C in 0 0.25 0.5 0.75 1;
          do
              echo "${ROI}, ${effect}, ${smooth}, ${E}, ${H}, ${C}, `cat /data/dave/cfe/experiment_2_sims/invivo/${ROI}/TPR05_IQR25/ROC_s${smooth}_effect${effect}_h${H}_e${E}_c${C}`, `cat /data/dave/cfe/experiment_2_sims/invivo/${ROI}/TPR05_mean/ROC_s${smooth}_effect${effect}_h${H}_e${E}_c${C}`, `cat /data/dave/cfe/experiment_2_sims/invivo/${ROI}/TPR05_IQR75/ROC_s${smooth}_effect${effect}_h${H}_e${E}_c${C}`" >> $output
          done;
        done;
      done;
    done;
  done;
done;
