.. _data_formats:

Data formats
============

*MRtrix3* is designed to interoperate with a wide range of data, and provides a
flexible input/output back-end shared by all of its applications. This means
that any *MRtrix3* command can read or write data in any of the supported
formats, with no need to explicitly convert data to a particular format before
processing: the format is recognised automatically, most commonly from the
filename extension.

This page provides a high-level overview of the kinds of data *MRtrix3* works
with and the formats in which they may be stored. Full detail on each is
provided on the dedicated pages:

-  :ref:`image_handling` — the volumetric image formats;
-  :ref:`tractography_data` — streamline tractography and its sidecar data;
-  :ref:`other_data` — numerical tables, transforms, surfaces and other data.


Image data
----------

An image is a multi-dimensional array of voxel values sampled on a regular grid.
Storing an image faithfully requires more than the raw voxel values alone: the
format must also record

-  the **data type** of the stored values (for example 16-bit integer or 32-bit
   floating-point, and the byte ordering), so that the binary data can be
   interpreted correctly and an appropriate precision preserved; and

-  the **transform** (a 4×4 affine matrix), which locates and orients the voxel
   grid within real (scanner) space, so that the image can be related
   consistently to other images and to spatial data such as streamlines.

The MRtrix image formats additionally allow arbitrary further metadata (such as
the diffusion gradient table) to be embedded in the image header, which is one
of the main reasons for their existence. The formats differ in which of these
capabilities they support; see :ref:`image_handling` for full detail, including
the list of supported :ref:`data types <data_types>`, the :ref:`image
transform <transform>` and :ref:`strides`.

.. csv-table::
  :header: "Format", "Extension", "Synopsis"

  ":ref:`MRtrix image <mrtrix_image_formats>`", "``.mif`` / ``.mih``", "Native format with a human-readable header able to carry arbitrary metadata and arbitrary data ordering."
  "Compressed MRtrix", "``.mif.gz``", "As above, compressed at the cost of runtime and memory."
  ":ref:`DICOM <dicom_format>`", "folder / ``.dcm``", "Scanner-native format, supported for reading only."
  ":ref:`NIfTI <nifti_format>`", "``.nii`` / ``.nii.gz``", "Widely used neuroimaging format for interoperation with other packages."
  ":ref:`FreeSurfer <mgh_formats>`", "``.mgh`` / ``.mgz``", "FreeSurfer image format, able to carry FreeSurfer-specific metadata."
  ":ref:`Analyse <analyze_format>`", "``.img`` / ``.hdr``", "Deprecated format; written as NIfTI instead."


Tractography data
-----------------

A *streamline* is an ordered sequence of 3D vertices approximating a path
through space, and a *tractogram* is a collection of streamlines. Streamlines
may be accompanied by *sidecar* data: quantitative values associated either with
each streamline (data per streamline) or with each vertex (data per vertex),
stored either embedded within the tractogram dataset or in a separate external
file. The formats differ in whether they store geometry losslessly, whether they
can carry sidecar data, and other capabilities; see :ref:`tractography_data` for
full detail and a capability comparison.

.. csv-table::
  :header: "Format", "Extension", "Synopsis"

  ":ref:`MRtrix tracks <mrtrix_tracks_format>`", "``.tck``", "Native lossless streamline format, storing vertex coordinates only."
  "QFib / ZFIB", "``.qfib`` / ``.zfib``", "Lossily compressed streamline formats."
  "TrackVis", "``.trk``", "Common interchange format able to carry sidecar data."
  "TRX", "``.trx``", "Modern container carrying geometry plus arbitrary sidecar data and a grid transform."
  "DSI Studio TinyTrack", "``.tt``", "Grid-quantised streamline format within a MATLAB container."
  "VTK", "``.vtk`` / ``.vtx``", "Polygonal-line geometry, optionally with scalar sidecar arrays (``.vtk``)."
  ":ref:`Track scalar file <mrtrix_scalar_track_format>`", "``.tsf``", "External per-vertex sidecar data for a matching ``.tck``."


Other data
----------

*MRtrix3* also interacts with a range of tabular and structural data. Most
numerical data (matrices and vectors, and quantities derived from them such as
transforms, gradient tables and response functions) share a common
human-readable :ref:`numerical text format <numerical_text_files>`, with the
`NumPy <https://numpy.org/>`__ ``.npy`` binary array format available as an
alternative. See :ref:`other_data` for full detail.

.. csv-table::
  :header: "Data", "Format", "Synopsis"

  ":ref:`Numerical tables <numerical_text_files>`", "``.csv`` / ``.tsv`` / ``.txt``", "Human-readable matrices and vectors; delimiter set by extension; ``#`` comments carry provenance."
  ":ref:`NumPy array <numpy_files>`", "``.npy``", "Binary numerical array, usable wherever a text table is."
  "Diffusion gradient table", "``.b`` / ``bvecs`` + ``bvals``", "DWI diffusion sensitisation scheme (MRtrix or FSL convention)."
  ":ref:`JSON sidecar <other_data>`", "``.json``", "BIDS image metadata for import / export."
  "Connectome lookup table", "text", "Maps node indices to anatomical names and colours (read-only)."
  "Surface mesh", "``.vtk`` / ``.stl`` / ``.obj``", "Anatomical surfaces as vertices and faces."
  ":ref:`Fixel directory <fixel_format>`", "directory", "Fibre-bundle-element data stored as a directory of images."
  ":ref:`Configuration <config_file_options>`", "``mrtrix.conf``", "User- and system-level settings (read-only)."
