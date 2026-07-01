.. _tractography_data:

Tractography data
=================

This page describes how *MRtrix3* represents streamline tractography data and
their associated per-streamline / per-vertex quantitative data, the set of
tractogram file formats supported for reading and writing, and the ways in
which sidecar data may be encoded and specified on the command line.

For the storage of *image* data, see :ref:`image_handling`; for the other forms
of tabular and structural data with which *MRtrix3* interacts, see
:ref:`other_data`.


.. _streamline_data_model:

Streamline data
---------------

The fundamental unit of tractography data is the *streamline*: an ordered
sequence of vertices, each of which is a 3-vector encoding a position in space.
Consecutive vertices are interpreted as connected samples along a continuous
path, such that a streamline approximates an infinitesimal trajectory through
3D space. A *tractogram* is simply an ordered vector of such streamlines.

*MRtrix3* holds streamline vertex coordinates in *real (scanner) space*, in
units of millimetres, consistent with the :ref:`image_coord_system` used for
image data. Every tractogram file format either stores these scanner-space
coordinates directly, or embeds within its own header the grid geometry (a
voxel-to-scanner affine transform, or a voxel size) required to reconstruct
them; no separate reference image is therefore required in order to read a
tractogram back into scanner space.

Each streamline may additionally carry a single scalar *weight*: a
non-negative multiplicative factor that modulates that streamline's
contribution wherever streamlines are aggregated (for example when computing
track density or connectome edge weights). Streamline weights are treated by
*MRtrix3* as a privileged, first-class attribute rather than as generic
sidecar data; they are produced by :ref:`tcksift2` and consumed by commands
such as :ref:`tckedit`, :ref:`tckmap`, :ref:`tckstats`, :ref:`tck2connectome`
and :ref:`connectome2tck`.


.. _tractogram_sidecar_data:

Tractogram sidecar data
-----------------------

Beyond the vertex coordinates and the privileged streamline weight, a
tractogram may be accompanied by arbitrary named quantitative data. Such data
are referred to as *sidecar* data, and come in the following forms according to
the granularity at which values are associated with the tractogram:

-  **Data per streamline (DPS)**: one value (or one row of values) for each
   streamline in the tractogram. Streamline weights are the most familiar
   example, but any per-streamline quantity (e.g. a mean scalar sampled along
   each streamline, a length, or a group label) may be stored this way.

-  **Data per vertex (DPV)**: one value (or one row of values) for each vertex
   of every streamline. This form is used to encode some quantity that varies
   *along* the length of each streamline, such as an image intensity sampled at
   each vertex.

A given field may hold either a single scalar per element (a column count of
one) or a fixed-width row of several values per element. In addition, the TRX
format (below) supports *data per group* (DPG): metadata attached to a named
subset of streamlines rather than to individual streamlines or vertices.


.. _supported_tractography_formats:

Supported tractogram file formats
---------------------------------

A tractogram argument on the *MRtrix3* command line may be provided in any of
the supported formats; the format is recognised from the filename extension (or,
for the TRX directory form, from the path resolving to a directory). There is no
need to explicitly convert between formats prior to processing, although the
:ref:`tckconvert` command is provided for that purpose where a specific format
is required for interoperability with external software.

The formats differ in the set of capabilities they can support. Rather than
probe each backend, *MRtrix3* has each format handler *broadcast* its
capabilities along a small number of orthogonal axes, so that a command can
determine up-front whether a requested operation is serviceable by a given
format, and raise a clean error otherwise rather than silently corrupting or
discarding data. The salient axes are:

-  **Read / write**: whether the format can be read, written, or both.

-  **Step size**: whether the format can represent streamlines whose vertices
   are arbitrarily spaced, or requires a *constant* step size along each
   streamline. A format of the latter kind encodes each streamline as a
   sequence of fixed-length steps and so cannot faithfully store non-uniformly
   sampled geometry.

-  **Non-finite vertices**: whether a genuine non-finite value (``NaN`` /
   infinity) can occur as a vertex coordinate. Several formats reserve ``NaN``
   and ``Inf`` as in-band delimiters (see the :ref:`.tck <mrtrix_tracks_format>`
   specification below), and the quantised / compressed formats cannot
   represent a non-finite coordinate at all; such formats forbid non-finite
   coordinates as data.

-  **Sidecar data**: whether the format can carry named DPS / DPV fields at all,
   and if so whether a *new* sidecar field can be added to an existing dataset
   in place, or whether augmenting a dataset requires the whole dataset to be
   rewritten.

-  **Embedded grid transform**: whether the format's header can record an
   arbitrary voxel-to-scanner affine transform, which is required if vertex
   positions are to be stored in the voxel space of a reference grid (as
   requested by :ref:`tckconvert`'s ``-output_voxel`` option) while remaining
   self-describing.

The following table summarises these capabilities across the supported formats:

+-----------------------+-------------+-------+-----------+-------------+------------------+---------------------+
| Format                | Extension   | R / W | Step size | Non-finite  | Sidecar          | Embedded grid       |
|                       |             |       |           | vertices    | (DPS / DPV)      | transform           |
+=======================+=============+=======+===========+=============+==================+=====================+
| MRtrix tracks         | ``.tck``    | R/W   | arbitrary | forbidden   | not supported    | no                  |
+-----------------------+-------------+-------+-----------+-------------+------------------+---------------------+
| QFib compressed       | ``.qfib``   | R/W   | constant  | forbidden   | not supported    | no                  |
+-----------------------+-------------+-------+-----------+-------------+------------------+---------------------+
| TrackVis              | ``.trk``    | R/W   | arbitrary | ``NaN``     | rewrite          | no                  |
+-----------------------+-------------+-------+-----------+-------------+------------------+---------------------+
| TRX directory         | ``.trx/``   | R/W   | arbitrary | ``NaN``     | append in place  | yes                 |
+-----------------------+-------------+-------+-----------+-------------+------------------+---------------------+
| TRX uncompressed      | ``.trx``    | R/W   | arbitrary | ``NaN``     | append in place  | yes                 |
| archive               |             |       |           |             |                  |                     |
+-----------------------+-------------+-------+-----------+-------------+------------------+---------------------+
| TRX compressed        | ``.trx``    | R/W   | arbitrary | ``NaN``     | rewrite          | yes                 |
| archive               |             |       |           |             |                  |                     |
+-----------------------+-------------+-------+-----------+-------------+------------------+---------------------+
| DSI Studio TinyTrack  | ``.tt``     | R/W   | arbitrary | forbidden   | not supported    | no                  |
+-----------------------+-------------+-------+-----------+-------------+------------------+---------------------+
| VTK PolyData          | ``.vtk``    | R/W   | arbitrary | ``NaN``     | rewrite          | no                  |
+-----------------------+-------------+-------+-----------+-------------+------------------+---------------------+
| VTK STREAMLINES       | ``.vtx``    | R/W   | arbitrary | ``NaN``     | not supported    | no                  |
+-----------------------+-------------+-------+-----------+-------------+------------------+---------------------+
| ZFIB                  | ``.zfib``   | R/W   | arbitrary | forbidden   | not supported    | no                  |
+-----------------------+-------------+-------+-----------+-------------+------------------+---------------------+

.. NOTE::
  A tractogram may additionally be passed between two *MRtrix3* commands
  directly through a :ref:`Unix pipe <unix_pipelines>`, in which case the data
  never touch the filesystem. The piped stream is a streaming, rewrite-only
  channel that can carry DPS / DPV sidecar data but forbids non-finite vertex
  coordinates.

The individual formats differ as follows:

-  The native **MRtrix tracks** format (``.tck``) stores explicit
   floating-point vertex coordinates in scanner space; it is lossless but
   carries only vertices (no named sidecar fields). Its detailed structure is
   documented :ref:`below <mrtrix_tracks_format>`.

-  The **QFib** (``.qfib``) and **ZFIB** (``.zfib``) formats apply *lossy*
   compression to streamline geometry; the reconstructed vertex positions are
   an approximation of the originals. ``.qfib`` additionally requires a
   *constant* step size within each streamline (the per-streamline step being
   inferred from the distance between its first two vertices), so it cannot
   represent an arbitrarily sampled tractogram.

-  The **DSI Studio TinyTrack** (``.tt``) format quantises vertex coordinates
   with respect to a voxel grid and is stored within a MATLAB Level-5
   (``.mat``) container.

-  The **TrackVis** (``.trk``) format is widely used for interoperability with
   external software and can carry named per-streamline (properties) and
   per-vertex (scalars) sidecar fields. Although its header reserves a field
   for a voxel-to-scanner transform, the *MRtrix3* writer does not populate it,
   so ``.trk`` does not advertise an embedded grid transform.

-  The **VTK PolyData** (``.vtk``) and **VTK STREAMLINES** (``.vtx``) formats
   store streamlines as polygonal-line geometry; ``.vtk`` can additionally
   carry named scalar arrays as sidecar data, whereas ``.vtx`` carries vertices
   only.

-  The **TRX** format is a modern container designed to carry streamline
   geometry together with arbitrary DPS, DPV and DPG sidecar data, and records
   a voxel-to-scanner affine (``VOXEL_TO_RASMM``) in its JSON header so that its
   contents remain self-describing. A TRX dataset may be materialised in three
   backing forms: as an unpacked **directory**, as an **uncompressed ZIP
   archive**, or as a **compressed (deflated) ZIP archive**. The directory and
   uncompressed-archive forms permit a new sidecar field to be appended to an
   existing dataset *in place* (without rewriting the streamline data), whereas
   the compressed-archive form requires the dataset to be rewritten. When a
   ``.trx`` path resolves to (or is designated as) a directory, the directory
   backing is used; otherwise a fresh ``.trx`` file is written using the backing
   selected by the ``TRXArchiveCompression`` :ref:`configuration option
   <config_file_options>`.


.. _tractography_sidecar_encoding:

Encoding and specifying sidecar data
------------------------------------

Sidecar data may be encoded in one of two ways:

-  **Embedded** within the tractogram dataset itself. Formats that support
   sidecar data (``.trk``, ``.vtk`` and the TRX backings; see the table above)
   store named DPS / DPV fields alongside the streamline geometry within the
   same dataset.

-  **External**, as a standalone file separate from the tractogram. The
   supported external formats depend on the granularity of the data:

   -  *Per-streamline* (DPS) data may be provided as a **numerical text file**
      (see :ref:`other_data`; the delimiter is inferred from the extension on
      write and any of space / comma / semicolon / tab is accepted on read,
      with ``#`` introducing comment lines) or as a **NumPy** ``.npy`` file.

   -  *Per-vertex* (DPV) data may be provided as a **Track Scalar File**
      (``.tsf``; see :ref:`below <mrtrix_scalar_track_format>`).

Wherever a command-line argument accepts tractogram sidecar data (the most
common example being streamline weights), the argument may be given in any of
the following forms:

-  ``<path>`` — a bare filesystem path, to read from or write to a standalone
   external sidecar file.

-  ``<path>::<field>`` — a qualified reference selecting a named field of
   sidecar data embedded within the tractogram dataset at ``<path>``.

-  ``::<field>`` — a reference to a named field of sidecar data embedded within
   the command's *own* input or output tractogram. This is the only form able
   to reference embedded sidecar data of a piped tractogram, which has no
   command-line filesystem path of its own.

The qualified forms are parsed on the *last* ``::`` occurrence, so that a
Windows drive-letter path (which contains a single colon) is never mistaken for
a qualified reference.


.. _mrtrix_tracks_format:

Tracks file format (``.tck``)
-----------------------------

The format for track files is similar to that for :ref:`mrtrix_image_formats`.
It consists of a text header in the same ``key: value`` format, ending with
a single 'END' statement (terminated by a newline character), and followed by
binary data.

The first line of the header should read ``mrtrix tracks`` to indicate
that the file contains tracks in MRtrix format. Further ``key: value``
pairs typically provide information about the parameters used to produce
the tracks, and for the most part are not required to read the data. The
only *required* keys are the following:

-  **file**
   A ``file: . offset`` entry is required to specify the byte offset
   from the beginning of the file to the start of the binary track data.
   At this stage, only the single-file format is supported - in other
   words the filename part must be specified as '.' (see above for
   details).

-  **datatype**
   Specifies the datatype (and byte order). Only real floating-point data
   types are permitted: either 16, 32 or 64 bits (32 is the default), and
   either little-endian (LE) or big-endian (BE) ordering (the native
   ordering of the device used to generate the file is used as default).
   The valid :ref:`data_types` are therefore:
   Float16BE, Float16LE, Float32BE, Float32LE, Float64BE, Float64LE.

While not strictly compulsory, track files generated by *MRtrix3* commands
will additionally always contain the following:

-  **timestamp**
   A floating-point value that can be effectively used as a unique
   identifier for the file produced. In *MRtrix3* commands this is based
   on the number of nanoseconds since the epoch of the system timer.

-  **count**
   The number of streamlines stored in the file. This is commonly used
   to produce accurate progress information for commands that read
   streamline data from file. Note that even if an *MRtrix3* command is
   terminated prematurely, the value stored in this entry *should*
   reflect the number of streamlines actually stored in the file; this
   can however be verified for any particular file using the *MRtrix3*
   command :ref:`tckinfo` with the ``-count`` option.

-  **total_count**
   For command :ref:`tckgen`, the value stored in this field reflects
   the total number of streamlines that were generated, before the
   application of criteria for streamline acceptance / rejection; for
   other commands that operate on pre-calculated streamlines data rather
   than generating them, this field will reflect the number of streamlines
   that were *input* to that command, rather than the number that were
   subsequently stored in the output file.

The binary track data themselves are stored as triplets of floating-point
values: one triplet of values per vertex along the track. Tracks are
separated using a triplet of ``NaN`` (Not A Number) values. Finally, a
triplet of ``Inf`` (infinity) values is used to indicate the end of the
file.



.. _mrtrix_scalar_track_format:

Track Scalar File format (``.tsf``)
-----------------------------------

The Track Scalar File (TSF) format is very similar to the
:ref:`mrtrix_tracks_format`, in that it includes a header of key-value
pairs, followed by a stream of binary data relating to streamlines, with
``NaN`` delimiting between streamlines and ``Inf`` indicating the end of
the file. However rather than storing information about the *locations*
of streamline vertices, this format instead encodes *some quantitative
value* at the location of each streamline vertex. It is therefore the
external file format used to provide *per-vertex* (DPV) tractogram sidecar
data.

It differs from the :ref:`mrtrix_tracks_format` in the following ways:

-  **Header**:

   -  The first line of the header should instead contain the string:
      ``mrtrix track scalars``.

   -  In addition to the ``file:`` and ``datatype:`` keys, a TSF file
      must also contain the ``timestamp`` key; the value stored here
      must be a *perfect match* to the value of the ``timestamp`` field
      stored in the header of the ``.tck`` file based on which the track
      scalar file is being generated.

-  **Data**:

   -  Rather than storing *triplets* of floating-point values, with a
      triplet of ``NaN`` values delimiting between streamlines and a
      triplet of ``Inf`` values indicating the end of the file, a
      ``.tsf`` files contains *one* floating-point value per streamline
      vertex, with *one* ``NaN`` value delimiting between streamlines
      and *one* ``Inf`` value indicating the end of the file.

When reading a ``.tsf`` file, validation of that file against the
streamline vertex data stored in a ``.tck`` file on which the track
scalar values are based is typically performed by comparing the
``timestamp`` and ``count`` fields in the headers of the two files.
Undefined behaviour in some instances occur can occur if an attempt is
made to read a particular ``.tsf`` alongside some ``.tck`` file to
which it does not correspond if these checks are not first performed.
If there is doubt regarding the validity of a ``.tsf`` / ``.tck`` file
pair, the *MRtrix3* command :ref:`tsfvalidate` can be used to perform
a more exhaustive cross-examination of the two files.
