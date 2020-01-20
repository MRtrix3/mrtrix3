FROM neurodebian:nd18.04-non-free

ARG DEBIAN_FRONTEND="noninteractive"
ARG CONFIGURE_FLAGS=""
ARG BUILD_FLAGS=""

WORKDIR /opt/mrtrix3
COPY [".", "."]

# Install ANTs and FSL.
RUN apt-get -qq update \
    && apt-get install -yq --no-install-recommends \
          "ants=2.2.0-1ubuntu1" \
          "fsl=5.0.9-5~nd18.04+1" \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install Mrtrix3
RUN temp_deps='g++ git libeigen3-dev' \
    && apt-get -qq update \
    && apt-get install -yq --no-install-recommends \
          $temp_deps \
          libfftw3-dev \
          libgl1-mesa-dev \
          libqt5opengl5-dev \
          libqt5svg5-dev \
          libtiff5-dev \
          python \
          python-numpy \
          qt5-default \
          zlib1g-dev \
    && ./configure $CONFIGURE_FLAGS \
    && ./build $BUILD_FLAGS \
    && apt-get remove -yq $temp_deps \
    && apt-get autoremove -yq --purge \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

ENV PATH="/opt/mrtrix3/bin:$PATH"
