#!/bin/bash

# Set default MacOS path
export PATH=/bin:/sbin:/usr/bin:/usr/sbin

# create and save the directory where all the components will be built
mkdir mrtrix3-installer
cd mrtrix3-installer
root=$(pwd)

# EIGEN
SECONDS=0
curl -O -L http://bitbucket.org/eigen/eigen/get/3.3.7.tar.bz2
tar xvf 3.3.7.tar.bz2
mkdir -p ${root}/mrtrix3/include/eigen3
cp -r eigen*/Eigen ${root}/mrtrix3/include/eigen3
cp -r eigen*/unsupported ${root}/mrtrix3/include/eigen3
EIGEN_SECONDS=$SECONDS

# TIFF
SECONDS=0
curl -O -L http://download.osgeo.org/libtiff/tiff-4.0.10.tar.gz
tar xvf tiff-4.0.10.tar.gz
cd tiff-4.0.10
./configure -prefix ${root}/mrtrix3 --enable-shared=NO
make
make install
cd ..
TIFF_SECONDS=$SECONDS

# FFTW
SECONDS=0
curl -O http://www.fftw.org/fftw-3.3.8.tar.gz
tar xvf fftw-3.3.8.tar.gz
cd fftw-3.3.8
./configure -prefix ${root}/mrtrix3 -disable-doc -disable-fortran
make
make install
cd ..
FFTW_SECONDS=$SECONDS

# QTBASE
SECONDS=0
curl -O http://ftp1.nluug.nl/languages/qt/archive/qt/5.12/5.12.3/submodules/qtbase-everywhere-src-5.12.3.tar.xz
tar xfv qtbase-everywhere-src-5.12.3.tar.xz
cd qtbase-everywhere-src-5.12.3
./configure -opensource -confirm-license -release  -no-dbus -no-openssl -no-harfbuzz -no-freetype  -no-cups -no-sqlite -no-framework -nomake examples -prefix ${root}/mrtrix3
make
make install
cd ..
QTBASE_SECONDS=$SECONDS

# QTSVG
SECONDS=0
curl -O http://ftp1.nluug.nl/languages/qt/archive/qt/5.12/5.12.3/submodules/qtsvg-everywhere-src-5.12.3.tar.xz
tar xfv qtsvg-everywhere-src-5.12.3.tar.xz
cd qtsvg-everywhere-src-5.12.3
${root}/mrtrix3/bin/qmake
make
make install
cd ..
QTSVG_SECONDS=$SECONDS

# MRTRIX
SECONDS=0
git clone https://github.com/MRtrix3/mrtrix3.git mrtrix3-src
cd mrtrix3-src
EIGEN_CFLAGS="-I${root}/mrtrix3/include/eigen3" \
FFTW_CFLAGS="-I${root}/mrtrix3/include" FFTW_LDFLAGS="-L${root}/mrtrix3/lib -lfftw3" \
TIFF_CFLAGS="-I${root}/mrtrix3/include" TIFF_LDFLAGS="-L${root}/mrtrix3/lib -ltiff" \
PATH=${root}/mrtrix3/bin:$PATH \
./configure
./build
cp -r bin/* ${root}/mrtrix3/bin/
cp -r lib/* ${root}/mrtrix3/lib/
cd ..
MRTRIX_SECONDS=$SECONDS

# Report build times
TOTAL_SECONDS=$((EIGEN_SECONDS + TIFF_SECONDS + FFTW_SECONDS + QTBASE_SECONDS + QTSVG_SECONDS + MRTRIX_SECONDS))
echo eigen : $EIGEN_SECONDS s
echo tiff  : $TIFF_SECONDS s
echo fftw  : $FFTW_SECONDS s
echo qtbase: $QTBASE_SECONDS s
echo qtsvg : $QTSVG_SECONDS s
echo mrtrix: $MRTRIX_SECONDS s
echo total : $TOTAL_SECONDS s
