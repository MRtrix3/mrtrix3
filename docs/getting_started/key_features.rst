Key features
============

While *MRtrix3* is primarily intended to be used for the analysis of
diffusion MRI data, at its fundamental level it is designed as a
general-purpose library for the analysis of *any* type of MRI data. As such,
it provides a back-end to simplify a large number of operations, many of
which will be invisible to the end-user. Specifically, *MRtrix3* features:

-  a :ref:`consistent command-line interface <command_line>`, with
   inline documentation for each command

-  universal import/export capabilities when 
   :ref:`accessing image data <image_handling>` across all *MRtrix3* applications.

-  :ref:`multi_file_image_file_formats` to load multiple images as a 
   single multi-dimensional dataset

-  efficient use of :ref:`unix_pipelines` for complex workflows

-  high performance on modern multi-core systems, with multi-threading
   used extensively throughout *MRtrix3*;

-  available on all common modern operating systems (GNU/Linux,
   MacOSX, Windows);

-  a consistent :ref:`image_coord_system` with most
   operations performed in scanner/world coordinates where possible.

