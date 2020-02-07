Key features
============

While *MRtrix3* is primarily intended to be used for the analysis of
diffusion MRI data, at its fundamental level it is designed as a
general-purpose library for the analysis of *any* type of MRI data. As such,
it provides a back-end to simplify a large number of operations, many of
which will be invisible to the end-user. Specifically, *MRtrix3* features:

-  a consistent :ref:`command-line interface <command-line-interface>`, with
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

.. TIP:: 

  Some proficiency with the Unix command-line is required to make the best use
  of this software. There are many resources online to help you get
  started if you are not already familiar with it. We also recommend our own
  `Introduction to the Unix command-line
  <https://command-line-tutorial.readthedocs.io/>`__, which was written with a
  particular focus on the types of use that are common when using *MRtrix3*.


