#!/bin/bash -ex
export PATH=/bin:/sbin:/usr/bin:/usr/sbin
PLD=$(pwd)
BRANCH=dev
THREADS=$(sysctl -n hw.ncpu)
PREFIX=${PLD}/mrtrix3
if [ -f "${PREFIX}-build-deps.tar.gz" ]; then
  DEPCACHE=1
  echo "Reusing previously built dependencies"
  tar xfz ${PREFIX}-build-deps.tar.gz
else
  # Grab latest versions from git
  EIGEN_VERSION=$(git ls-remote --tags https://github.com/eigenteam/eigen-git-mirror.git | awk '{print $2}' | grep -v '\^{}$' | grep -v alpha | grep -v beta | grep -v '-' | sort -V | tail -1)
  EIGEN_VERSION=${EIGEN_VERSION#*/*/}
  echo "Using eigen ${EIGEN_VERSION}"

  TIFF_VERSION=$(git ls-remote --tags https://gitlab.com/libtiff/libtiff.git | awk '{print $2}' | grep -v '\^{}$' | grep -v alpha | grep -v beta | grep v.* | sort -V | tail -1)
  TIFF_VERSION=${TIFF_VERSION#*/*/v}
  echo "Using tiff ${TIFF_VERSION}"
  
  PNG_VERSION=$(git ls-remote --tags https://github.com/glennrp/libpng.git | awk '{print $2}' | grep -v '\^{}$' | grep -v alpha | grep -v beta | grep v.* | sort -V | tail -1)
  PNG_VERSION=${PNG_VERSION#*/*/v}
  echo "Using png ${PNG_VERSION}"
  
  FFTW_VERSION=$(git ls-remote --tags https://github.com/FFTW/fftw3.git | awk '{print $2}' | grep -v '\^{}$' | grep -v alpha | grep -v beta | grep 'fftw-3' | sort -V | tail -1)
  FFTW_VERSION=${FFTW_VERSION#*/*/fftw-}
  echo "Using fftw ${FFTW_VERSION}"
  
  QT_VERSION=$(git ls-remote --tags https://github.com/qt/qt5.git | awk '{print $2}' | grep -v '\^{}$' | grep -v alpha | grep -v beta | grep -v '-' | grep '5.9' | sort -V | tail -1)
  QT_VERSION=${QT_VERSION#*/*/v}
  echo "Using qt ${QT_VERSION}"

  # EIGEN
  SECONDS=0
  DNAME=eigen*
  FNAME=${EIGEN_VERSION}.tar.bz2
  curl -s -O -L http://bitbucket.org/eigen/eigen/get/${FNAME}
  tar xf ${FNAME}
  mkdir -p ${PREFIX}/include/eigen3
  cp -R ${DNAME}/Eigen ${PREFIX}/include
  cp -R ${DNAME}/unsupported ${PREFIX}/include
  rm -r ${FNAME} ${DNAME}
  EIGEN_SECONDS=${SECONDS}

  # TIFF
  SECONDS=0
  DNAME=tiff-${TIFF_VERSION}
  FNAME=${DNAME}.tar.gz
  curl -s -O -L http://download.osgeo.org/libtiff/${FNAME}
  tar xf ${FNAME}
  cd ${DNAME}
  ./configure -q -prefix ${PREFIX} --enable-shared=NO --without-x
  make install > /dev/null
  cd ..
  rm -r ${FNAME} ${DNAME}
  TIFF_SECONDS=${SECONDS}

  # PNG
  SECONDS=0
  DNAME=libpng-${PNG_VERSION}
  FNAME=${DNAME}.tar.xz
  curl -s -O -L https://ftp.osuosl.org/pub/blfs/conglomeration/libpng/${FNAME}
  tar xf ${FNAME}
  cd ${DNAME}
  ./configure -q -prefix ${PREFIX} --enable-shared=NO
  make install > /dev/null
  cd ..
  rm -r ${FNAME} ${DNAME} 
  PNG_SECONDS=${SECONDS}

  # FFTW
  SECONDS=0
  DNAME=fftw-${FFTW_VERSION}
  FNAME=${DNAME}.tar.gz
  curl -s -O http://www.fftw.org/${FNAME}
  tar xf ${FNAME}
  cd ${DNAME}
  ./configure -q -prefix ${PREFIX} --disable-doc --disable-fortran --disable-debug --enable-threads --disable-dependency-tracking --enable-sse2 --enable-avx
  make install > /dev/null
  cd ..
  rm -r ${FNAME} ${DNAME} 
  FFTW_SECONDS=${SECONDS}

  # QT5 BASE
  SECONDS=0
  DNAME=qtbase-opensource-src-${QT_VERSION}
  FNAME=${DNAME}.tar.xz
  curl -s -O http://ftp1.nluug.nl/languages/qt/archive/qt/${QT_VERSION%.*}/${QT_VERSION}/submodules/${FNAME}
  tar xf ${FNAME}
  cd ${DNAME}
  ./configure -opensource -confirm-license -release -no-dbus -no-openssl -no-harfbuzz -no-freetype  -no-cups -no-framework -nomake examples -prefix ${PREFIX}
  make -j ${THREADS} > /dev/null
  make install > /dev/null
  cd ..
  rm -r ${FNAME} ${DNAME} 
  QTBASE_SECONDS=${SECONDS}

  # QT5 SVG
  SECONDS=0
  DNAME=qtsvg-opensource-src-${QT_VERSION} 
  FNAME=${DNAME}.tar.xz
  curl -s -O http://ftp1.nluug.nl/languages/qt/archive/qt/${QT_VERSION%.*}/${QT_VERSION}/submodules/${FNAME}
  tar xf ${FNAME}
  cd ${DNAME}
  ${PREFIX}/bin/qmake
  make -j ${THREADS} > /dev/null
  make install > /dev/null
  cd ..
  rm -r ${FNAME} ${DNAME} 
  QTSVG_SECONDS=${SECONDS}

  tar cpfz ${PREFIX}-build-deps.tar.gz mrtrix3
fi

# MRTRIX3
SECONDS=0
git clone https://github.com/MRtrix3/mrtrix3.git mrtrix3-src -b ${BRANCH}
cd ${PREFIX}-src
CFLAGS="-I${PREFIX}/include" LINKFLAGS="-L${PREFIX}/lib" TIFF_LINKFLAGS="-llzma -ltiff" PATH=${PREFIX}/bin:${PATH} ./configure
NUMBER_OF_PROCESSORS=${THREADS} ./build
MRTRIX_VERSION=$(cat lib/mrtrix3/_version.py | awk '{print $3}' | tr -d '"')
cd ..
MRTRIX_SECONDS=${SECONDS}


mv ${PREFIX} ${PREFIX}_dep
mkdir -p ${PREFIX}/mrtrix3
cp -R ${PREFIX}-src/bin    ${PREFIX}/mrtrix3
cp -R ${PREFIX}-src/lib    ${PREFIX}/mrtrix3
cp -R ${PREFIX}-src/share  ${PREFIX}/mrtrix3
cp -R ${PREFIX}-src/matlab ${PREFIX}/mrtrix3
cp ${PREFIX}-src/set_path  ${PREFIX}/mrtrix3
rm -rf ${PREFIX}-src
cp ${PREFIX}_dep/lib/libQt5{Core,Gui,OpenGL,PrintSupport,Svg,Widgets}${QT_POSTFIX}.*.dylib ${PREFIX}/mrtrix3/lib
mkdir -p ${PREFIX}/mrtrix3/bin/plugins/{platforms,imageformats}
cp ${PREFIX}_dep/plugins/platforms/libqcocoa${QT_POSTFIX}.dylib  ${PREFIX}/mrtrix3/bin/plugins/platforms
cp ${PREFIX}_dep/plugins/imageformats/libqsvg${QT_POSTFIX}.dylib ${PREFIX}/mrtrix3/bin/plugins/imageformats
rm -rf ${PREFIX}_dep

cp -R ${PLD}/MRView.app ${PREFIX} 
mkdir -p ${PREFIX}/MRView.app/Contents/MacOS/
mv ${PREFIX}/mrtrix3/bin/mrview ${PREFIX}/MRView.app/Contents/MacOS/
cp ${PLD}/mrview ${PREFIX}/mrtrix3/bin

cp -R ${PLD}/SHView.app ${PREFIX}     
mkdir -p ${PREFIX}/SHView.app/Contents/MacOS/
mv ${PREFIX}/mrtrix3/bin/shview ${PREFIX}/SHView.app/Contents/MacOS/
cp ${PLD}/shview ${PREFIX}/mrtrix3/bin

cd ${PREFIX}/..
tar cfz mrtrix3.tar.gz mrtrix3
rm -rf ${PREFIX}

TOTAL_SECONDS=$((EIGEN_SECONDS + TIFF_SECONDS + PNG_SECONDS + FFTW_SECONDS + QTBASE_SECONDS + QTSVG_SECONDS + MRTRIX_SECONDS))
if [[ ! -n "$DEPCACHE" ]]; then
  echo "eigen ${EIGEN_VERSION}: ${EIGEN_SECONDS} s"
  echo "tiff ${TIFF_VERSION}: ${TIFF_SECONDS} s"
  echo "png ${PNG_VERSION}: ${PNG_SECONDS} s"
  echo "fftw ${FFTW_VERSION}: ${FFTW_SECONDS} s"
  echo "qtbase ${QT_VERSION}: ${QTBASE_SECONDS} s"
  echo "qtsvg ${QT_VERSION}: ${QTSVG_SECONDS} s"
fi
echo "mrtrix ${MRTRIX_VERSION}: ${MRTRIX_SECONDS} s"
echo "total : ${TOTAL_SECONDS} s"
