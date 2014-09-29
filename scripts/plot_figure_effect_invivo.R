require(ggplot2)

mf_labeller <- function(var, value){ 
  value <- as.character(value)
  if (var=="effect") {
    value[value=="0.1"]   <- "effect=10%"
    value[value=="0.2"]   <- "effect=20%"
    value[value=="0.3"]   <- "effect=30%"
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
png("/home/dave/Gdrive/Documents/JournalPapers/CFE/Figures/invivo2/AUC_effect1_smooth10.png", width=200, height=150, units = 'mm', res = 350)
all_data <- read.csv('/data/dave/cfe/experiment_2_sims/invivo2/aucdata.csv');
sub <- subset(all_data, effect == 0.2 & ROI == 'cst', select = c (smoothing, ROI, effect, C, E, H, IQR25, AUC, IQR75))
sub$C <- factor(sub$C)
print(ggplot(data=sub,  aes(x=E, y=AUC)) + geom_ribbon(aes(ymin=IQR25, ymax=IQR75 ,fill=C), alpha=0.2)+ geom_line(aes(colour=C)) + ylim(0,0.4) + facet_grid(smoothing ~ H, labeller = mf_labeller)) 
dev.off()
