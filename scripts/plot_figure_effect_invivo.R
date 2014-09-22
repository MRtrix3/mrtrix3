require(ggplot2)

mf_labeller <- function(var, value){ 
  value <- as.character(value)
  if (var=="effect") {
    value[value=="0.15"]   <- "effect=15%"
    value[value=="0.3"]   <- "effect=30%"
    value[value=="0.45"]   <- "effect=45%"
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
png("/home/dave/Gdrive/Documents/JournalPapers/CFE/Figures/invivo/AUC_effect6_smooth10.png", width=200, height=150, units = 'mm', res = 350)
all_data <- read.csv('/data/dave/cfe/experiment_2_sims/invivo/aucdata.csv');
sub <- subset(all_data, effect == 0.6 & smoothing == 10, select = c (ROI, effect, C, E, H, IQR25, AUC, IQR75))
sub$H <- factor(sub$H)
print(ggplot(data=sub,  aes(x=E, y=AUC)) + geom_ribbon(aes(ymin=IQR25, ymax=IQR75 ,fill=H), alpha=0.2)+ geom_line(aes(colour=H)) + ylim(0,1) + facet_grid(ROI ~ C, labeller = mf_labeller)) 
dev.off()
