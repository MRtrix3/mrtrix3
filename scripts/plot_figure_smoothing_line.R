require(ggplot2)
require(grid)

mf_labeller <- function(var, value){ 
  value <- as.character(value)
  if (var=="effect") {
    value[value=="0.1"]   <- "Effect = 10%"
    value[value=="0.2"]   <- "Effect = 20%"
    value[value=="0.3"]   <- "Effect = 30%"
  }
  if (var=="smoothing") {
    value[value=="0"]   <- "Smoothing = 0mm"
    value[value=="5"]   <- "Smoothing = 5mm"
    value[value=="10"]   <- "Smoothing = 10mm"
    value[value=="20"]   <- "Smoothing = 20mm"
  }
  if (var=="ROI") {
    value[value=="arcuate"]   <- "Arcuate"
    value[value=="cingulum"]   <- "Cingulum"
    value[value=="posterior_cingulum"]   <- "Post Cingulum"
    value[value=="cst"]   <- "Corticospinal"
    value[value=="ad"]   <- "Alzheimer's-like"
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
ppi <- 400
png("/home/dave/Gdrive/Documents/JournalPapers/CFE/Figures/invivo2/AUC_cst_smoothing.png", width=7*ppi, height=4*ppi, res = ppi)
all_data <- read.csv('/data/dave/cfe/experiment_2_sims/invivo2/aucdata.csv');
sub <- subset(all_data, ROI == 'cst' & C == 0 & effect != 0.4, select = c (smoothing, ROI, effect, C, E, H, IQR25, AUC, IQR75))
sub$H <- factor(sub$H)
print(ggplot(data=sub,  aes(x=E, y=AUC)) + geom_ribbon(aes(ymin=IQR25, ymax=IQR75 ,fill=H), alpha=0.2)+ geom_line(aes(colour=H)) + ylim(0,1) + facet_grid(effect ~ smoothing, labeller = mf_labeller) + theme(plot.margin = unit(c(0,0,0,0),'mm'))) 
dev.off()
