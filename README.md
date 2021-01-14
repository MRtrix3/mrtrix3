# MRtrix3 module for motion and distortion correction.

This module contains the SHARD reconstruction software for slice-level motion 
correction in multi-shell diffusion MRI, as described in

Daan Christiaens, Lucilio Cordero-Grande, Maximilian Pietsch, Jana Hutter, Anthony N. Price, Emer J. Hughes, Katy Vecchiato, Maria Deprez, A. David Edwards, Joseph V.Hajnal, and J-Donald Tournier, *Scattered slice SHARD reconstruction for motion correction in multi-shell diffusion MRI*, NeuroImage 117437 (2020). doi:[10.1016/j.neuroimage.2020.117437](https://doi.org/10.1016/j.neuroimage.2020.117437)


## Setup & build

Prerequisites: MRtrix 3.0, Eigen 3.3.3, and python 3.x with numpy and scipy.

The code is built like any other [MRtrix3](https://github.com/MRtrix3/mrtrix3) 
module, i.e., by setting up a symbolic link to the core build script:

```
$ git clone https://github.com/dchristiaens/shard-recon.git
$ cd shard-recon
$ ln -s /path/to/mrtrix3/build
$ ln -s /path/to/mrtrix3/bin/mrtrix3.py bin/
$ ./build
```

This will compile the code into `shard-recon/bin`, which then needs to be added 
to the `PATH`.


## Help & support

Contact daan.christiaens@kcl.ac.uk


