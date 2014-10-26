#!/bin/bash

output=$1
echo "ROI, effect, smoothing, E, H, C, IQR25, AUC, IQR75" > $output

for ROI in ad arcuate cst cingulum posterior_cingulum;
do
  echo $ROI
  for effect in 0.1 0.2 0.3;
  do
    for smooth in 0 5 10 20;
    do
      for E in 0.5 1 2 3 4 5 6;
        do
        for H in 0.5 1 2 3 4 5 6;
        do
          for C in 0 0.25 0.5 0.75 1;
          do
          if [ -f /data/dave/cfe/experiment_2_sims/invivo2/results/${ROI}/IQR25/effect${effect}_s${smooth}_c${C}_h${H}_e${E} ];
           then
              echo "${ROI}, ${effect}, ${smooth}, ${E}, ${H}, ${C}, `cat /data/dave/cfe/experiment_2_sims/invivo2/results/${ROI}/IQR25/effect${effect}_s${smooth}_c${C}_h${H}_e${E}`, `cat /data/dave/cfe/experiment_2_sims/invivo2/results/${ROI}/mean/effect${effect}_s${smooth}_c${C}_h${H}_e${E}`, `cat /data/dave/cfe/experiment_2_sims/invivo2/results/${ROI}/IQR75/effect${effect}_s${smooth}_c${C}_h${H}_e${E}`" >> $output
           fi;
          done;
        done;
      done;
    done;
  done;
done;
