FROM neurodebian:nd18.04-non-free

ARG DEBIAN_FRONTEND="noninteractive"
ARG CONFIGURE_FLAGS=""
ARG BUILD_FLAGS=""

# Install ANTs and FSL.
RUN apt-get -qq update \
    && apt-get install -yq --no-install-recommends \
          "ants=2.2.0-1ubuntu1" \
          ca-certificates \
          curl \
          "fsl=5.0.9-5~nd18.04+1" \
          "fsl-first-data" \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    # Install eddy. The sources are private now, so neurodebian no longer provides it.
    && . /etc/fsl/fsl.sh \
    && cd $FSLDIR/bin \
    && curl -fsSLO https://fsl.fmrib.ox.ac.uk/fsldownloads/patches/eddy-patch-fsl-5.0.9/centos6/eddy_openmp \
    && ln -s eddy_openmp eddy \
    && chmod +x eddy_openmp

# Install Mrtrix3
WORKDIR /opt/mrtrix3
COPY [".", "."]
RUN temp_deps='g++ git libeigen3-dev' \
    && apt-get -qq update \
    && apt-get install -yq --no-install-recommends \
          $temp_deps \
          libfftw3-dev \
          libgl1-mesa-dev \
          libpng-dev \
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

WORKDIR /work

ENTRYPOINT ["bash", "-c", "source /etc/fsl/fsl.sh && bash $@"]
