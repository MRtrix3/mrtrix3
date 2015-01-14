#!/bin/bash

output=$1
echo "ROI, SNR, smoothing, E, H, C, IQR25, AUC, IQR75" > $output

for ROI in cst arcuate cingulum ad posterior_cingulum;
do
  for s in 1 2 3 5;
  do
    for smooth in 0 5 10 20;
    do
      for E in 0.5 1 1.5 2 2.5 3 3.5 4 4.5 5 5.5 6;
      do
        for H in 0.5 1 1.5 2 2.5 3 3.5 4 4.5 5 5.5 6;
        do
          for C in 0 0.25 0.5 0.75 1;
          do
            if [ -f /data/dave/cfe/experiment_2_sims/artificial/results/${ROI}/mean/snr${s}_s${smooth}_c${C}_h${H}_e${E} ];
             then
                echo "${ROI}, ${s}, ${smooth}, ${E}, ${H}, ${C}, `cat /data/dave/cfe/experiment_2_sims/artificial/results/${ROI}/IQR25/snr${s}_s${smooth}_c${C}_h${H}_e${E}`, `cat /data/dave/cfe/experiment_2_sims/artificial/results/${ROI}/mean/snr${s}_s${smooth}_c${C}_h${H}_e${E}`, `cat /data/dave/cfe/experiment_2_sims/artificial/results/${ROI}/IQR75/snr${s}_s${smooth}_c${C}_h${H}_e${E}`" >> $output
            fi;
          done;
        done;
      done;
    done;
  done;
done;
