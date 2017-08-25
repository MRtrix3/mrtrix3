MRtrix3 module for motion and distortion correction.

Currently available:
-   Command `dwirecon` that interpolates a SH volume from scattered slices with given motion parameters.
-   Accounts for DWI gradient orientation (provided per volume or per slice).
-   Script `dwimotioncorrect` iterates between recon and registration for motion correction.

TODO:
-   Accounting for exact PSF
-   Accounting for field map and eddy current distortion

