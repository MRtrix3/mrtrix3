.. _mrtrix_config:

Configuration file
======

The behaviour of a number of aspects of *MRtrix3* can be controlled by
the user via the *MRtrix3 configuration file*. Note, that this file is distinct
from the build configuration file that is generated as part of the *MRtrix3*
installation, but rather is used to specify default settings for a number of
parameters, many of which relate to data visualisation when using ``mrview``.

For all available configurable options, please refer to the
`configuration file options <config_file_options>`_ page.

Location
^^^^^^^^

*MRtrix3* applications will attempt to read configuration information from a two
locations. The system-wide configuration file ``/etc/mrtrix.conf`` is read
first if present, followed by the user-specific configuration
``~/.mrtrix.conf``.  If both system and user-specific configuration files
exist, the parameters specified in the two configuration files will be
aggregated, with user-specified configuration options taking precedence in the
case of a conflict. In the case that a particular configuration parameter is
not defined, *MRtrix3* will resort to hard-coded defaults.


Format
^^^^^^

The configuration files are text files, with each line containing a key:
value pair. For example

::

    Analyse.LeftToRight: false
    NumberOfThreads: 2

.. NOTE::
    Key names are case-sensitive.

The value entry may be interpreted by *MRtrix3* applications as either:

-  ``Boolean``: allowed values here are true or false
-  ``Integer``: any integer value
-  ``Floating-point``: any floating-point value
-  ``Text``: any text string, without any further interpretation

The list of all configuration file options can be found
`here <config_file_options>`_.

