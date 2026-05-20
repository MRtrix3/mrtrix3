.. _axis_realignment:

Axis realignment at image load
==============================

*MRtrix3* adopts a single canonical coordinate convention for image data,
referred to as **RAS**: the positive direction of the first image axis is
closest to subject **R**\ ight, the positive direction of the second axis is
closest to subject **A**\ nterior, and the positive direction of the third
axis is closest to subject **S**\ uperior. Any image whose stored axes do
*not* approximately conform to this convention is silently permuted and / or
flipped at load time so that the loaded image *appears* axial without
moving any voxel intensities.

This page describes that mechanism, what it changes, what it does **not**
change, how to inspect the on-disk versus interpreted values, and how to
disable it. The realignment itself is performed by
``Header::realign_transform`` (``cpp/core/header.cpp``) when
``Header::open`` is called on a non-RAS source.

Why "closest to"?
-----------------

The language used in the introduction above,
where each image axis is described as being "closest to" an anatomical direction,
is highly deliberate.
When an image is described as being "RAS",
this does **not** guarantee that the direction in space traversed by first image axis
corresponds *exactly* to anatomical right
(similarly for the other two spatial axes).
It only means that
---assuming the patient orientation was entered correctly at the scanner console---
the anatomical axis that the positive direction of the first image axis
*most closely* corresponds to is Right.

This might be expressed symbolically as:

.. math:

    +i \approx Right
    +j \approx Anterior
    +k \approx Superior

, where *i*, *j* and *k* are used to symbolise the three spatial image axes,
as is used in the Brain Imaging Data Structure (BIDS).
Indeed, intentionally disambiguating between image axes *i*, *j* and *k*
versus anatomical directions *L* / *R* / *A* / *P* / *I*  *S*
frequently assists in communicating and understanding ideas
relating to spatial transformations such as in this page.

What realignment changes
------------------------

When the on-disk image is not approximately RAS, MRtrix3 modifies the
following fields *in the in-memory header only*. The voxel intensities on
disk are never touched.

-  The 4x4 affine **transform** matrix is rotated by a permutation of its
   columns and / or sign-flips, so the leading 3x3 block is as close to the
   identity matrix as can be achieved through those operations
   (ie. image axes *i*, *j*, *k* are as close to anatomical R, A, S
   as can be achieved using axis permutations and flips only).
-  The image **strides** are permuted and / or sign-flipped to match
   the new axis ordering, so iterating axes 0 → 1 → 2 approximately traverses
   anatomical R → A → S.
-  Axis-dependent **metadata** that encode directions or per-slice
   ordering with respect to the image axes are reoriented to remain valid:

   -  ``pe_scheme`` and the BIDS-style ``PhaseEncodingDirection`` field
      (phase encoding direction is expressed in image-axis space;
      see :ref:`pe_scheme`).
   -  ``SliceEncodingDirection`` and ``SliceTiming``
      (slice-ordering direction and per-slice timing offsets).

-  Axis-dependent metadata that arrive *after* image open via
   ``-json_import`` (or the equivalent JSON sidecar accompanying a NIfTI)
   are subjected to the same transformation as was applied to the corresponding image,
   to preserve correspondence between image data and metadata.

What is **not** realignment
---------------------------

Some other transformations occur at load time and are sometimes confused
with realignment, but are independent:

-  **FSL bvec import.** The FSL `bvecs` / `bvals` convention stores
   diffusion gradient directions as cosine angles relative to *image axes*
   (with a caveat; see :ref:`dw_scheme`).
   This contrasts with the MRtrix3 convention
   (for both storage in header metadata (``dw_scheme``)
   / text files / internal computations),
   where the gradient table contains cosine angles relative to the physical
   axes of the scanner (so-called *real-space* or *scanner-space* coordinates).
   When a gradient table is read by MRtrix3 via ``-fslgrad``,
   it is immediately transformed into scanner-space,
   with the inverse transform applied when writing out FSL bvecs (``-export_grad_fsl``).
   This conversion is performed for every image
   regardless of whether any axis realignment was applied,
   as it is a fundamental contrast between format conventions.
   See :ref:`dw_scheme` for more detail on gradient-table conventions.
-  **NIfTI write-time axis shuffling.** When writing to a NIfTI file from
   a header whose strides are not RAS, MRtrix3 may shuffle the spatial
   axes on write so that the image intensity data appear in the same order
   in which they are encoded within the internal representation
   of the image being written.
   This is because unlike the MRtrix image formats,
   the NIfTI format does not support arbitrary axis strides.
   This is a separate write-side step,
   though may modify the same set of metadata fields
   as can be affected by the load-time realignment.

What realignment does **not** change
-------------------------------------

-  The bytes of the image file on disk.
-  Quantitative voxel values
-  Diffusion gradient magnitudes
-  Metadata that are either scalar, or that are not explicitly programmed
   to possess an encoding related to image axes.
-  The total number of axes (``ndim``) or the size of any axis.
-  The voxel spacing along each (post-permutation) image axis.

Internal convention for the applied shuffle
-------------------------------------------

Internally, the applied shuffle is stored as a triplet of permutation indices
and a triplet of boolean flips.
The convention is:

-  The flip is applied **first**, to the source-axis columns of the
   original transform (``flips[i]`` is indexed by **source** axis).
-  The permutation is then applied as a column rearrangement
   (``permutations[i]`` is the **source** axis index placed at output
   position ``i``).

The user-facing description, produced by ``mrinfo`` and exported through
``-json_all``, does not require the reader to know this convention: it
enumerates per output axis the source axis it came from and whether the
sign was reversed, e.g.::

   Axes realignment:
     output axis 0 (~R) <- source axis 2, sign reversed
     output axis 1 (~A) <- source axis 0, sign preserved
     output axis 2 (~S) <- source axis 1, sign reversed

How to see the on-disk values
-----------------------------

``mrinfo`` by default prints the *interpreted* (post-realignment) view of an image.
When realignment is non-identity,
the default output additionally annotates the affected lines inline::

   $ mrinfo dwi_sag.nii.gz
   ...
   Data strides:      [ 1 2 3 4 ]    (on-disk: [ 3 -1 -2 4 ])
   Transform:                0.9999       ...
                             ...
     On-disk transform:      0.07944      ...
                             ...
     Axes realignment:
                     output axis 0 (~R) <- source axis 2, sign reversed
                     output axis 1 (~A) <- source axis 0, sign preserved
                     output axis 2 (~S) <- source axis 1, sign reversed
                     (disable with -config RealignTransform false)
   ...

Two new options on ``mrinfo`` give scriptable access:

-  ``-realignment`` prints the per-output-axis enumeration above (and
   nothing if the image was not realigned).
-  ``-ondisk`` modifies ``-transform``, ``-strides``, ``-petable``, and
   ``-property`` so they report the pre-realignment values, e.g.::

      $ mrinfo dwi_sag.nii.gz -property PhaseEncodingDirection
      i-
      $ mrinfo dwi_sag.nii.gz -property PhaseEncodingDirection -ondisk
      j-

For machine consumption, ``mrinfo -json_all out.json`` includes a
top-level ``realignment`` object whenever realignment was non-identity,
containing ``permutations``, ``flips``, ``axis_mapping``,
``transform_on_disk``, ``strides_on_disk`` and ``keyval_on_disk``
(the subset of keyvals whose values differ between interpreted and on-disk).

When to worry
-------------

There are three common situations where realignment can mislead an
unsuspecting user:

#. **Comparing PhaseEncodingDirection across tools.** Running
   ``mrinfo image.nii.gz -property PhaseEncodingDirection`` and reading
   the same field from a sibling JSON file may produce *different*
   strings. The JSON contains the value as it sits on disk; ``mrinfo``
   reports it relative to the realigned (interpreted) axes.

#. **Piping NIfTI → MRtrix → NIfTI.**
   Realignment information is stored only in the in-memory representation;
   it is not encoded in the metadata of any image on disk.
   Once an image has been written to any file format,
   the next process to read it sees the realigned axes
   as if they were the on-disk axes.
   ``mrinfo`` executed on a ``.mif`` written from a non-axial NIfTI
   will therefore show *no* realignment / on-disk annotation:
   the ``.mif`` format faithfully stores the arbitrary strides
   that resulted from the prior axis realignment,
   and so the resulting image is already approximately RAS.

#. **Mixing -json_import with non-corresponding images.**
   Using ``-json_import`` in conjunction with an image
   to which realignment was applied *by a previous command*
   will **fail** to reorient the JSON's axis-dependent fields
   to conform to the prior realignment of that image.
   Only pass ``-json_import`` when the metadata file
   is a direct sidecar of the source image.
   A benefit of utilising the MRtrix image formats throughout a pipeline
   is that associated metadata embedded in the image header
   should have any requisite conversions or transformations
   to accompany an image operation applied automatically.
   See :ref:`pe_scheme` for a worked example.

How to disable
--------------

Two scopes:

-  **Disable the realignment entirely.**
   Set ``RealignTransform: false`` in your MRtrix3 configuration file,
   or pass ``-config RealignTransform false`` on the command line.
   The image is then loaded
   with on-disk axes / strides / transform / metadata unchanged.
   Beware that downstream MRtrix3 commands then receive data
   that may not be in the expected canonical orientation;
   this has consequences for:

   -   Tools that assume RAS-relative coordinates
       (for instance the manual ``-pe_dir`` argument to ``dwifslpreproc``);
   -   Commands that receive as input multiple images
       that must reside on the same image voxel gradient
       to facilitate per-voxel operations across them,
       but those images do not all already possess the same strides.

-  **Disable only the on-load notification.**
   Set ``RealignmentVerbose: false`` in your configuration file,
   or pass ``-config RealignmentVerbose false`` on the command line.
   The realignment itself still occurs;
   only the default-visible console line is suppressed.

See also
--------

-  :ref:`image_handling` — coordinate system, transform and strides.
-  :ref:`pe_scheme` — phase encoding scheme storage and interaction with
   realignment.
-  :ref:`dw_scheme` — diffusion gradient table conventions, including
   the FSL bvec ↔ MRtrix scanner-space conversion.
-  ``-config RealignTransform`` / ``-config RealignmentVerbose``
   in :ref:`config_file_options`.
