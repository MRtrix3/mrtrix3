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

What realignment changes
------------------------

When the on-disk image is not approximately RAS, MRtrix3 modifies the
following fields *in the in-memory header only*. The voxel intensities on
disk are never touched.

-  The 4x4 affine **transform** matrix is rotated by a permutation of its
   columns and / or sign-flips, so the leading 3x3 block is close to the
   identity in the RAS sense.
-  The image **strides** are permuted (and possibly sign-flipped) to match
   the new axis ordering, so iterating axis 0 → 1 → 2 traverses R → A → S
   relative to the scanner.
-  Axis-dependent **metadata** that encode directions or per-slice
   ordering with respect to the image axes are reoriented to remain valid:

   -  ``pe_scheme`` and the BIDS-style ``PhaseEncodingDirection`` field
      (phase encoding direction expressed in image-axis space; see
      :ref:`pe_scheme`).
   -  ``SliceEncodingDirection`` and ``SliceTiming`` (slice-ordering
      direction and per-slice timing offsets).

-  Axis-dependent metadata that arrive *after* image open via
   ``-json_import`` (or the equivalent JSON sidecar accompanying a NIfTI)
   are transformed in the same way on import, so they remain consistent
   with the realigned image axes.

What is **not** realignment
---------------------------

Some other transformations occur at load time and are sometimes confused
with realignment, but are independent:

-  **FSL bvec ↔ MRtrix gradient table conversion.** FSL stores
   diffusion gradient directions in *image-axis space* by convention,
   whereas MRtrix3 stores the gradient table (``dw_scheme``) in
   *scanner-space* coordinates. When a gradient table is read via
   ``-fslgrad`` it is multiplied by the on-disk image-to-scanner linear
   transform to enter scanner space, and an inverse transform is applied
   when writing out FSL bvecs. This conversion is performed for every
   image regardless of whether realignment was applied, and is
   intentionally silent: it is a convention bridge, not a side-effect of
   realignment. See :ref:`dw_scheme` for the gradient-table conventions.
-  **NIfTI write-time axis shuffling.** When writing to a NIfTI file from
   a header whose strides are not RAS, MRtrix3 may shuffle the spatial
   axes on write in order to fit within the NIfTI format's limited
   stride support. This is a separate, write-side step and may modify
   the same fields independently of the load-time realignment.
-  **Voxel data.** Realignment never modifies the voxel intensity array.

What realignment does **not** change
-------------------------------------

-  The bytes of the image file on disk.
-  Quantitative voxel values, gradient magnitudes, or any scalar metadata.
-  The total number of axes (``ndim``) or the size of any axis.
-  The voxel spacing along each (post-permutation) image axis.

Internal convention for the applied shuffle
-------------------------------------------

Internally, the applied shuffle is stored as a permutation triple and a
flip triple. The convention is:

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
     output axis 0 (R) <- source axis 2, sign reversed
     output axis 1 (A) <- source axis 0, sign preserved
     output axis 2 (S) <- source axis 1, sign reversed

How to see the on-disk values
-----------------------------

``mrinfo`` by default prints the *interpreted* (post-realignment) view.
When realignment is non-identity, the default output additionally
annotates the affected lines inline::

   $ mrinfo dwi_sag.nii.gz
   ...
   Data strides:      [ 1 2 3 4 ]    (on-disk: [ 3 -1 -2 4 ])
   Transform:                0.9999       ...
                             ...
     On-disk transform:      0.07944      ...
                             ...
     Axes realignment:
                     output axis 0 (R) <- source axis 2, sign reversed
                     output axis 1 (A) <- source axis 0, sign preserved
                     output axis 2 (S) <- source axis 1, sign reversed
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
``transform_on_disk``, ``strides_on_disk`` and ``keyval_on_disk`` (the
subset of keyvals whose values differ between interpreted and on-disk).

When to worry
-------------

There are three common situations where realignment can mislead an
unsuspecting user:

#. **Comparing PhaseEncodingDirection across tools.** Running
   ``mrinfo image.nii.gz -property PhaseEncodingDirection`` and reading
   the same field from a sibling JSON file may produce *different*
   strings. The JSON contains the value as it sits on disk; ``mrinfo``
   reports it relative to the realigned (interpreted) axes.

#. **Piping NIfTI → MRtrix → NIfTI.** Realignment information is stored
   only in the in-memory ``Header``. Once an image has been written to
   any file format, the next process to read it sees the realigned axes
   as if they were the on-disk axes. ``mrinfo`` on a ``.mif`` written
   from a non-axial NIfTI will therefore show *no* on-disk annotation,
   because by construction ``.mif`` stores arbitrary strides faithfully
   and the new load is identity-realignment.

#. **Mixing -json_import with intermediate piped images.** Using
   ``-json_import`` against an intermediate piped image to which
   realignment has already been applied will reorient the JSON's
   axis-dependent fields *as if they referred to the realigned axes*,
   which is typically not what was intended. Always pass
   ``-json_import`` together with the original source image. See
   :ref:`pe_scheme` for a worked example.

How to disable
--------------

Two scopes:

-  **Disable the realignment entirely.** Set ``RealignTransform: false``
   in your MRtrix3 configuration file, or pass ``-config RealignTransform
   false`` on the command line. The image is then loaded with on-disk
   axes / strides / transform / metadata unchanged. Beware that
   downstream MRtrix3 commands then receive data that may not be in the
   expected canonical orientation, which has consequences for tools that
   assume RAS-relative coordinates (for instance the manual
   PhaseEncodingDirection argument to ``dwifslpreproc``).

-  **Disable only the on-load notification.** Set
   ``RealignmentVerbosity: quiet`` in your configuration file, or
   pass ``-config RealignmentVerbosity quiet`` on the command line. The
   realignment itself still occurs; only the default-visible console
   line is suppressed. Useful in large pipelines where every image is
   known to be non-axial.

See also
--------

-  :ref:`image_data` — coordinate system, transform and strides.
-  :ref:`pe_scheme` — phase encoding scheme storage and interaction with
   realignment.
-  :ref:`dw_scheme` — diffusion gradient table conventions, including
   the FSL bvec ↔ MRtrix scanner-space conversion.
-  ``-config RealignTransform`` / ``-config RealignmentVerbosity``
   in :ref:`config_file_options`.
