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

The -coord option is used to select the coordinates within the input image that are to be retained in the output image. This can therefore be used to include or exclude subsets of slices along a particular spatial axis, or volumes / series within higher dimensions. For instance: -coord 3 0 extracts the first volume from a 4D image; -coord 1 24 extracts slice number 24 along the y-axis.

The colon operator can be particularly useful in conjunction with the -coord option, in order to select multiple coordinates. For instance: -coord 3 1:59 would select all but the first volume from an image containing 60 volumes.

The -vox option is used to change the size of the voxels in the output image. Note that this does not re-sample the image based on a new voxel size (that is done using the mrresize command); this only changes the voxel size as reported in the image header. Voxel sizes for individual axes can be set independently, using a comma-separated list of values; e.g. -vox 1,,3.5 will change the voxel size along the x & z axes to 1.0mm and 3.5mm respectively, and leave the y-axis voxel size unchanged.

The -axes option specifies which axes from the input image will be used to form the output image. This allows the permutation, omission, or addition of axes into the output image. The axes should be supplied as a comma-separated list of axis indices, e.g. -axes 0,1,2 would select only the three spatial axes to form the output image. If an axis from the input image is to be omitted from the output image, it must have dimension 1; either in the input image itself, or a single coordinate along that axis must be selected by the user by using the -coord option. An axis of unity dimension can be inserted by supplying -1 at the corresponding position in the list.

The -scaling option specifies the data scaling parameters stored within the image header that are used to rescale the image intensity values. Where the raw data stored in a particular voxel is I, the value within that voxel is interpreted as: value = offset + (scale x I). To adjust this scaling, the relevant parameters must be provided as a comma-separated 2-vector of floating-point values, in the format "offset,scale" (no quotation marks).

By default, the intensity scaling parameters in the input image header are passed through to the output image header when writing to an integer image, and reset to 0,1 (i.e. no scaling) for floating-point and binary images. Note that the -scaling option will therefore have no effect for floating-point or binary output images.

Note that for both the -coord and -axes options, indexing starts from 0 rather than 1. E.g. -coord 3 <#> selects volumes (the fourth dimension) from the series; -axes 0,1,2 includes only the three spatial axes in the output image.

Options
-------

Options for manipulating fundamental image properties
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-coord axis coord** retain data from the input image only at the coordinates specified

-  **-vox sizes** change the voxel dimensions of the output image

-  **-axes axes** specify the axes from the input image that will be used to form the output image

-  **-scaling values** specify the data scaling parameters used to rescale the intensity values

Options for handling JSON (JavaScript Object Notation) files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-json_import file** import data from a JSON file into header key-value pairs

-  **-json_export file** export data from an image header key-value pairs into a JSON file

Options to modify generic header entries
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-clear_property key** remove the specified key from the image header altogether.

-  **-set_property key value** set the value of the specified key in the image header.

-  **-append_property key value** append the given value to the specified key in the image header (this adds the value specified as a new line in the header value).

Stride options
^^^^^^^^^^^^^^

-  **-stride spec** specify the strides of the output data in memory, as a comma-separated list. The actual strides produced will depend on whether the output image format can support it.

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

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

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


