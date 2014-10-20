require(ggplot2)
require(grid)

mf_labeller <- function(var, value){ 
  value <- as.character(value)
  if (var=="effect") {
    value[value=="0.1"]   <- "effect=10%"
    value[value=="0.2"]   <- "effect=20%"
    value[value=="0.3"]   <- "effect=30%"
  }
  if (var=="ROI") {
    value[value=="arcuate"]   <- "Arcuate"
    value[value=="cingulum"]   <- "Cingulum"
    value[value=="posterior_cingulum"]   <- "Post Cing"
    value[value=="cst"]   <- "Corticospinal"
    value[value=="ad"]   <- "Alzheimers"
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
png("/home/dave/Gdrive/Documents/JournalPapers/CFE/Figures/invivo2/AUC_heat_effect1_smooth10.png", width=7*ppi, height=7*ppi, res = ppi)
all_data <- read.csv('/data/dave/cfe/experiment_2_sims/invivo2/aucdata.csv');
sub <- subset(all_data, effect == 0.1 & smoothing == 10, select = c (smoothing, ROI, effect, C, E, H, IQR25, AUC, IQR75))
sub$H <- factor(sub$H)
sub$E <- factor(sub$E)
sub$ROI <- factor(sub$ROI, levels = c("arcuate", "cst", "cingulum", "posterior_cingulum", "ad"))

print(ggplot(data=sub,  aes(x=E, y=H)) + geom_tile(aes(fill = AUC), colour = "white") 
      + scale_fill_gradientn(colours=c("black","blue","cyan","yellow","red")) 
      + facet_grid(ROI ~ C, labeller = mf_labeller) 
      + theme(panel.grid.major = element_blank(), panel.grid.minor = element_blank(), plot.margin = unit(c(2,0,0,0),'mm'),panel.background = element_blank(), axis.line = element_line(colour = "white"))                                                                                                                                                                                                           
      + scale_x_discrete(expand = c(0, 0)) + scale_y_discrete(expand = c(0, 0) ))

dev.off()
