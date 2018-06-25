FROM ubuntu:18.04

ARG DEBIAN_FRONTEND="noninteractive"
ARG CONFIGURE_FLAGS=""
ARG BUILD_FLAGS=""

WORKDIR /opt/mrtrix3
COPY [".", "."]

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
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ENV PATH="/opt/mrtrix3/bin:$PATH"
