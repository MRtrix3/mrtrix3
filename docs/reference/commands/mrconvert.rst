.. _mrconvert:

mrconvert
===================

Synopsis
--------

Perform conversion between different file types and optionally extract a subset of the input image

Usage
--------

::

    mrconvert [ options ]  input output

-  *input*: the input image.
-  *output*: the output image.

Description
-----------

If used correctly, this program can be a very useful workhorse. In addition to converting images between different formats, it can be used to extract specific studies from a data set, extract a specific region of interest, or flip the images. Some of the possible operations are described in more detail below.

Note that for both the -coord and -axes options, indexing starts from 0 rather than 1. E.g. -coord 3 <#> selects volumes (the fourth dimension) from the series; -axes 0,1,2 includes only the three spatial axes in the output image.

Additionally, for the second input to the -coord option and the -axes option, you can use any valid number sequence in the selection, as well as the 'end' keyword (see the main documentation for details); this can be particularly useful to select multiple coordinates.

The -vox option is used to change the size of the voxels in the output image as reported in the image header; note however that this does not re-sample the image based on a new voxel size (that is done using the mrgrid command).

By default, the intensity scaling parameters in the input image header are passed through to the output image header when writing to an integer image, and reset to 0,1 (i.e. no scaling) for floating-point and binary images. Note that the -scaling option will therefore have no effect for floating-point or binary output images.

The -axes option specifies which axes from the input image will be used to form the output image. This allows the permutation, omission, or addition of axes into the output image. The axes should be supplied as a comma-separated list of axis indices. If an axis from the input image is to be omitted from the output image, it must either already have a size of 1, or a single coordinate along that axis must be selected by the user by using the -coord option. Examples are provided further below.

The -bvalue_scaling option controls an aspect of the import of diffusion gradient tables. When the input diffusion-weighting direction vectors have norms that differ substantially from unity, the b-values will be scaled by the square of their corresponding vector norm (this is how multi-shell acquisitions are frequently achieved on scanner platforms). However in some rare instances, the b-values may be correct, despite the vectors not being of unit norm (or conversely, the b-values may need to be rescaled even though the vectors are close to unit norm). This option allows the user to control this operation and override MRrtix3's automatic detection.

Example usages
--------------

-   *Extract the first volume from a 4D image, and make the output a 3D image*::

        $ mrconvert in.mif -coord 3 0 -axes 0,1,2 out.mif

    The -coord 3 0 option extracts, from axis number 3 (which is the fourth axis since counting begins from 0; this is the axis that steps across image volumes), only coordinate number 0 (i.e. the first volume). The -axes 0,1,2 ensures that only the first three axes (i.e. the spatial axes) are retained; if this option were not used in this example, then image out.mif would be a 4D image, but it would only consist of a single volume, and mrinfo would report its size along the fourth axis as 1.

-   *Extract slice number 24 along the AP direction*::

        $ mrconvert volume.mif slice.mif -coord 1 24

    MRtrix3 uses a RAS (Right-Anterior-Superior) axis convention, and internally reorients images upon loading in order to conform to this as far as possible. So for non-exotic data, axis 1 should correspond (approximately) to the anterior-posterior direction.

-   *Extract only every other volume from a 4D image*::

        $ mrconvert all.mif every_other.mif -coord 3 1:2:end

    This example demonstrates two features: Use of the colon syntax to conveniently specify a number sequence (in the format 'start:step:stop'); and use of the 'end' keyword to generate this sequence up to the size of the input image along that axis (i.e. the number of volumes).

-   *Alter the image header to report a new isotropic voxel size*::

        $ mrconvert in.mif isotropic.mif -vox 1.25

    By providing a single value to the -vox option only, the specified value is used to set the voxel size in mm for all three spatial axes in the output image.

-   *Alter the image header to report a new anisotropic voxel size*::

        $ mrconvert in.mif anisotropic.mif -vox 1,,3.5

    This example will change the reported voxel size along the first and third axes (ideally left-right and inferior-superior) to 1.0mm and 3.5mm respectively, and leave the voxel size along the second axis (ideally anterior-posterior) unchanged.

-   *Turn a single-volume 4D image into a 3D image*::

        $ mrconvert 4D.mif 3D.mif -axes 0,1,2

    Sometimes in the process of extracting or calculating a single 3D volume from a 4D image series, the size of the image reported by mrinfo will be "X x Y x Z x 1", indicating that the resulting image is in fact also 4D, it just happens to contain only one volume. This example demonstrates how to convert this into a genuine 3D image (i.e. mrinfo will report the size as "X x Y x Z".

-   *Insert an axis of size 1 into the image*::

        $ mrconvert XYZD.mif XYZ1D.mif -axes 0,1,2,-1,3

    This example uses the value -1 as a flag to indicate to mrconvert where a new axis of unity size is to be inserted. In this particular example, the input image has four axes: the spatial axes X, Y and Z, and some form of data D is stored across the fourth axis (i.e. volumes). Due to insertion of a new axis, the output image is 5D: the three spatial axes (XYZ), a single volume (the size of the output image along the fourth axis will be 1), and data D will be stored as volume groups along the fifth axis of the image.

-   *Manually reset the data scaling parameters stored within the image header to defaults*::

        $ mrconvert with_scaling.mif without_scaling.mif -scaling 0.0,1.0

    This command-line option alters the parameters stored within the image header that provide a linear mapping from raw intensity values stored in the image data to some other scale. Where the raw data stored in a particular voxel is I, the value within that voxel is interpreted as: value = offset + (scale x I).  To adjust this scaling, the relevant parameters must be provided as a comma-separated 2-vector of floating-point values, in the format "offset,scale" (no quotation marks). This particular example sets the offset to zero and the scale to one, which equates to no rescaling of the raw intensity data.

Options
-------

Options for manipulating fundamental image properties
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-coord axis selection** *(multiple uses permitted)* retain data from the input image only at the coordinates specified in the selection along the specified axis. The selection argument expects a number sequence, which can also include the 'end' keyword.

-  **-vox sizes** change the voxel dimensions reported in the output image header

-  **-axes axes** specify the axes from the input image that will be used to form the output image

-  **-scaling values** specify the data scaling parameters used to rescale the intensity values

Options for handling JSON (JavaScript Object Notation) files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-json_import file** import data from a JSON file into header key-value pairs

-  **-json_export file** export data from an image header key-value pairs into a JSON file

Options to modify generic header entries
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-clear_property key** *(multiple uses permitted)* remove the specified key from the image header altogether.

-  **-set_property key value** *(multiple uses permitted)* set the value of the specified key in the image header.

-  **-append_property key value** *(multiple uses permitted)* append the given value to the specified key in the image header (this adds the value specified as a new line in the header value).

-  **-copy_properties source** clear all generic properties and replace with the properties from the image / file specified.

Stride options
^^^^^^^^^^^^^^

-  **-strides spec** specify the strides of the output data in memory; either as a comma-separated list of (signed) integers, or as a template image from which the strides shall be extracted and used. The actual strides produced will depend on whether the output image format can support it.

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-bvalue_scaling mode** enable or disable scaling of diffusion b-values by the square of the corresponding DW gradient norm (see Desciption). Valid choices are yes/no, true/false, 0/1 (default: automatic).

DW gradient table export options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-export_grad_mrtrix path** export the diffusion-weighted gradient table to file in MRtrix format

-  **-export_grad_fsl bvecs_path bvals_path** export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format

Options for importing phase-encode tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-import_pe_table file** import a phase-encoding table from file

-  **-import_pe_eddy config indices** import phase-encoding information from an EDDY-style config / index file pair

Options for exporting phase-encode tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-export_pe_table file** export phase-encoding table to file

-  **-export_pe_eddy config indices** export phase-encoding information to an EDDY-style config / index file pair

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2023 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Covered Software is provided under this License on an "as is"
basis, without warranty of any kind, either expressed, implied, or
statutory, including, without limitation, warranties that the
Covered Software is free of defects, merchantable, fit for a
particular purpose or non-infringing.
See the Mozilla Public License v. 2.0 for more details.

For more details, see http://www.mrtrix.org/.


