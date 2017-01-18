Hanging on network file system when writing images
==================================================

When any *MRtrix3* command must read or write image data, there are two
primary mechanisms by which this is performed:

1. `Memory mapping <https://en.wikipedia.org/wiki/Memory-mapped_file>`_:
The operating system provides access to the contents of the file as
though it were simply a block of data in memory, without needing to
explicitly load all of the image data into RAM.

2. Preload / delayed write-back: When opening an existing image, the
entire image contents are loaded into a block of RAM. If an image is
modified, or a new image created, this occurs entirely within RAM, with
the image contents written to disk storage only at completion of the
command.

This design ensures that loading images for processing is as fast as
possible and does not incur unnecessary RAM requirements, and writing
files to disk is as efficient as possible as all data is written as a
single contiguous block.

Memory mapping will be used wherever possible. However one circumstance
where this should *not* be used is when *write access* is required for
the target file, and it is stored on a *network file system*: in this
case, the command typically slows to a crawl (e.g. progressbar stays at
0% indefinitely), as the memory-mapping implementation repeatedly
performs small data writes and attempts to keep the entire image data
synchronised.

*MRtrix3* will now *test* the type of file system that a target image is
stored on; and if it is a network-based system, it will *not* use
memory-mapping for images that may be written to. *However*, if you
experience the aforementioned slowdown in such a circumstance, it is
possible that the particular configuration you are using is not being
correctly detected or identified. If you are unfortunate enough to
encounter this issue, please report to the developers the hardware
configuration and file system type in use.
