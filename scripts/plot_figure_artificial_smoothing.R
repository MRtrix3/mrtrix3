require(ggplot2)
require(grid)

mf_labeller <- function(var, value){ 
  value <- as.character(value)
  if (var=="SNR") {
    value[value=="1"]   <- "SNR=1"
    value[value=="2"]   <- "SNR=2"
    value[value=="3"]   <- "SNR=3"
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



ppi <- 800
png("/home/dave/Gdrive/Documents/JournalPapers/CFE/Figures/Artificial/AUC_arcuate_smoothing.png", width=7*ppi, height=4.55*ppi, res = ppi)
all_data <- read.csv('/raid1/CFE_backup/aucdata.csv');
sub <- subset(all_data, (SNR == 1 | SNR == 2 | SNR == 3) & ROI == 'arcuate' & C == 0.5, select = c (smoothing, ROI, SNR, C, E, H, IQR25, AUC, IQR75))
sub$H <- factor(sub$H)
sub$E <- factor(sub$E)
print(ggplot(data=sub,  aes(x=E, y=H)) + geom_tile(aes(fill = AUC), colour = "white") 
      + scale_fill_gradientn(colours=c("#352a87","#3340b3","#0c5dde","#066fdf","#127cd7","#118ad2","#069ccf","#06a7c3","#15b0b4","#33b8a0","#5abd8a","#80bf79","#a5be6a","#c4bb5d","#e2b951","#fabb40","#fbce2d","#f4e11e","#f8fa0d"), breaks=seq(0,1,by=0.2), limits=c(0, 1)) 
      + facet_grid(SNR ~ smoothing, labeller = mf_labeller) 
      + theme(panel.grid.major = element_blank(), panel.grid.minor = element_blank(), plot.margin = unit(c(2,0,0,0),'mm'),panel.background = element_blank(), axis.line = element_line(colour = "white"))
      + scale_x_discrete(expand = c(0, 0), labels=c('','1','','2','','3','','4','','5','','6')) + scale_y_discrete(expand = c(0, 0), labels=c('','1','','2','','3','','4','','5','','6')))
dev.off()
