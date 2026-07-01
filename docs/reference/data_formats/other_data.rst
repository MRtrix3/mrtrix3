.. _other_data:

Other data
==========

Beyond :ref:`image data <image_handling>` and :ref:`tractography data
<tractography_data>`, *MRtrix3* reads and writes a number of other forms of
data from and to the filesystem: numerical tables, geometric and structural
data, and various tabular metadata. This page describes those forms.


.. _numerical_text_files:

Numerical text files
--------------------

Much of the tabular numerical data handled by *MRtrix3* — matrices, vectors,
and the many quantities derived from them — is stored in a common,
human-readable text format. A file consists of rows of whitespace- or
delimiter-separated numerical values, one matrix row per line.

-  **Delimiter on write**: the delimiter used when writing is inferred from the
   filename extension: ``.csv`` selects a comma, ``.tsv`` selects a tab, and any
   other extension (including ``.txt``) selects a space.

-  **Delimiter on read**: import is delimiter-agnostic, so that files produced
   by other software are readily accepted. Any of space, comma, semicolon or tab
   is accepted as a separator, irrespective of the filename extension, and
   repeated separators are collapsed.

-  **Comments and provenance**: the ``#`` character introduces a comment; all
   text from the first ``#`` on a line to the end of that line is ignored when
   reading the numerical data. This mechanism doubles as a generic metadata
   channel: writers emit ``# key: value`` comment lines, and — unless explicitly
   disabled — automatically append a ``# command_history: ...`` line recording
   the command invocation that produced the file. This provides the same
   provenance tracking for text files as the ``command_history``
   :ref:`header key-value pair <header_keyvalue_pairs>` provides for images.

.. _numpy_files:

NumPy arrays (``.npy``)
'''''''''''''''''''''''

Wherever a numerical matrix or vector is read or written, the
`NumPy <https://numpy.org/>`__ ``.npy`` array format may be used in place of a
text file, simply by giving the argument a ``.npy`` extension. On reading, the
array data are memory-mapped, both C-order and Fortran-order storage are
supported, and the array's ``dtype`` is honoured. This binary format avoids the
precision loss and parsing cost of a text representation for large arrays.

.. NOTE::
  Support for the compressed, multi-array `NumPy <https://numpy.org/>`__ ``.npz``
  container is planned, and will become available once related feature
  developments are merged.


Transformation matrices
-----------------------

Linear (affine) spatial transformations are stored as numerical text files
(using the :ref:`format above <numerical_text_files>`), as a 4×4 matrix; when
writing, the redundant final row ``0 0 0 1`` is always included. An optional
centre of rotation / scaling associated with the transformation is round-tripped
as a ``# centre: x y z`` comment line. These files are produced and consumed by
commands such as :ref:`mrregister`, :ref:`mrtransform` and :ref:`transformcalc`.


Diffusion gradient tables
-------------------------

The diffusion sensitisation (gradient) scheme associated with a DWI series may
be supplied or exported as an external file in either of two conventions:

-  **MRtrix format** (``.b``): a single text file with one row per volume, each
   row being ``[ x y z b ]`` — a unit direction vector followed by the
   *b*-value. Specified via the ``-grad`` option, and exported via
   ``-export_grad_mrtrix``. This same table may alternatively be embedded within
   an image header under the ``dw_scheme`` key (see :ref:`embedded_dw_scheme`).

-  **FSL format** (``bvecs`` / ``bvals``): a *pair* of files, one holding the
   3×N direction components and the other the 1×N *b*-values. Specified via the
   ``-fslgrad`` option, and exported via ``-export_grad_fsl``. Because the FSL
   convention encodes directions relative to the image axes, the associated
   image is required in order to interpret the directions unambiguously.


Phase encoding tables
---------------------

The phase-encoding scheme of an acquisition may be imported or exported in
several interchangeable representations:

-  an **MRtrix phase-encoding table**: a text file with one row per volume,
   ``[ dir_x dir_y dir_z total_readout_time ]`` (``-import_pe_table`` /
   ``-export_pe_table``);

-  the **FSL** ``topup`` **format**: a text phase-encoding table
   (``-import_pe_topup`` / ``-export_pe_topup``);

-  the **FSL** ``eddy`` **format**: a *pair* of files (a configuration file plus
   a per-volume index file) (``-import_pe_eddy`` / ``-export_pe_eddy``).


Direction sets
--------------

Sets of directions on the sphere (as used for diffusion gradient design and
related tasks) are stored as numerical text files, in either of two
representations: spherical coordinates as ``[ azimuth inclination ]`` pairs (two
columns), or Cartesian ``[ x y z ]`` 3-vectors (three columns). These are read
by auto-detecting the number of columns, and the on-write representation may be
selected explicitly. Such files are used by commands including :ref:`dirgen`,
:ref:`dirflip`, :ref:`dirorder` and :ref:`dirstat`.


Response functions
------------------

The response function(s) used for spherical deconvolution are stored as
numerical text matrices (using the :ref:`format above <numerical_text_files>`),
with one row per unique *b*-value shell and one column per even zonal spherical
harmonic degree. These are produced by response-function estimation commands and
consumed by :ref:`dwi2fod` and related commands.


JSON sidecar files (``.json``)
------------------------------

Image metadata — in particular `BIDS <https://bids.neuroimaging.io/>`__ fields
such as phase-encoding and slice-timing information — may be imported from or
exported to a JSON sidecar file. The ``-json_import`` and ``-json_export``
options (of :ref:`mrconvert` and :ref:`mrinfo`) carry these key-value data
between an image header and an external ``.json`` file.


Connectome node lookup tables
-----------------------------

A connectome node lookup table maps integer node indices to anatomical names and
(optionally) colours. These are read-only text files, and *MRtrix3* recognises
several dialects — a basic form, the FreeSurfer ``FreeSurferColorLUT.txt`` form,
the AAL form, the ITK-SNAP form, and an MRtrix form — automatically detecting
which is in use. As with other text files, ``#`` introduces a comment. Lookup
tables are used by commands such as :ref:`labelconvert`, :ref:`label2colour`,
:ref:`label2mesh` and :ref:`tck2connectome`.

.. NOTE::
  The inter-regional connectivity **matrices** themselves (connectomes) are
  stored using the generic :ref:`numerical text file <numerical_text_files>`
  format; there is no bespoke connectome container format.


Surface data
------------

*MRtrix3* represents anatomical surfaces as polygonal meshes (a set of vertices
and the faces connecting them), and can attach per-vertex scalar data to them.

-  **Meshes** may be read from VTK PolyData (``.vtk``), STL (``.stl``) and
   Wavefront OBJ (``.obj``) files, as well as from FreeSurfer binary surface
   geometry files (recognised as the fallback when the extension is none of the
   above). Writing is supported to ``.vtk``, ``.stl`` and ``.obj`` (the
   FreeSurfer geometry format is read-only). Meshes are used by commands such as
   :ref:`label2mesh`, :ref:`mesh2voxel`, :ref:`meshconvert` and :ref:`voxel2mesh`.

-  **Surface scalar** (per-vertex overlay) data may be read from a plain text
   vector, or from FreeSurfer ``w``-files or ``curv``-files; on writing, only the
   plain numerical text representation is produced.

-  **FreeSurfer annotation** (``.annot``) and **label** (``.label``) files are
   supported for reading: the former encodes a per-vertex parcellation together
   with an embedded colour lookup table, the latter a list of vertices with
   associated scalar values.


Statistical subject lists
-------------------------

The group-level statistics commands accept a plain text file that lists the
input data for a cohort, one filename per line (blank lines are skipped). Each
listed file is then loaded as the data for one subject. The design and contrast
matrices supplied to these commands are ordinary :ref:`numerical text
<numerical_text_files>` matrices.


Fixel directory format
----------------------

Fixel data (fibre-bundle elements within voxels) are stored not as a single file
but as a *directory* containing an index image, a directions image, and one or
more per-fixel data images. Its constituent files are ordinary
:ref:`image <supported_image_formats>` files (``.mif`` / ``.nii``, optionally
compressed). This format is documented in full at :ref:`fixel_format`.


Configuration file
------------------

*MRtrix3* reads its behaviour-modifying settings from a configuration file in
the same ``key: value`` text format as the image header. This is a read-only
data source (from the software's perspective), searched for at a system-wide and
a per-user location. Its available keys are documented at
:ref:`config_file_options`.
