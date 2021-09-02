.. _using_containers:

Running *MRtrix3* using containers
==================================

From MRtrix version ``3.0.3`` onwards, official Docker and Singularity
containers are provided for utilising *MRtrix3* commands, enabling use
of all *MRtrix3* commands (including those that possess dependencies on
other neuroimaging software packages) without necessitating any software
installation on the user system.

Using Docker
------------

Run terminal command
^^^^^^^^^^^^^^^^^^^^

Non-GUI commands are typically executed as follows::

    docker run --rm -it mrtrix3 <command>

(replacing "``<command>``" with the name of the command to be executed,
along with any arguments / options to be provided to it)

If an *MRtrix3* image has not been built on the local system, the
most recent *MRtrix3* Docker image will be automatically downloaded from
[DockerHub](https://hub.docker.com/r/mrtrix3/mrtrix3).

Run GUI command
^^^^^^^^^^^^^^^

The following instructions have been shown to work on Linux::

    xhost +local:root
    docker run --rm -it -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=$DISPLAY mrtrix3 mrview
    xhost -local:root  # Run this when finished.

Explicitly build image locally
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As an alternative to downloading the image from DockerHub as described
above, the following instruction can be run from a location in which the
*MRtrix3* source code has been cloned::

    docker build --tag mrtrix3 .
    
Set ``DOCKER_BUILDKIT=1`` to build parts of the Docker image in parallel,
which can speed up build time.
Use ``--build-arg MAKE_JOBS=4`` to build *MRtrix3* with 4 processors
(can substitute this with any number of processors > 0); if omitted,
*MRtrix3* will be built using a single thread only.
You will need to `register <https://fsl.fmrib.ox.ac.uk/fsldownloads_registration>`_
on the FSL website in order to obtain file ``fslinstaller.py``, which is a
prerequisite for building the container.

Using Singularity
-----------------

Build container natively
^^^^^^^^^^^^^^^^^^^^^^^^

The following instruction can be run from the location in which the
*MRtrix3* source code has been cloned::

    singularity build MRtrix3.sif Singularity

You will need to `register <https://fsl.fmrib.ox.ac.uk/fsldownloads_registration>`_
on the FSL website in order to obtain file ``fslinstaller.py``, which is a
prerequisite for building the container.

Build container from DockerHub
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This command converts the Docker image as stored on DockerHub into a
Singularity container stored on the user's local system::

    singularity build MRtrix3.sif docker://mrtrix/mrtrix3:<version>
    
(Replace "``<version>``" with the specific version tag of *MRtrix3*
desired)

Run terminal command
^^^^^^^^^^^^^^^^^^^^

Unlike Docker containers, where an explicit "``docker run``" command must be
utilised to execute a command within a container, a Singularity image itself
acts as an executable file that can be invoked directly::

    MRtrix3.sif <command>

(replacing "``<command>``" with the name of the command to be executed,
along with any arguments / options to be provided to it)

Run GUI command
^^^^^^^^^^^^^^^

The following usage has been shown to work on Linux::

    singularity exec -B /run MRtrix3.sif mrview




