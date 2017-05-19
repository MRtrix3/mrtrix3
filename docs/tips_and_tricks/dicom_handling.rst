.. _dicom_handling:

DICOM handling
==============

*MRtrix3* includes its own fast DICOM handling backend, allowing all *MRtrix3*
applications to seamlessly support DICOM images as input. While this works well
in most cases, it can fail in some circumstances. Issues specific to this
format are outlined below.


How MRtrix3 handles DICOM data
------------------------------

When interpreting the argument provided for an input image to any *MRtrix3*
command, the image handling backend will assume that the data are in DICOM
format if the argument corresponds to a *folder*, or ends with the ``.dcm``
suffix. The DICOM handling backend will then quickly scan through the files
(recursively through the entire folder if one was provided), and build a
table of contents. This consists of a *tree*, containing one or more
*patients*, each containing one or more *studies*, each of which consists of
one of more image *series*. When multiple choices are possible (e.g. multiple
series are available), the application will present a menu to select the data
of interest. For example:

.. code-block:: console

    $ mrinfo "Siemens Trio/"
    mrinfo: [done] scanning DICOM folder "Siemens Trio/"
    Select series ('q' to abort):
       0 -    9 MR images 10:20:52 localizer_sf (*fl2d1) [1]
       1 -   21 MR images 10:24:39 diff 20DW 2NEX ALL (*ep_b1000#17) [2]
       2 -   54 MR images 10:24:40 diff 20DW 2NEX ALL_ADC (*ep_b0_1000) [3]
       3 -   54 MR images 10:24:40 diff 20DW 2NEX ALL_TRACEW (*ep_b1000t) [4]
       4 -   54 MR images 10:24:40 diff 20DW 2NEX ALL_FA (*ep_b0_1000) [5]
       5 -   21 MR images 10:28:52 ep2d_diff_MDDW_AT_WIP (ep_b1000#2) [7]
       6 -   54 MR images 10:28:53 ep2d_diff_MDDW_AT_WIP_ADC (ep_b0_1000) [8]
       7 -   54 MR images 10:28:53 ep2d_diff_MDDW_AT_WIP_TRACEW (ep_b1000t) [9]
       8 -   54 MR images 10:28:54 ep2d_diff_MDDW_AT_WIP_FA (ep_b0_1000) [10]
    ? 

The user should then enter the integer corresponding to the study of interest,
for example (following from the above):

.. code-block:: console

    ...

    ? 5
    mrinfo: [100%] reading DICOM series "ep2d_diff_MDDW_AT_WIP"
    ************************************************
    Image:               "TOURNIER DONALD (1) [MR] ep2d_diff_MDDW_AT_WIP"
    ************************************************
      Dimensions:        84 x 84 x 54 x 21
      Voxel size:        2.5 x 2.5 x 2.5 x ?
      Data strides:      [ -1 -2 3 4 ]
      Format:            DICOM
      Data type:         unsigned 16 bit integer (little endian)
      Intensity scaling: offset = 0, multiplier = 1
      Transform:               0.9986   4.186e-08    -0.05229      -99.76
                            -0.002193      0.9991    -0.04188      -83.83
                              0.05224     0.04193      0.9978      -43.07
      EchoTime:          0.08
      PhaseEncodingDirection: j-
      TotalReadoutTime:  0.0252
      comments:          TOURNIER DONALD (1) [MR] ep2d_diff_MDDW_AT_WIP
                         study: Head Brain_basic
                         DOS: 11/05/2007 10:28:52
      dw_scheme:         0,0,0,0
      [21 entries]       -0.99949686000000004,-0.0050327100000000001,-0.0050327100000000001,1000
                         ...
                         -0.11020774999999999,0.27551937999999998,0.95513387000000005,1000
                         -0.031670320000000002,0.79175793999999999,0.57006572,1000

What happens at this stage is a deeper scan through those files that relate
specifically to the data selected, to gather all the information required to
read the data correctly. This two-stage process allows *MRtrix3* to scan
through large datasets rapidly to allow the user to quickly select just those
datasets of interest. 

Selecting multiple matching series as a single dataset
......................................................

It is also possible to load multiple series as a single dataset, when the image
dimensions and other paramaters match. This is done by using
:ref:`number_sequences` to specify the series of interest. For example:

.. code-block:: console

    $ mrinfo "Siemens Trio/"
    mrinfo: [done] scanning DICOM folder "Siemens Trio/"
    Select series ('q' to abort):
       0 -    9 MR images 10:20:52 localizer_sf (*fl2d1) [1]
       1 -   21 MR images 10:24:39 diff 20DW 2NEX ALL (*ep_b1000#17) [2]
       2 -   54 MR images 10:24:40 diff 20DW 2NEX ALL_ADC (*ep_b0_1000) [3]
       3 -   54 MR images 10:24:40 diff 20DW 2NEX ALL_TRACEW (*ep_b1000t) [4]
       4 -   54 MR images 10:24:40 diff 20DW 2NEX ALL_FA (*ep_b0_1000) [5]
       5 -   21 MR images 10:28:52 ep2d_diff_MDDW_AT_WIP (ep_b1000#2) [7]
       6 -   54 MR images 10:28:53 ep2d_diff_MDDW_AT_WIP_ADC (ep_b0_1000) [8]
       7 -   54 MR images 10:28:53 ep2d_diff_MDDW_AT_WIP_TRACEW (ep_b1000t) [9]
       8 -   54 MR images 10:28:54 ep2d_diff_MDDW_AT_WIP_FA (ep_b0_1000) [10]
       9 -   21 MR images 10:35:01 diff 20DW 2NEX AT TE110 (*ep_b1000#1) [12]
      10 -   54 MR images 10:35:01 diff 20DW 2NEX AT TE110_ADC (*ep_b0_1000) [13]
      11 -   54 MR images 10:35:02 diff 20DW 2NEX AT TE110_TRACEW (*ep_b1000t) [14]
      12 -   54 MR images 10:35:02 diff 20DW 2NEX AT TE110_FA (*ep_b0_1000) [15]
      13 -   21 MR images 10:39:39 diff 20DW 2NEX AT TE80 (*ep_b1000#4) [17]
      14 -   54 MR images 10:39:51 diff 20DW 2NEX AT TE80_ADC (*ep_b0_1000) [18]
      15 -   54 MR images 10:39:51 diff 20DW 2NEX AT TE80_TRACEW (*ep_b1000t) [19]
      16 -   54 MR images 10:39:51 diff 20DW 2NEX AT TE80_FA (*ep_b0_1000) [20]
    ? 4,12,16
    mrinfo: [100%] reading DICOM series "diff 20DW 2NEX ALL_FA"
    mrinfo: [100%] reading DICOM series "diff 20DW 2NEX AT TE110_FA"
    mrinfo: [100%] reading DICOM series "diff 20DW 2NEX AT TE80_FA"
    ************************************************
    Image:               "TOURNIER DONALD (1) [MR] diff 20DW 2NEX ALL FA"
    ************************************************
      Dimensions:        84 x 84 x 54 x 3
      Voxel size:        2.5 x 2.5 x 2.5 x ?
      Data strides:      [ -1 -2 3 4 ]
      Format:            DICOM
      Data type:         unsigned 16 bit integer (little endian)
      Intensity scaling: offset = 0, multiplier = 1
      Transform:               0.9986   4.186e-08    -0.05229      -99.76
                            -0.002193      0.9991    -0.04188      -83.83
                              0.05224     0.04193      0.9978      -43.07
      EchoTime:          0.08
      PhaseEncodingDirection: j-
      TotalReadoutTime:  0.0249
      comments:          TOURNIER DONALD (1) [MR] diff 20DW 2NEX ALL FA
                         study: Head Brain_basic
                         DOB: 09/03/1977
                         DOS: 11/05/2007 10:24:40
    
In the above example, the application accessed 3 FA maps produced with
different echo times as a single 4D dataset, consisting of DICOM series
``4,12,16``.

Loading DICOM data from scripts
...............................

It is good practice to write scripts to perform the full analysis from the raw
data, so that the analysis can be performed afresh if required, and so that the
exact steps taken at every stage of the analysis are recorded. However, access
to DICOM data requires user interaction to select the right series for each
subject. Thankfully, it is simple to record these selections and use them in
scripts once the correct choices are known. For example, assuming we have a
data folder containing lots of data, and we are interested in Donald's T1
scan:

.. code-block:: console

    $ mrinfo DICOM_folder/
    mrinfo: [done] scanning DICOM folder "DICOM_folder/"
    Select patient (q to abort):
       1 - WILLATS LISA (000188) 06/04/1981
       2 - TOURNIER DONALD (BRI) 09/03/1977
    ? 2
    patient: TOURNIER DONALD (BRI) 09/03/1977
    Select series ('q' to abort):
       0 -    9 MR images 15:31:22 localiser (*fl2d1) [1]
       1 -  160 MR images 15:37:34 t1_mpr_1mm iso qk (*tfl3d1_ns) [2]
       2 -   60 MR images 15:38:33 AX MPR T1 (*tfl3d1_ns) [3]
       3 -   60 MR images 15:38:56 COR MPR T1 (*tfl3d1_ns) [4]
       4 -   51 MR images 15:39:28 SAG MPR T1 (*tfl3d1_ns) [5]
       5 -    8 MR images 15:46:57 svs_se_30 PWM NWS (*tfl3d1_ns) [6]
       6 -    8 MR images 15:52:32 svs_se_30 PWM WS 32 ACQ (*tfl3d1_ns) [7]
       7 -  167 MR images 15:58:40 diff60_b3000_2.3_iPat2+ADC (*ep_b3000#93) [8]
       8 -   54 MR images 16:16:50 diff60_b3000_2.3_iPat2+ADC_ADC (*ep_b0_3000) [9]
       9 -  108 MR images 16:16:50 diff60_b3000_2.3_iPat2+ADC_TRACEW (*ep_b3000t) [10]
      10 -   54 MR images 16:16:51 diff60_b3000_2.3_iPat2+ADC_FA (*ep_b0_3000) [11]
    ? 1
    mrinfo: [100%] reading DICOM series "t1_mpr_1mm iso qk"
    ************************************************
    Image:               "TOURNIER DONALD (BRI) [MR] t1_mpr_1mm iso qk"
    ************************************************
      Dimensions:        160 x 256 x 256
      Voxel size:        1 x 1 x 1
      Data strides:      [ 3 -1 -2 ]
      Format:            DICOM
      Data type:         unsigned 16 bit integer (little endian)
      Intensity scaling: offset = 0, multiplier = 1
      Transform:               0.9987     0.05056    0.003483      -85.68
                             -0.05056      0.9987  -0.0001763      -106.9
                            -0.003487   9.906e-09           1      -130.2
      EchoTime:          0.00255
      PhaseEncodingDirection: j-
      TotalReadoutTime:  0
      comments:          TOURNIER DONALD (BRI) [MR] t1_mpr_1mm iso qk
                         study: BRI_Temp_backup Donald
                         DOB: 09/03/1977
                         DOS: 03/10/2007 15:37:34

We can see that the relevant series is obtained using the choices ``2`` (to get
the second patient) and ``1`` (to get the second series for that patient). This
can be scripted using the ``echo`` command to *pipe* these numbers directly to the
relevant command, with no further user interaction required, for example:

.. code-block:: console
    
    $ echo "2 1" | mrconvert DICOM_folder/ T1_anat.nii
    mrconvert: [done] scanning DICOM folder "DICOM_folder/"
    Select patient (q to abort):
       1 - WILLATS LISA (000188) 06/04/1981
       2 - TOURNIER DONALD (BRI) 09/03/1977
    ? patient: TOURNIER DONALD (BRI) 09/03/1977
    Select series ('q' to abort):
       0 -    9 MR images 15:31:22 localiser (*fl2d1) [1]
       1 -  160 MR images 15:37:34 t1_mpr_1mm iso qk (*tfl3d1_ns) [2]
       2 -   60 MR images 15:38:33 AX MPR T1 (*tfl3d1_ns) [3]
       3 -   60 MR images 15:38:56 COR MPR T1 (*tfl3d1_ns) [4]
       4 -   51 MR images 15:39:28 SAG MPR T1 (*tfl3d1_ns) [5]
       5 -    8 MR images 15:46:57 svs_se_30 PWM NWS (*tfl3d1_ns) [6]
       6 -    8 MR images 15:52:32 svs_se_30 PWM WS 32 ACQ (*tfl3d1_ns) [7]
       7 -  167 MR images 15:58:40 diff60_b3000_2.3_iPat2+ADC (*ep_b3000#93) [8]
       8 -   54 MR images 16:16:50 diff60_b3000_2.3_iPat2+ADC_ADC (*ep_b0_3000) [9]
       9 -  108 MR images 16:16:50 diff60_b3000_2.3_iPat2+ADC_TRACEW (*ep_b3000t) [10]
      10 -   54 MR images 16:16:51 diff60_b3000_2.3_iPat2+ADC_FA (*ep_b0_3000) [11]
    mrconvert: [100%] reading DICOM series "t1_mpr_1mm iso qk"
    mrconvert: [100%] copying from "TOURNIER D...BRI) [MR] t1_mpr_1mm iso qk" to "T1_anat.nii"
    


When the DICOM import goes wrong
--------------------------------

Errors can occur in the DICOM import for several reasons. In some cases, we can
identify the problem in the *MRtrix3* code and provide a fix to handle these
data. In other cases, the data are simply not complete, not
standards-compliant, or stored using encodings that *MRtrix3* doesn't currently
handle.

The application crashes
..........................

If running a simple command such as:

.. code-block:: console
 
    $ mrconvert DICOM/ out.nii
    mrconvert: [SYSTEM FATAL CODE: SIGSEGV (11)] Segmentation fault: Invalid memory access

crashes without a relevant error message, then this is an overt bug that needs
fixing within the *MRtrix3* code. Even if the data are not DICOM-compliant, the
code should nonetheless be able to detect this and exit gracefully with a clear
indication of what the problem is. In these cases, please send the problematic
data sets to members of the *MRtrix3* team for inspection.

ERROR: missing image frames for DICOM image
...........................................

.. NOTE::

  This also applies to the "dimensions mismatch in DICOM series" error message. 

DICOM data are often stored with individual slices in separate files.
Unfortunately, there is no requirement in the DICOM standard that the files for
a given dataset should all reside within the same folder. This means that it's
not uncommon for files belonging to the same series to be spread over multiple
different sub-folders. This makes it all too easy for some of the images in a
DICOM series to go missing, due to users forgetting to copy over all of the
folders. Another way this can happen is when users copy the data from their
DICOM client (e.g. PACS system) before the DICOM sender has finished sending
the data (these transfers can take a long time...). Attempts to read the data
will fail with a message like this:

.. code-block:: console

    $ mrinfo DICOM/
    mrinfo: [done] scanning DICOM folder "DICOM/"
    mrinfo: [100%] reading DICOM series "DWI_60"
    mrinfo: [ERROR] missing image frames for DICOM image "Joe Bloggs [MR] DWI_60"
    mrinfo: [ERROR] error opening image "DICOM/"

In these cases, it is simply not possible to load the data, since there are
missing frames within it. The only solution here is to go back to the data
source, find the missing data, and try again.

ERROR: no diffusion encoding information found in image
.......................................................

This indicates that *MRtrix3* was unable to find any information regarding the
DW gradient directions (bvevcs/bvals) in the DICOM headers, leading to errors
like:

.. code-block:: console

    $ dwi2tensor DICOM/ dt.mif
    dwi2tensor: [done] scanning DICOM folder "DICOM/"
    dwi2tensor: [100%] reading DICOM series "DWI_60"
    dwi2tensor: [ERROR] no diffusion encoding information found in image "Joe Bloggs [MR] DWI_60"

This can happen for a number of reasons:

- the information is simply not present. This can happen with custom sequences
  not explicitly designed to provide this information, or lack of support for
  providing this information from some manufacturers. The only possible
  solution in this case is to obtain the DW information from a different
  source, and provide it to *MRtrix3* manually using the ``-grad`` or ``-fslgrad``
  options in those commands that support it. 

- the information is present, but in a format that *MRtrix3* doesn't yet
  support. This is a very rare occurence these days, but still possible. If
  you're convinced your data should contain this information, please get in
  touch with members of the *MRtrix3* team, so we can take a look and see if
  support can be added to the code. 

- the information *was* present, but has been stripped out by third-party
  software, in particular anonymisation packages. These typically work by
  stripping out all potentially patient-identifiable information.
  This often includes removing any *private* (vendor-specific) DICOM
  entries, since it's not possible for a computer program to guarantee that
  these entries contain no sensitive information. Unfortunately, these entries
  often do contain important information, notably the DW gradient information.
  In these cases, the only sensible solution is to request the raw
  non-anonymised data, convert these correctly, and anonymise the *converted*
  images.

ERROR: unsupported transfer syntax
..................................

The DICOM standard specifies a default *transfer syntax* to encode the
information and the imaging data themselves. However, it also allows specifies
a number of other storage formats to store the imaging data, notably compressed
formats such as different variants of `JPEG <https://jpeg.org/>`__ and `MPEG
<http://mpeg.chiariglione.org/>`__ (see the official `DICOM standard
<http://dicom.nema.org/dicom/2013/output/chtml/part05/chapter_10.html>`__ for
details). Importantly, these compressed formats are not mandatory: a compliant
DICOM implementation does not need to support these features. This makes it
entirely possible (and indeed, quite common) for a fully DICOM-compliant
implementation to produce data that cannot be understood by another fully
DICOM-compliant implementation - a less than ideal situation... 

*MRtrix3* does not currently support non-default transfer syntaxes - only those
that the standard defines as mandatory, and variants thereof. For reference,
these are:

- Implicit VR Little Endian (``1.2.840.10008.1.2``)

- Explicit VR Little Endian (``1.2.840.10008.1.2.1``)

- Explicit VR Big Endian (``1.2.840.10008.1.2.2``)

Any other transfer syntax will be flagged as unsupported, and *MRtrix3* will be
unable to read the data, providing an error message similar to this:

.. code-block:: console

    $ mrinfo DICOM
    mrinfo: [done] scanning DICOM folder "DICOM"
    mrinfo: [ERROR] unable to read DICOM images in "DICOM":
    mrinfo: [ERROR]   unsupported transfer syntax found in DICOM data
    mrinfo: [ERROR]   consider using third-party tools to convert your data to standard uncompressed encoding
    mrinfo: [ERROR]   e.g. dcmtk: http://dicom.offis.de/dcmtk.php.en
    mrinfo: [ERROR] error opening image "DICOM"

Thankfully, other tools exist that should be able to convert the data to a
format that *MRtrix3* (and other DICOM tools) will read. The `dcmtk
<http://dicom.offis.de/dcmtk.php.en>`__ DICOM toolkit in particular provides
the ``dcmdjpeg`` command to decompress data stored using JPEG transfer syntax.
On Linux, a directory of such files can be decompressed as follows (amend the
various ``PATH`` as required for your system):

.. code-block:: console
    
    export PATH=/opt/dcmtk/bin:$PATH
    export DCMDICTPATH=/opt/dcmtk/share/dcmtk/dicom.dic

    for img in dcmdir/*
    do
        dcmdjpeg $img ${img}.tmp
        mv ${img}.tmp $img
    done

*MRtrix3* commands should now be able to read the directory successfully:

.. code-block:: console

    mrinfo dcmdir
    mrinfo: [done] scanning DICOM folder "data/driss/t1"
    mrinfo: [100%] reading DICOM series "AX FSPGR 3D ASSET  C+"
    ...

