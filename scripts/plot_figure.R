require(ggplot2)

mf_labeller <- function(var, value){ 
  value <- as.character(value)
  if (var=="SNR") {
    value[value=="1"]   <- "SNR=1"
    value[value=="2"]   <- "SNR=2"
    value[value=="5"]   <- "SNR=5"
  }
  if (var=="C") {
    value[value=="0"]   <- "C=0"
    value[value=="0.25"]   <- "C=0.25"
    value[value=="0.5"]   <- "C=0.5"
    value[value=="0.75"]   <- "C=0.75"
    value[value=="1"]   <- "C=1"
  }
  return(value)
}

setwd('/home/dave/dev/fixel_based_stats/scripts/')
source('multiplot.R')
png("/home/dave/Gdrive/Documents/JournalPapers/CFE/Figures/AUC_uncinate_smooth10.png", width=180, height=230, units = 'mm', res = 350)
all_data <- read.csv('/data/dave/cfe/experiment_2_sims/artificial2/all_data.csv');
sub <- subset(all_data, ROI == 'uncinate' & smoothing == 10, select = c (SNR, C, E, H, IQR25, AUC, IQR75))
sub$H <- factor(sub$H)
print(ggplot(data=sub,  aes(x=E, y=AUC)) + geom_ribbon(aes(ymin=IQR25, ymax=IQR75 ,fill=H), alpha=0.2)+ geom_line(aes(colour=H)) + ylim(0,1) + facet_grid(C ~ SNR, labeller = mf_labeller)) 
dev.off()
