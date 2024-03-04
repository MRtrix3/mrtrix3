
.. _list-of-mrtrix3-commands:

########################
List of MRtrix3 commands
########################




.. toctree::
    :hidden:

    commands/5tt2gmwmi
    commands/5tt2vis
    commands/5ttcheck
    commands/5ttedit
    commands/5ttgen
    commands/afdconnectivity
    commands/amp2response
    commands/amp2sh
    commands/connectome2tck
    commands/connectomeedit
    commands/connectomestats
    commands/dcmedit
    commands/dcminfo
    commands/dirflip
    commands/dirgen
    commands/dirmerge
    commands/dirorder
    commands/dirsplit
    commands/dirstat
    commands/dwi2adc
    commands/dwi2fod
    commands/dwi2mask
    commands/dwi2response
    commands/dwi2tensor
    commands/dwibiascorrect
    commands/dwibiasnormmask
    commands/dwicat
    commands/dwidenoise
    commands/dwiextract
    commands/dwifslpreproc
    commands/dwigradcheck
    commands/dwinormalise
    commands/dwishellmath
    commands/fixel2peaks
    commands/fixel2sh
    commands/fixel2tsf
    commands/fixel2voxel
    commands/fixelcfestats
    commands/fixelconnectivity
    commands/fixelconvert
    commands/fixelcorrespondence
    commands/fixelcrop
    commands/fixelfilter
    commands/fixelreorient
    commands/fod2dec
    commands/fod2fixel
    commands/for_each
    commands/label2colour
    commands/label2mesh
    commands/labelconvert
    commands/labelsgmfirst
    commands/labelstats
    commands/mask2glass
    commands/maskdump
    commands/maskfilter
    commands/mesh2voxel
    commands/meshconvert
    commands/meshfilter
    commands/mraverageheader
    commands/mrcalc
    commands/mrcat
    commands/mrcentroid
    commands/mrcheckerboardmask
    commands/mrclusterstats
    commands/mrcolour
    commands/mrconvert
    commands/mrdegibbs
    commands/mrdump
    commands/mredit
    commands/mrfilter
    commands/mrgrid
    commands/mrhistmatch
    commands/mrhistogram
    commands/mrinfo
    commands/mrmath
    commands/mrmetric
    commands/mrregister
    commands/mrstats
    commands/mrthreshold
    commands/mrtransform
    commands/mrtrix_cleanup
    commands/mrview
    commands/mtnormalise
    commands/peaks2amp
    commands/peaks2fixel
    commands/population_template
    commands/responsemean
    commands/sh2amp
    commands/sh2peaks
    commands/sh2power
    commands/sh2response
    commands/shbasis
    commands/shconv
    commands/shview
    commands/tck2connectome
    commands/tck2fixel
    commands/tckconvert
    commands/tckdfc
    commands/tckedit
    commands/tckgen
    commands/tckglobal
    commands/tckinfo
    commands/tckmap
    commands/tckresample
    commands/tcksample
    commands/tcksift
    commands/tcksift2
    commands/tckstats
    commands/tcktransform
    commands/tensor2metric
    commands/transformcalc
    commands/transformcompose
    commands/transformconvert
    commands/tsfdivide
    commands/tsfinfo
    commands/tsfmult
    commands/tsfsmooth
    commands/tsfthreshold
    commands/tsfvalidate
    commands/vectorstats
    commands/voxel2fixel
    commands/voxel2mesh
    commands/warp2metric
    commands/warpconvert
    commands/warpcorrect
    commands/warpinit
    commands/warpinvert


.. csv-table::
    :header: "Lang", "Command", "Synopsis"

    |cpp.png|, :ref:`5tt2gmwmi`, "Generate a mask image appropriate for seeding streamlines on the grey matter-white matter interface"
    |cpp.png|, :ref:`5tt2vis`, "Generate an image for visualisation purposes from an ACT 5TT segmented anatomical image"
    |cpp.png|, :ref:`5ttcheck`, "Thoroughly check that one or more images conform to the expected ACT five-tissue-type (5TT) format"
    |cpp.png|, :ref:`5ttedit`, "Manually set the partial volume fractions in an ACT five-tissue-type (5TT) image using mask images"
    |python.png|, :ref:`5ttgen`, "Generate a 5TT image suitable for ACT"
    |cpp.png|, :ref:`afdconnectivity`, "Obtain an estimate of fibre connectivity between two regions using AFD and streamlines tractography"
    |cpp.png|, :ref:`amp2response`, "Estimate response function coefficients based on the DWI signal in single-fibre voxels"
    |cpp.png|, :ref:`amp2sh`, "Convert a set of amplitudes (defined along a set of corresponding directions) to their spherical harmonic representation"
    |cpp.png|, :ref:`connectome2tck`, "Extract streamlines from a tractogram based on their assignment to parcellated nodes"
    |cpp.png|, :ref:`connectomeedit`, "Perform basic operations on a connectome"
    |cpp.png|, :ref:`connectomestats`, "Connectome group-wise statistics at the edge level using non-parametric permutation testing"
    |cpp.png|, :ref:`dcmedit`, "Edit DICOM file in-place"
    |cpp.png|, :ref:`dcminfo`, "Output DICOM fields in human-readable format"
    |cpp.png|, :ref:`dirflip`, "Invert the polarity of individual directions so as to optimise a unipolar electrostatic repulsion model"
    |cpp.png|, :ref:`dirgen`, "Generate a set of uniformly distributed directions using a bipolar electrostatic repulsion model"
    |cpp.png|, :ref:`dirmerge`, "Splice / merge multiple sets of directions in such a way as to maintain near-optimality upon truncation"
    |cpp.png|, :ref:`dirorder`, "Reorder a set of directions to ensure near-uniformity upon truncation"
    |cpp.png|, :ref:`dirsplit`, "Split a set of evenly distributed directions (as generated by dirgen) into approximately uniformly distributed subsets"
    |cpp.png|, :ref:`dirstat`, "Report statistics on a direction set"
    |cpp.png|, :ref:`dwi2adc`, "Convert mean dwi (trace-weighted) images to mean ADC maps"
    |cpp.png|, :ref:`dwi2fod`, "Estimate fibre orientation distributions from diffusion data using spherical deconvolution"
    |python.png|, :ref:`dwi2mask`, "Generate a binary mask from DWI data"
    |python.png|, :ref:`dwi2response`, "Estimate response function(s) for spherical deconvolution"
    |cpp.png|, :ref:`dwi2tensor`, "Diffusion (kurtosis) tensor estimation"
    |python.png|, :ref:`dwibiascorrect`, "Perform B1 field inhomogeneity correction for a DWI volume series"
    |python.png|, :ref:`dwibiasnormmask`, "Perform a combination of bias field correction, intensity normalisation, and mask derivation, for DWI data"
    |python.png|, :ref:`dwicat`, "Concatenating multiple DWI series accounting for differential intensity scaling"
    |cpp.png|, :ref:`dwidenoise`, "dMRI noise level estimation and denoising using Marchenko-Pastur PCA"
    |cpp.png|, :ref:`dwiextract`, "Extract diffusion-weighted volumes, b=0 volumes, or certain shells from a DWI dataset"
    |python.png|, :ref:`dwifslpreproc`, "Perform diffusion image pre-processing using FSL's eddy tool; including inhomogeneity distortion correction using FSL's topup tool if possible"
    |python.png|, :ref:`dwigradcheck`, "Check the orientation of the diffusion gradient table"
    |python.png|, :ref:`dwinormalise`, "Perform various forms of intensity normalisation of DWIs"
    |python.png|, :ref:`dwishellmath`, "Apply an mrmath operation to each b-value shell in a DWI series"
    |cpp.png|, :ref:`fixel2peaks`, "Convert data in the fixel directory format into a 4D image of 3-vectors"
    |cpp.png|, :ref:`fixel2sh`, "Convert a fixel-based sparse-data image into an spherical harmonic image"
    |cpp.png|, :ref:`fixel2tsf`, "Map fixel values to a track scalar file based on an input tractogram"
    |cpp.png|, :ref:`fixel2voxel`, "Convert a fixel-based sparse-data image into some form of scalar image"
    |cpp.png|, :ref:`fixelcfestats`, "Fixel-based analysis using connectivity-based fixel enhancement and non-parametric permutation testing"
    |cpp.png|, :ref:`fixelconnectivity`, "Generate a fixel-fixel connectivity matrix"
    |cpp.png|, :ref:`fixelconvert`, "Convert between the old format fixel image (.msf / .msh) and the new fixel directory format"
    |cpp.png|, :ref:`fixelcorrespondence`, "Obtain fixel-fixel correpondence between a subject fixel image and a template fixel mask"
    |cpp.png|, :ref:`fixelcrop`, "Crop/remove fixels from sparse fixel image using a binary fixel mask"
    |cpp.png|, :ref:`fixelfilter`, "Perform filtering operations on fixel-based data"
    |cpp.png|, :ref:`fixelreorient`, "Reorient fixel directions"
    |cpp.png|, :ref:`fod2dec`, "Generate FOD-based DEC maps, with optional panchromatic sharpening and/or luminance/perception correction"
    |cpp.png|, :ref:`fod2fixel`, "Perform segmentation of continuous Fibre Orientation Distributions (FODs) to produce discrete fixels"
    |python.png|, :ref:`for_each`, "Perform some arbitrary processing step for each of a set of inputs"
    |cpp.png|, :ref:`label2colour`, "Convert a parcellated image (where values are node indices) into a colour image"
    |cpp.png|, :ref:`label2mesh`, "Generate meshes from a label image"
    |cpp.png|, :ref:`labelconvert`, "Convert a connectome node image from one lookup table to another"
    |python.png|, :ref:`labelsgmfirst`, "In a FreeSurfer parcellation image, replace the sub-cortical grey matter structure delineations using FSL FIRST"
    |cpp.png|, :ref:`labelstats`, "Compute statistics of parcels within a label image"
    |python.png|, :ref:`mask2glass`, "Create a glass brain from mask input"
    |cpp.png|, :ref:`maskdump`, "Print out the locations of all non-zero voxels in a mask image"
    |cpp.png|, :ref:`maskfilter`, "Perform filtering operations on 3D / 4D mask images"
    |cpp.png|, :ref:`mesh2voxel`, "Convert a mesh surface to a partial volume estimation image"
    |cpp.png|, :ref:`meshconvert`, "Convert meshes between different formats, and apply transformations"
    |cpp.png|, :ref:`meshfilter`, "Apply filter operations to meshes"
    |cpp.png|, :ref:`mraverageheader`, "Calculate the average (unbiased) coordinate space of all input images"
    |cpp.png|, :ref:`mrcalc`, "Apply generic voxel-wise mathematical operations to images"
    |cpp.png|, :ref:`mrcat`, "Concatenate several images into one"
    |cpp.png|, :ref:`mrcentroid`, "Determine the centre of mass / centre of gravity of an image"
    |cpp.png|, :ref:`mrcheckerboardmask`, "Create bitwise checkerboard image"
    |cpp.png|, :ref:`mrclusterstats`, "Voxel-based analysis using permutation testing and threshold-free cluster enhancement"
    |cpp.png|, :ref:`mrcolour`, "Apply a colour map to an image"
    |cpp.png|, :ref:`mrconvert`, "Perform conversion between different file types and optionally extract a subset of the input image"
    |cpp.png|, :ref:`mrdegibbs`, "Remove Gibbs Ringing Artifacts"
    |cpp.png|, :ref:`mrdump`, "Print out the values within an image"
    |cpp.png|, :ref:`mredit`, "Directly edit the intensities within an image from the command-line"
    |cpp.png|, :ref:`mrfilter`, "Perform filtering operations on 3D / 4D MR images"
    |cpp.png|, :ref:`mrgrid`, "Modify the grid of an image without interpolation (cropping or padding) or by regridding to an image grid with modified orientation, location and or resolution. The image content remains in place in real world coordinates."
    |cpp.png|, :ref:`mrhistmatch`, "Modify the intensities of one image to match the histogram of another"
    |cpp.png|, :ref:`mrhistogram`, "Generate a histogram of image intensities"
    |cpp.png|, :ref:`mrinfo`, "Display image header information, or extract specific information from the header"
    |cpp.png|, :ref:`mrmath`, "Compute summary statistic on image intensities either across images, or along a specified axis of a single image"
    |cpp.png|, :ref:`mrmetric`, "Computes a dissimilarity metric between two images"
    |cpp.png|, :ref:`mrregister`, "Register two images together using a symmetric rigid, affine or non-linear transformation model"
    |cpp.png|, :ref:`mrstats`, "Compute images statistics"
    |cpp.png|, :ref:`mrthreshold`, "Create bitwise image by thresholding image intensity"
    |cpp.png|, :ref:`mrtransform`, "Apply spatial transformations to an image"
    |python.png|, :ref:`mrtrix_cleanup`, "Clean up residual temporary files & scratch directories from MRtrix3 commands"
    |cpp.png|, :ref:`mrview`, "The MRtrix image viewer"
    |cpp.png|, :ref:`mtnormalise`, "Multi-tissue informed log-domain intensity normalisation"
    |cpp.png|, :ref:`peaks2amp`, "Extract amplitudes from a peak directions image"
    |cpp.png|, :ref:`peaks2fixel`, "Convert peak directions image to a fixel directory"
    |python.png|, :ref:`population_template`, "Generates an unbiased group-average template from a series of images"
    |python.png|, :ref:`responsemean`, "Calculate the mean response function from a set of text files"
    |cpp.png|, :ref:`sh2amp`, "Evaluate the amplitude of an image of spherical harmonic functions along specified directions"
    |cpp.png|, :ref:`sh2peaks`, "Extract the peaks of a spherical harmonic function in each voxel"
    |cpp.png|, :ref:`sh2power`, "Compute the total power of a spherical harmonics image"
    |cpp.png|, :ref:`sh2response`, "Generate an appropriate response function from the image data for spherical deconvolution"
    |cpp.png|, :ref:`shbasis`, "Examine the values in spherical harmonic images to estimate (and optionally change) the SH basis used"
    |cpp.png|, :ref:`shconv`, "Perform spherical convolution"
    |cpp.png|, :ref:`shview`, "View spherical harmonics surface plots"
    |cpp.png|, :ref:`tck2connectome`, "Generate a connectome matrix from a streamlines file and a node parcellation image"
    |cpp.png|, :ref:`tck2fixel`, "Compute a fixel TDI map from a tractogram"
    |cpp.png|, :ref:`tckconvert`, "Convert between different track file formats"
    |cpp.png|, :ref:`tckdfc`, "Perform the Track-Weighted Dynamic Functional Connectivity (TW-dFC) method"
    |cpp.png|, :ref:`tckedit`, "Perform various editing operations on track files"
    |cpp.png|, :ref:`tckgen`, "Perform streamlines tractography"
    |cpp.png|, :ref:`tckglobal`, "Multi-Shell Multi-Tissue Global Tractography"
    |cpp.png|, :ref:`tckinfo`, "Print out information about a track file"
    |cpp.png|, :ref:`tckmap`, "Map streamlines to an image, with various options for generating image contrast"
    |cpp.png|, :ref:`tckresample`, "Resample each streamline in a track file to a new set of vertices"
    |cpp.png|, :ref:`tcksample`, "Sample values of an associated image along tracks"
    |cpp.png|, :ref:`tcksift`, "Filter a whole-brain fibre-tracking data set such that the streamline densities match the FOD lobe integrals"
    |cpp.png|, :ref:`tcksift2`, "Optimise per-streamline cross-section multipliers to match a whole-brain tractogram to fixel-wise fibre densities"
    |cpp.png|, :ref:`tckstats`, "Calculate statistics on streamlines lengths"
    |cpp.png|, :ref:`tcktransform`, "Apply a spatial transformation to a tracks file"
    |cpp.png|, :ref:`tensor2metric`, "Generate maps of tensor-derived parameters"
    |cpp.png|, :ref:`transformcalc`, "Perform calculations on linear transformation matrices"
    |cpp.png|, :ref:`transformcompose`, "Compose any number of linear transformations and/or warps into a single transformation"
    |cpp.png|, :ref:`transformconvert`, "Convert linear transformation matrices"
    |cpp.png|, :ref:`tsfdivide`, "Divide corresponding values in track scalar files"
    |cpp.png|, :ref:`tsfinfo`, "Print out information about a track scalar file"
    |cpp.png|, :ref:`tsfmult`, "Multiply corresponding values in track scalar files"
    |cpp.png|, :ref:`tsfsmooth`, "Gaussian filter a track scalar file"
    |cpp.png|, :ref:`tsfthreshold`, "Threshold and invert track scalar files"
    |cpp.png|, :ref:`tsfvalidate`, "Validate a track scalar file against the corresponding track data"
    |cpp.png|, :ref:`vectorstats`, "Statistical testing of vector data using non-parametric permutation testing"
    |cpp.png|, :ref:`voxel2fixel`, "Map the scalar value in each voxel to all fixels within that voxel"
    |cpp.png|, :ref:`voxel2mesh`, "Generate a surface mesh representation from a voxel image"
    |cpp.png|, :ref:`warp2metric`, "Compute fixel-wise or voxel-wise metrics from a 4D deformation field"
    |cpp.png|, :ref:`warpconvert`, "Convert between different representations of a non-linear warp"
    |cpp.png|, :ref:`warpcorrect`, "Replaces voxels in a deformation field that point to a specific out of bounds location with nan,nan,nan"
    |cpp.png|, :ref:`warpinit`, "Create an initial warp image, representing an identity transformation"
    |cpp.png|, :ref:`warpinvert`, "Invert a non-linear warp field"


.. |cpp.png| image:: cpp.png
   :alt: C++

.. |python.png| image:: python.png
   :alt: Python

