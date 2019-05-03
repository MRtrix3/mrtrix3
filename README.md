# MRtrix3 module for motion and distortion correction.

This module contains a *beta*-release of the SHARD reconstruction software for 
slice-level motion correction in multi-shell diffusion MRI.


## Setup & build

The code is built like any other [MRtrix3](https://github.com/MRtrix3/mrtrix3) 
module, i.e., by setting up a symbolic link to the core build script:

```
$ git clone https://gitlab.com/ChD/shard-recon
$ cd shard-recon
$ ln -s /path/to/mrtrix3/build
$ ./build
```

This will compile the code into `shard-recon/bin`, which then needs to be added 
to the `PATH`.

Note: For this to work, the code needs to compiled against the current MRtrix3 
`master` branch, configured with Eigen 3.3.3. 


## Help & support

Contact daan.christiaens@kcl.ac.uk
