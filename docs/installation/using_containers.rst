.. _using_containers:

Running *MRtrix3* using containers
==================================

From MRtrix version ``3.0.3`` onwards, official Docker and Singularity
containers are provided for utilising *MRtrix3* commands, enabling use
of all *MRtrix3* commands (including those that possess dependencies on
other neuroimaging software packages) without necessitating any software
installation on the user system.

Third-party neuroimaging software
---------------------------------

There are several *MRtrix3* commands that make use of software tools
that are provided as part of third-party neuroimaging software packages.
While the *MRtrix3* containers for versions ``3.0.3`` -- ``3.0.x``
embedded these third-party dependencies within the *MRtrix3* container,
from version ``3.1.0`` onwards this is no longer the case.
These containers now contain *exclusively* *MRtrix3* commands only.
Any attempt to run within these containers an *MRtrix3* command
that invokes a third-party neuroimaging software tool will fail.
More information on this topic can be found on the
:ref:`third-party software page<third-party-software>`,
including information on the alternative container
":ref:`MRtrix3_with3p
<https://github.com/MRtrix3/mrtrix3-with3p>`__",
which includes both *MRtrix3* and the set of third-party neuroimaging
software tools that are utilised by at least one *MRtrix3* command.

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
[DockerHub](https://hub.docker.com/repository/docker/mrtrix3/mrtrix3).

Run GUI command
^^^^^^^^^^^^^^^

The following instructions have been shown to work on Linux::

    xhost +local:root
    docker run --rm -it --device /dev/dri/ -v /run:/run -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=$DISPLAY -e XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR -u $UID mrtrix3 mrview
    xhost -local:root  # Run this when finished.

It may however be possible that you will need to modify these commands
in order to operate without warning / error on your system.

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

Using Singularity
-----------------

Build container natively
^^^^^^^^^^^^^^^^^^^^^^^^

The following instruction can be run from the location in which the
*MRtrix3* source code has been cloned::

    singularity build MRtrix3.sif Singularity

Build container from DockerHub
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This command converts the Docker image as stored on DockerHub into a
Singularity container stored on the user's local system::

    singularity build MRtrix3.sif docker://mrtrix3/mrtrix3:<version>

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

The following basic usage has been shown to work on Linux::

    singularity run -B /run MRtrix3.sif mrview

<<<<<<< Updated upstream
=======
If you have NVidia graphics drivers, you may need to instead use the `--nv` option::

    singularity run --nv /run MRtrix3.sif mrview

>>>>>>> Stashed changes
If you wish to utilise a *clean environment* when executing ``mrview``,
you will likely find that it is necessary to explicitly set the ``DISPLAY``
and ``XDG_RUNTIME_DIR`` environment variables. This could be done in a
few different ways:

1.  Set environment variables that will be added to the clean
    environment of the container::

        export SINGULARITYENV_DISPLAY=$DISPLAY
        export SINGULARITYENV_XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR
        singularity run --cleanenv -B /run MRtrix3.sif mrview

1.  Explicitly set those envvars during invocation
    (requires a relatively up-to-date Singularity)::

        singularity run --cleanenv --env DISPLAY=$DISPLAY,XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR -B /run MRtrix3.sif mrview

1.  Create a text file that specifies the environment variables to be set,
    and provide the path to that file at the command-line
    (requires a relatively up-to-date Singularity)::

        echo $'DISPLAY=$DISPLAY\nXDG_RUNTIME_DIR=$XDG_RUNTIME_DIR' > ~/.mrview.conf
        singularity run --cleanenv --env-file ~/.singularity/mrview.conf -B /run MRtrix3.sif mrview

If you experience difficulties here with ``mrview``, you may have better
success if the Singularity container is built directly from the *MRtrix3*
source code using the definition file "``Singularity``" rather than
converting from a Docker container or using a custom definition file.

If *not* using a clean environment, and you see the specific error::

    Qt: Session management error: None of the authentication protocols specified are supported

This can be resolved by running::

    unset SESSION_MANAGER
