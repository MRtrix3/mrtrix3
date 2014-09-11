#!/bin/bash

output=$1
echo "ROI, SNR, smoothing, E, H, C, IQR25, AUC, IQR75" > $output

for ROI in arcuate uncinate CC cingulum posterior_cingulum all;
do
  for s in 1 2 5;
  do
    for smooth in 0 5 10 20;
    do
      for E in 0.5 1 2 3;
      do
        for H in 0.5 1 2 3;
        do
          for C in 0 0.25 0.5 0.75 1;
          do
              echo "${ROI}, ${s}, ${smooth}, ${E}, ${H}, ${C}, `cat /data/dave/cfe/experiment_2_sims/artificial2/${ROI}/IQR25/ROC_s${smooth}_snr${s}_h${H}_e${E}_c${C}`, `cat /data/dave/cfe/experiment_2_sims/artificial2/${ROI}/mean/ROC_s${smooth}_snr${s}_h${H}_e${E}_c${C}`, `cat /data/dave/cfe/experiment_2_sims/artificial2/${ROI}/IQR75/ROC_s${smooth}_snr${s}_h${H}_e${E}_c${C}`" >> $output
          done;
        done;
      done;
    done;
  done;
done;
