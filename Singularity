Bootstrap: debootstrap
OSVersion: focal
MirrorURL: http://us.archive.ubuntu.com/ubuntu/
Include: apt

%environment

# ACPCdetect
    ARTHOME=/opt/art
    export ARTHOME

# ANTs
    ANTSPATH=/opt/ants/bin
    export ANTSPATH

# FreeSurfer
    FREESURFER_HOME=/opt/freesurfer
    export FREESURFER_HOME

# FSL
    FSLDIR=/opt/fsl
    FSLOUTPUTTYPE=NIFTI_GZ
    FSLMULTIFILEQUIT=TRUE
    FSLTCLSH=/opt/fsl/bin/fsltclsh
    FSLWISH=/opt/fsl/bin/fslwish
    export FSLDIR FSLOUTPUTTYPE FSLMULTIFILEQUIT FSLTCLSH FSLWISH
    
# All
    LD_LIBRARY_PATH="/usr/local/cuda-9.1/lib64:/usr/local/cuda-9.1/bin:/.singularity.d/libs:/usr/lib:/opt/fsl/lib:$LD_LIBRARY_PATH"
    PATH="/opt/mrtrix3/bin:/opt/ants/bin:/opt/art/bin:/opt/fsl/bin:/usr/local/cuda-9.1/bin:$PATH"
    export LD_LIBRARY_PATH PATH

%post
# Non-interactive installation of packages
    export DEBIAN_FRONTEND=noninteractive

# Grab additional repositories
    sed -i 's/main/main restricted universe multiverse/g' /etc/apt/sources.list
    apt-get update && apt-get upgrade -y

# Runtime requirements
    apt-get update && apt-get install -y --no-install-recommends dc less libfftw3-bin liblapack3 libpng16-16 libqt5network5 libqt5widgets5 libtiff5 python3 python3-distutils zlib1g

# Build requirements
    apt-get update && apt-get install -y --no-install-recommends build-essential ca-certificates curl git libeigen3-dev libfftw3-dev libgl1-mesa-dev libpng-dev libqt5opengl5-dev libqt5svg5-dev libtiff5-dev qt5-qmake qtbase5-dev-tools wget zlib1g-dev

# Add CUDA 9.1 support for eddy (based on https://gist.github.com/DaneGardner/accd6fd330348543167719002a661bd5#install-cuda-toolkit)
    # Create tmp directory for CUDA installers
	mkdir /cuda_tmp && cd /cuda_tmp
	curl -LO https://developer.nvidia.com/compute/cuda/9.1/Prod/local_installers/cuda_9.1.85_387.26_linux
	curl -LO https://developer.nvidia.com/compute/cuda/9.1/Prod/patches/1/cuda_9.1.85.1_linux
	curl -LO https://developer.nvidia.com/compute/cuda/9.1/Prod/patches/2/cuda_9.1.85.2_linux
	curl -LO https://developer.nvidia.com/compute/cuda/9.1/Prod/patches/3/cuda_9.1.85.3_linux

	# Only install CUDA 9.1 toolkit
	sh cuda_9.1.85_387.26_linux --silent --override --toolkit

	# Install patches
	sh cuda_9.1.85.1_linux --silent --accept-eula
	sh cuda_9.1.85.2_linux --silent --accept-eula
	sh cuda_9.1.85.3_linux --silent --accept-eula

	# Clean up
	cd && rm -r /cuda_tmp
	
# Neuroimaging software / data dependencies
    # Download minified ART ACPCdetect (V2.0).
    mkdir -p /opt/art && curl -fsSL https://osf.io/73h5s/download | tar xz -C /opt/art --strip-components 1
    # Download minified ANTs (2.3.4).
    mkdir -p /opt/ants && curl -fsSL https://osf.io/3ad69/download | tar xz -C /opt/ants --strip-components 1
    # Download FreeSurfer lookup table file (v7.1.1).
    mkdir -p /opt/freesurfer && curl -fsSL -o /opt/freesurfer/FreeSurferColorLUT.txt https://raw.githubusercontent.com/freesurfer/freesurfer/v7.1.1/distribution/FreeSurferColorLUT.txt
    # Download minified FSL (6.0.4).
    mkdir -p /opt/fsl && curl -fsSL https://osf.io/dv258/download | tar xz -C /opt/fsl --strip-components 1

# Use Python3 for anything requesting Python, since Python2 is not installed
    ln -s /usr/bin/python3 /usr/bin/python

# MRtrix3 setup
    git clone -b master --depth 1 https://github.com/MRtrix3/mrtrix3.git /opt/mrtrix3
    cd /opt/mrtrix3 && ./configure && ./build -persistent -nopaginate && rm -rf testing/ tmp/ && cd ../../

# apt cleanup to recover as much space as possible
    apt-get remove -y build-essential ca-certificates curl git libeigen3-dev libfftw3-dev libgl1-mesa-dev libpng-dev libqt5opengl5-dev libqt5svg5-dev libtiff5-dev qt5-qmake qtbase5-dev-tools wget zlib1g-dev && apt-get autoremove -y && apt-get clean && rm -rf /var/lib/apt/lists/* #/tmp/* #/var/tmp/*

%runscript
    exec /usr/bin/bash -c "$@"
