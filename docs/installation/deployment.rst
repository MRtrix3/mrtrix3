.. _deployment:

Deploying MRtrix3 
=================

The installation instructions provided in the preceding pages produce a working
install for the current user only. There are many advantages to this: 

- no need for admin privileges, either for the initial install (beyond
  installation of dependencies), or any subsequent updates;
- users are in control of the precise version of MRtrix3 they are using for
  their specific projects - no system updates will interfere with their study.

However, system administrators and software distributors will want to install
*MRtrix3* in a system-wide location to make it accessible to all users; and/or to
deploy it to other systems without requiring a full rebuild. While *MRtrix3*
does not provide an explicit command to do this, it is a trivial process: 

- build the code
- copy the ``bin/``, ``lib/`` and ``share/`` folders together to the desired
  target location
- set the ``PATH`` to point to the ``bin/`` folder. 

This can be done any number of ways. The only requirement is that these 3
folders are co-located alongside each other, so that the executables can find
the *MRtrix3* shared library, and the scripts can find the requisite python
modules. 

Note also that this structure is broadly compatible with the `Linux Filesystem
Hierarchy Standard <https://en.wikipedia.org/wiki/Filesystem_Hierarchy_Standard>`__. 
It should be perfectly possible to merge the *MRtrix3* ``bin/``, ``lib/`` and
``share/`` folders with the system's existing equivalent locations in ``/usr/``
or ``/usr/local/`` if desired, in which case there would be no need to
explicitly set the ``PATH`` (assuming ``/usr/bin`` or ``/usr/local/bin/`` are
already in the ``PATH``). However, there is no *requirement* that it be
installed anywhere in particular, and we expect most sysadmins will prefer to
place them in a separate location to minimise any chance of conflict. 

Below we provide step-by-step instructions for  creating a single tar file that
can then be copied to other systems and extracted in the desired folder:

1. Obtain, configure and build the desired version of *MRtrix3*:

   .. code-block:: console

        $ git checkout http://github.com/MRtrix3/mrtrix3.git
        $ cd mrtrix3
        $ ./configure
        $ ./build

2. Collate the relevant folders and their contents into a single archive file:
   
   .. code-block:: console
 
        $ tar cvfz mrtrix3.tgz bin/ lib/ share/

3. Copy the resulting ``mrtrix3.tgz`` file over to the target system, into a
   suitable location., for example (as root):

   .. code-block:: console

        # mkdir /usr/local/mrtrix3
        # cp mrtrix3.tgz /usr/local/mrtrix3/

4. Extract the archive in this location (as root):

   .. code-block:: console

        # cd /usr/local/mrtrix3/
        # tar xvfz mrtrix3.tgz

   Assuming no errors were generated, you can safely remove the ``mrtrix3.tgz``
   file at this point.

5. Add the newly-extracted ``bin/`` folder to the ``PATH``, e.g.:

   .. code-block:: console

        $ export PATH=/usr/local/mrtrix3/bin:"$PATH"

   At which point *MRtrix3* command should be available to the corresponding
   user. 

   Note that the above command will only add *MRtrix3* to the ``PATH`` for the
   current shell session. You would need to add the equivalent line to your
   users' startup scripts, using whichever mechanism is appropriate for your
   system. 

