ARG MAKE_JOBS="1"
ARG DEBIAN_FRONTEND="noninteractive"

FROM python:3.8-slim AS base
FROM buildpack-deps:buster AS base-builder

FROM base-builder AS mrtrix3-builder

# Git commitish from which to build MRtrix3.
ARG MRTRIX3_GIT_COMMITISH="master"
# Command-line arguments for `./configure`
ARG MRTRIX3_CONFIGURE_FLAGS=""
# Command-line arguments for `./build`
ARG MRTRIX3_BUILD_FLAGS="-persistent -nopaginate"

RUN apt-get -qq update \
    && apt-get install -yq --no-install-recommends \
        libeigen3-dev \
        libfftw3-dev \
        libgl1-mesa-dev \
        libpng-dev \
        libqt5opengl5-dev \
        libqt5svg5-dev \
        libtiff5-dev \
        qt5-default \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone, build, and install MRtrix3.
ARG MAKE_JOBS
WORKDIR /opt/mrtrix3
RUN git clone -b $MRTRIX3_GIT_COMMITISH --depth 1 https://github.com/MRtrix3/mrtrix3.git . \
    && ./configure $MRTRIX3_CONFIGURE_FLAGS \
    && NUMBER_OF_PROCESSORS=$MAKE_JOBS ./build $MRTRIX3_BUILD_FLAGS \
    && rm -rf tmp

# Download minified ART ACPCdetect (V2.0).
FROM base-builder as acpcdetect-installer
WORKDIR /opt/art
RUN curl -fsSL https://osf.io/73h5s/download \
    | tar xz --strip-components 1

# Download minified ANTs (2.3.4-2).
FROM base-builder as ants-installer
WORKDIR /opt/ants
RUN curl -fsSL https://osf.io/yswa4/download \
    | tar xz --strip-components 1

# Download FreeSurfer files.
FROM base-builder as freesurfer-installer
WORKDIR /opt/freesurfer
RUN curl -fsSLO https://raw.githubusercontent.com/freesurfer/freesurfer/v7.1.1/distribution/FreeSurferColorLUT.txt

# Download minified FSL (6.0.4-2)
FROM base-builder as fsl-installer
WORKDIR /opt/fsl
RUN curl -fsSL https://osf.io/dtep4/download \
    | tar xz --strip-components 1

# Build final image.
FROM base AS final

# Install runtime system dependencies.
RUN apt-get -qq update \
    && apt-get install -yq --no-install-recommends \
        binutils \
        dc \
        less \
        libfftw3-3 \
        libgl1-mesa-glx \
        libgomp1 \
        liblapack3 \
        libpng16-16 \
        libqt5core5a \
        libqt5gui5 \
        libqt5network5 \
        libqt5svg5 \
        libqt5widgets5 \
        libquadmath0 \
        libtiff5 \
        python3-distutils \
    && rm -rf /var/lib/apt/lists/*

COPY --from=acpcdetect-installer /opt/art /opt/art
COPY --from=ants-installer /opt/ants /opt/ants
COPY --from=freesurfer-installer /opt/freesurfer /opt/freesurfer
COPY --from=fsl-installer /opt/fsl /opt/fsl
COPY --from=mrtrix3-builder /opt/mrtrix3 /opt/mrtrix3

ENV ANTSPATH="/opt/ants/bin" \
    ARTHOME="/opt/art" \
    FREESURFER_HOME="/opt/freesurfer" \
    FSLDIR="/opt/fsl" \
    FSLOUTPUTTYPE="NIFTI_GZ" \
    FSLMULTIFILEQUIT="TRUE" \
    FSLTCLSH="/opt/fsl/bin/fsltclsh" \
    FSLWISH="/opt/fsl/bin/fslwish" \
    LD_LIBRARY_PATH="/opt/fsl/lib:$LD_LIBRARY_PATH" \
    PATH="/opt/mrtrix3/bin:/opt/ants/bin:/opt/art/bin:/opt/fsl/bin:$PATH"

# Fix "Singularity container cannot load libQt5Core.so.5" on CentOS 7
RUN strip --remove-section=.note.ABI-tag /usr/lib/x86_64-linux-gnu/libQt5Core.so.5 \
    && ldconfig \
    && apt-get purge -yq binutils

CMD ["/bin/bash"]
