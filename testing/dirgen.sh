MRTRIX_RNG_SEED=1 dirgen 20 tmp.txt -force && diff tmp.txt dirgen/dir20.txt
MRTRIX_RNG_SEED=7 dirgen 22 -unipolar tmp.txt -force && diff tmp.txt dirgen/dir22_unipolar.txt
MRTRIX_RNG_SEED=12 dirgen 25 -cartesian tmp.txt -force && diff tmp.txt dirgen/dir25_cartesian.txt
