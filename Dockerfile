ARG MAKE_JOBS="1"
ARG DEBIAN_FRONTEND="noninteractive"

FROM python:3.8-slim AS base
FROM buildpack-deps:bookworm AS base-builder

FROM base-builder AS mrtrix3-builder

# Git commitish from which to build MRtrix3.
ARG MRTRIX3_GIT_COMMITISH="dev"

RUN apt-get -qq update \
    && apt-get install -yq --no-install-recommends \
        cmake \
        git \
        libeigen3-dev \
        libfftw3-dev \
        libgl1-mesa-dev \
        libpng-dev \
        libqt5opengl5-dev \
        libqt5svg5-dev \
        libtiff5-dev \
        ninja-build \
        qt5-qmake \
        qtbase5-dev \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone, build, and install MRtrix3.
ARG MAKE_JOBS
WORKDIR /src/mrtrix3
RUN git clone -b $MRTRIX3_GIT_COMMITISH --depth 1 https://github.com/MRtrix3/mrtrix3.git . \
    && cmake -B/tmp/build -GNinja -DCMAKE_INSTALL_PREFIX=/opt/mrtrix3 \
        -DMRTRIX_USE_QT5=ON -DMRTRIX_BUILD_TESTS=OFF \
    && cmake --build /tmp/build \
    && cmake --install /tmp/build

# Build final image.
FROM base AS final

# Install runtime system dependencies.
RUN apt-get -qq update \
    && apt-get install -yq --no-install-recommends \
        binutils \
        dc \
        less \
        libfftw3-single3 \
        libfftw3-double3 \
        libgl1-mesa-glx \
        libpng16-16 \
        libqt5core5a \
        libqt5gui5 \
        libqt5network5 \
        libqt5svg5 \
        libqt5widgets5 \
        libquadmath0 \
        libtiff5-dev \
        procps \
    && rm -rf /var/lib/apt/lists/*

COPY --from=mrtrix3-builder /opt/mrtrix3 /opt/mrtrix3
COPY --from=mrtrix3-builder /src/mrtrix3 /src/mrtrix3

# - Set up softlinks corresponding to any potentially invoked 
#   external neuroimaging software commands
# - Create FSL FIRST data directory
#   so that "5ttgen fsl" script proceeds to point of attempting to run FSL executable
#   so that the appropriate error message is produced.
# - Create empty FreeSurferColorLUT.txt so that 5ttgen hsvs proceeds beyond point
#   of merely checking for existence of that file
WORKDIR /opt/thirdparty
COPY thirdparty/commands.txt /opt/thirdparty/commands.txt
COPY thirdparty/error.sh /opt/thirdparty/error.sh
RUN xargs --arg-file=commands.txt -n1 ln -s error.sh \
    && mkdir -p data/first/models_336_bin \
    && touch FreeSurferColorLUT.txt
WORKDIR /

ENV ANTSPATH=/opt/thirdparty \
    ARTHOME=/opt/thirdparty \
    FREESURFER_HOME=/opt/thirdparty \
    FSLDIR=/opt/thirdparty \
    FSLOUTPUTTYPE=NIFTI \
    PATH="/opt/mrtrix3/bin:/opt/thirdparty:$PATH"

# Fix "Singularity container cannot load libQt5Core.so.5" on CentOS 7
RUN strip --remove-section=.note.ABI-tag /usr/lib/x86_64-linux-gnu/libQt5Core.so.5 \
    && ldconfig \
    && apt-get purge -yq binutils

RUN ln -s /usr/bin/python3 /usr/bin/python

CMD ["/bin/bash"]
