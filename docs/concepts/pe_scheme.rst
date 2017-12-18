Phase encoding scheme handling
==============================

From version 3.0RC1 onwards, *MRtrix3* is capable of importing information from
DICOM relating to the phase encoding of the acquired images, and encoding this
information within key-value fields inside an image header. This information can
then later be used by the ``dwipreproc`` script, specifically using its
``-rpe_header`` command-line option, to automatically set up and execute FSL's
``topup`` and ``eddy`` commands without requiring explicit input from the user
regarding the phase encoding design of the imaging experiment. This page
explains how this information is encoded and manipulated by *MRtrix3*.


.. NOTE::

    Due to variations in sequences, acquisition protocols, DICOM encoding, and
    image manipulation operations, *MRtrix3* cannot be guaranteed to obtain or
    retain the phase encoding information correctly in all circumstances. Manual
    inspection of these data and/or the outcomes of image processing when making
    use of these data is advocated. If circumstances leading to incorrect import
    or encoding of this information in *MRtrix3* is reproducible, please report
    this to the *MRtrix3* developers so that the software can be updated to
    support such data.


Phase encoding information storage
----------------------------------

The phase encoding information for a particular image file can be stored in one
of two ways.

-  The most convenient of these is storage of (one or more) key-value field(s)
   encapsulated within the image header, just as can be used for
   the diffusion gradient scheme. This ensures that the information
   is retained through image processing, as each *MRtrix3* command passes the
   header entries of the input image through to the output image.

-  Alternatively, this information can be stored within a JSON file that
   accompanies the relevant image file(s). This information would then typically
   be imported / exported using the ``-json_import`` and ``-json_export`` options
   in ``mrconvert``.

Precisely *how* this phase encoding information is *encoded* however depends on
the nature of the phase encoding information for that image; specifically,
whether the phase encoding information is *identical* for all volumes within an
image file (or if it contains just one volume), or whether the phase encoding
information *varies* between volumes within the image series.


Fixed phase encoding
....................

In the case where both the phase encoding *direction* and the EPI *total readout
time* are equivalent for all volumes within an image, this information is encoded
within two fields: "``PhaseEncodingDirection``" and "``TotalReadoutTime``". These
fields are consistent with `BIDS
<http://bids.neuroimaging.io//>`_ (the Brain Imaging Data Structure).

``PhaseEncodingDrection`` can take one of six values: "``i``", "``i-``", "``j``",
"``j-``", "``k``", ``k-``. These correspond to the first, second and third axes of
the corresponding image, using the RAS (Right-Anterior-Superior) convention common
to both NIfTI and *MRtrix3*. For example: If the phase encoding is applied A>>P
(anterior-posterior), this is the *second* spatial axis, but the phase encoding is
also *reversed* along that axis ("RAS" indicates voxel positions in the second
axis *increasing* when moving toward the anterior of the brain, whereas "A>>P"
indicates the opposite); hence in this example ``PhaseEncodingDirection`` would
have the value "``j-``".

.. NOTE::

    The phase encoding direction is defined specifically with respect to *image
    axes*. It is therefore *not* affected by the image *strides*, which only affect
    how the data for these axes are arranged when they are stored as a
    one-dimensional list of values.

    The phase encoding direction also does *not* relate to "``x``", "``y``" and
    "``z``" axis directions in "real" / "scanner" space, as do other
    representations of orientation information in *MRtrix3*. This is because phase
    encoding specifically affects the appearance of the image along the image axis
    in which phase encoding was applied; for instance, if the image were to be
    rotated, the EPI field inhomogeneity distortions would still align with the
    relevant image axis after this rotation, whereas in real-space the effective
    direction of phase encoding would have changed.

``TotalReadoutTime`` provides the total time required for the EPI readout train.
Specifically, this is the time between the centre of the first echo, and the centre
of the last echo, in the train; this is consistent with BIDS, and is sometimes
referred to as the "FSL definition", since it is consistent with relevant
calculations performed within FSL tools. It should be defined in seconds.


Variable phase encoding
.......................

If the phase encoding direction and/or the total readout time varies between
different volumes within a single image series, then the two key-value fields
described above are not sufficient to fully encode this information. In this
situation, *MRtrix3* will instead use a key-value entry "``pe_scheme``" (similar to
the "``dw_scheme``" entry used for the diffusion gradient scheme).

This information is stored as a *table*, where each row contains the phase encoding
direction and the readout time for the corresponding volume; the number of rows in
this table must therefore be equal to the number of volumes in the image. In each
row, the first three numbers encode the phase encoding direction, and the fourth
number is the total readout time. The direction is specified as a unit direction in
the image coordinate system; for instance, a phase encoding direction of A>>P would
be encoded as ``[ 0 -1 0 ]``.


Manipulation of phase encoding data
-----------------------------------

The primary purpose of storing this phase encoding information is to automate the
correction of EPI susceptibility distortions. However this can only occur if the
information stored is not invalidated through the manipulation of the corresponding
image data. Therefore, any *MRtrix3* command that is capable of manipulating the
image data in such a way as to invalidate the phase encoding information will
*automatically* modify this phase encoding information appropriately. This includes
modifying the representation of this information between the fixed and variable
phase encoding cases.

Consider, for instance, a pair of b=0 images, where the first was acquired with
phase encoding direction ``A>>P``, and the second was acquired using phase encoding
direction ``P>>A``::

    $ mrinfo AP.mif
    ******************************
    Image:            AP.mif
    ******************************
      ...
      PhaseEncodingDirection: j-
      TotalReadoutTime:  0.0575
      ...

    $ mrinfo PA.mif
    ******************************
    Image:            PA.mif
    ******************************
      ...
      PhaseEncodingDirection: j
      TotalReadoutTime:  0.0575
      ...

Now watch what happens when we concatenate these two images together::

    $ mrcat AP.mif PA.mif AP_PA_pair.mif -axis 3
    mrcat: [100%] concatenating "AP.mif"
    mrcat: [100%] concatenating "PA.mif"
    # mrinfo AP_PA_pair.mif
    ******************************
    Image:            AP_PA_pair.mif
    ******************************
      ...
      pe_scheme:     0,-1,0,0.0575
                     0,1,0,0.0575
      ...

When the two input images are concatenated, *MRtrix3* additionally concatenates the
phase encoding information of the input volumes; since it detects that these are not
consistent between volumes, it stores this information using the ``pe_scheme`` header
entry, rather than ``PhaseEncodingDirection`` and ``TotalReadoutTime``.

The ``mrconvert`` command has a number of additional functionalities that can be used
to manipulate this information:

-  The ``-import_pe_table`` and ``-export_pe_table`` options can be used to
   import/export the phase encoding information from / to file as a table, i.e. in
   the format used for the ``pe_scheme`` header entry described above. Note that even
   if all volumes in the image have the same phase encoding direction and total
   readout time, these options will still import / export these data in table format.

-  The ``-import_pe_eddy`` and ``-export_pe_eddy`` options can be used to
   import/export the phase encoding information in the format required by FSL's
   ``eddy`` tool. The `FSL documentation page <https://fsl.fmrib.ox.ac.uk/fsl/fslwiki/eddy/UsersGuide#A--acqp>`_
   describes this format in more detail.

-  The ``-json_import`` and ``-json_export`` options can be used to import/export
   *all* header key-value entries from/to an external JSON file. This may be useful
   in particular for operating within the BIDS specification. There is a caveat here:
   If you use the ``-json_export`` option on an image with *fixed* phase encoding,
   the ``PhaseEncodingDirection`` and ``TotalReadoutTime`` fields will be written as
   expected by BIDS; however if the image contains *variable* phase eocnding, then
   the ``pe_scheme`` header entry will be written to the JSON file, and this will not
   be appropriately interpreted by other BIDS tools.

-  The ``-set_property`` option may be useful to *override* these header entries if
   they are deemed incorrect by some other source of information.
