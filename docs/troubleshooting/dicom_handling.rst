DICOM Handling
==============


Lossless JPEG compression
-------------------------

Some scanners export images in DICOM files using lossless JPEG compression. When
an *MRtrix3* command tries to read such a file, it will emit a warning

.. code::

    mrinfo dcmdir
    ...
    mrinfo: [WARNING] unknown DICOM transfer syntax: "1.2.840.10008.1.2.4.70" in file "data/foo/bar/IMG00235" - ignored
    ...

These files can be decompressed using the ``dcmdjpeg`` command from the 
DCMTK_ DICOM toolkit. On Linux, a directory of such files can be decompressed as follows

.. code::
    
    export PATH=/opt/dcmtk/bin:$PATH
    export DCMDICTPATH=/opt/dcmtk/share/dcmtk/dicom.dic

    for img in dcmdir/*
    do
        dcmdjpeg $img ${img}.tmp
        mv ${img}.tmp $img
    done

*MRtrix3* commands should now be able to read the directory successfully:

.. code::

    mrinfo dcmdir
    mrinfo: [done] scanning DICOM folder "data/driss/t1"
    mrinfo: [100%] reading DICOM series "AX FSPGR 3D ASSET  C+"
    ...


.. _DCMTK: http://dicom.offis.de/dcmtk.php.en
