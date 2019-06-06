THREADS=$(sysctl -n hw.ncpu)
export PATH=/bin:/sbin:/usr/bin:/usr/sbin
mkdir mrtrix3-installer
cd mrtrix3-installer
PREFIX=$(pwd)/mrtrix3

# Grab latest versions from git
EIGEN_VERSION=$(git ls-remote --tags https://github.com/eigenteam/eigen-git-mirror | awk '{print $2}' | grep -v '\^{}$' | grep -v '-' | sort -V | tail -1)
EIGEN_VERSION=${EIGEN_VERSION#*/*/}

TIFF_VERSION=$(git ls-remote --tags  https://gitlab.com/libtiff/libtiff.git/ | awk '{print $2}' | grep -v '\^{}$' | grep v.* | sort -V | tail -1)
TIFF_VERSION=${TIFF_VERSION#*/*/v}

FFTW_VERSION=$(git ls-remote --tags https://github.com/FFTW/fftw3 | awk '{print $2}' | grep -v '\^{}$' | grep 'fftw-3' | sort -V | tail -1)
FFTW_VERSION=${FFTW_VERSION#*/*/fftw-}

QT_VERSION=$(git ls-remote --tags https://github.com/qt/qt5 | awk '{print $2}' | grep -v '\^{}$' | grep -v '-' | sort -V | tail -1)
QT_VERSION=${QT_VERSION#*/*/v}

echo "Using eigen ${EIGEN_VERSION}"
echo "Using tiff ${TIFF_VERSION}"
echo "Using fftw ${FFTW_VERSION}"
echo "Using qt ${QT_VERSION}"

# EIGEN
SECONDS=0
curl -O -L http://bitbucket.org/eigen/eigen/get/${EIGEN_VERSION}.tar.bz2
tar xvf ${EIGEN_VERSION}.tar.bz2
mkdir -p ${PREFIX}/include/eigen3
cp -r eigen*/Eigen ${PREFIX}/include/eigen3
cp -r eigen*/unsupported ${PREFIX}/include/eigen3
EIGEN_SECONDS=${SECONDS}

# TIFF
SECONDS=0
curl -O -L http://download.osgeo.org/libtiff/tiff-${TIFF_VERSION}.tar.gz
tar xvf tiff-${TIFF_VERSION}.tar.gz
cd tiff-${TIFF_VERSION}
./configure -prefix ${PREFIX} --enable-shared=NO --without-x
make
make install
cd ..
TIFF_SECONDS=${SECONDS}

# FFTW
SECONDS=0
curl -O http://www.fftw.org/fftw-${FFTW_VERSION}.tar.gz
tar xvf fftw-${FFTW_VERSION}.tar.gz
cd fftw-${FFTW_VERSION}
./configure -prefix ${PREFIX} --disable-doc --disable-fortran --disable-debug --enable-threads --disable-dependency-tracking --enable-sse2 --enable-avx
make
make install
cd ..
FFTW_SECONDS=${SECONDS}

# QT5 BASE
SECONDS=0
curl -O http://ftp1.nluug.nl/languages/qt/archive/qt/${QT_VERSION%.*}/${QT_VERSION}/submodules/qtbase-everywhere-src-${QT_VERSION}.tar.xz
tar xfv qtbase-everywhere-src-${QT_VERSION}.tar.xz
cd qtbase-everywhere-src-${QT_VERSION}
./configure -opensource -confirm-license -release  -no-dbus -no-openssl -no-harfbuzz -no-freetype  -no-cups -no-sqlite -no-framework -nomake examples -prefix ${PREFIX}
make -j ${THREADS}
make install
cd ..
QTBASE_SECONDS=${SECONDS}

# QT5 SVG
SECONDS=0
curl -O http://ftp1.nluug.nl/languages/qt/archive/qt/${QT_VERSION%.*}/${QT_VERSION}/submodules/qtsvg-everywhere-src-${QT_VERSION}.tar.xz
tar xfv qtsvg-everywhere-src-${QT_VERSION}.tar.xz
cd qtsvg-everywhere-src-${QT_VERSION}
${PREFIX}/bin/qmake
make -j ${THREADS}
make install
cd ..
QTSVG_SECONDS=${SECONDS}

# MRTRIX
SECONDS=0
git clone https://github.com/MRtrix3/mrtrix3.git mrtrix3-src
cd mrtrix3-src
EIGEN_CFLAGS="-I${PREFIX}/include/eigen3" \
FFTW_CFLAGS="-I${PREFIX}/include" FFTW_LDFLAGS="-L${PREFIX}/lib -lfftw3" \
TIFF_CFLAGS="-I${PREFIX}/include" TIFF_LDFLAGS="-L${PREFIX}/lib -ltiff" \
PATH=${PREFIX}/bin:${PATH} \
./configure
NUMBER_OF_PROCESSORS=${THREADS} ./build
rm -rf ${PREFIX}/bin/*
cp -r bin/* ${PREFIX}/bin/
mv ${PREFIX}/plugins ${PREFIX}/bin/
rm -rf ${PREFIX}/bin/plugins/{bearer,generic,iconengines,printsupport,sqldrivers}
rm -rf ${PREFIX}/bin/plugins/platforms/libqminimal.dylib
rm -rf ${PREFIX}/bin/plugins/imageformats/libq{gif,ico,jpeg}.dylib
rm -rf ${PREFIX}/lib/{cmake,pkgconfig,*.la,*.a,*.prl,libQt5Test.*,libQt5Sql.*,libQt5Xml.*,libQt5Network.*,libQt5Concurrent.*}
cp -r lib/* ${PREFIX}/lib/
rm -rf ${PREFIX}/share/{doc,man}
cp -r share/* ${PREFIX}/share/
cp -r matlab ${PREFIX}/
cp set_path ${PREFIX}/
rm -rf ${PREFIX}/{doc,include,mkspecs}
cd ..
MRTRIX_SECONDS=${SECONDS}

tar cfz mrtrix3.tar.gz mrtrix3

TOTAL_SECONDS=$((EIGEN_SECONDS + TIFF_SECONDS + FFTW_SECONDS + QTBASE_SECONDS + QTSVG_SECONDS + MRTRIX_SECONDS))
echo "eigen ${EIGEN_VERSION}: ${EIGEN_SECONDS} s"
echo "tiff ${TIFF_VERSION}: ${TIFF_SECONDS} s"
echo "fftw ${FFTW_VERSION}: ${FFTW_SECONDS} s"
echo "qtbase ${QT_VERSION}: ${QTBASE_SECONDS} s"
echo "qtsvg ${QT_VERSION}: ${QTSVG_SECONDS} s"
echo "mrtrix: ${MRTRIX_SECONDS} s"
echo "total : ${TOTAL_SECONDS} s"
