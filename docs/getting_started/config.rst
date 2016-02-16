.. _mrtrix_config:

Configuration
======

The behaviour of a number of aspects of *MRtrix3* can be controlled by
the user via the *MRtrix configuration file*. Note, that this file is distinct
from the build configuration file that is generated as part of the *MRtrix* installation,
but rather is used to specify default settings for a number of parameters that are
predominantly related to data visualisation --- specifically when using ``mrview``.
For all available configurable options, please refer to the :ref:`mrtrix_config_options`.

Location
^^^^^^^^

*MRtrix* applications will attempt to read configuration information from
a number of possible locations. In particular, one can specify both
a system-wide configuration file or a user-specific configuration
file, with the exact platform-specific location outlined below:

+---------------------------------------+------------------------+------------------------------------------------------+
| platform                              | system                 | user                                                 |
+=======================================+========================+======================================================+
| Unix (including GNU/Linux & MacOSX)   | ``/etc/mrtrix.conf``   | ``$HOME/.mrtrix.conf``                               |
+---------------------------------------+------------------------+------------------------------------------------------+
| Windows                               | ``C:\mrtrix.conf``     | ``$HOME\mrtrix.conf``\  (e.g. C:and Settings.conf)   |
+---------------------------------------+------------------------+------------------------------------------------------+

If both a system and user-specific configuration file is found, a given *MRtrix* command
will aggregate the specified parameters of the two configuration files, with user-specified configuration
options taking precedence in the case of a conflict. In the case that a particular configuration parameter 
is not defined, *MRtrix* will resort to hard-coded defaults.


Format
^^^^^^

The configuration files are text files, with each line containing a key:
value pair. For example

::

    Analyse.LeftToRight: false
    NumberOfThreads: 2

.. NOTE::
    Key names are case-sensitive.

The value entry may be interpreted by *MRtrix* applications as either:

-  ``Boolean``: allowed values here are true or false
-  ``Integer``: any integer value
-  ``Floating-point``: any floating-point value
-  ``Text``: any text string, without any further interpretation

.. _mrtrix_config_options:

List of configuration file options
^^^^^^^

.. include:: mrtrix_config_options.rst
