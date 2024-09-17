#!/usr/bin/env bash

# This is a script to package mrtrix3 as a tarball. It is intended to be
# run on a system with a package manager that supports apt (e.g. Ubuntu).
# It will automatically install the necessary dependencies to build but
# it assumes that the host system has an existing installation of Qt 6.
# NOTE: if you're using a custom installation of Qt, you may need to
# set the environment variable QMAKE to point to the qmake executable
# of your Qt installation (and possible set LD_LIBRARY_PATH to point to
# root_of_qt_installation/lib).
# The script takes the source directory as an argument and creates a tarball
# containing the built binaries and the necessary helper scripts.

# Usage: ./package-linux-tarball.sh /path/to/source/directory

running_dir=$(pwd)
source_dir=$(realpath $1)
tarball_dir=$running_dir/mrtrix_tarball_tmp

set -e

clean_up() {
  printf "Cleaning up.\n"
  rm -rf $tarball_dir
}

mkdir -p $tarball_dir

trap clean_up EXIT

if [ ! -d "$1" ]; then
  printf "Source directory does not exist: $1"
  exit 1
fi

printf "Installing dependencies to build the project.\n"

sudo apt install cmake \
    '^libxcb.*-dev' \
    libgl1-mesa-dev \
    ninja-build \
    python3 \
    libeigen3-dev \
    zlib1g-dev \
    libfftw3-dev \
    libpng-dev

if ! command -v clang++-17 &> /dev/null
then
    printf "Clang 17 not found, installing it.\n"
    wget https://apt.llvm.org/llvm.sh
    chmod +x llvm.sh
    sudo ./llvm.sh 17
fi

printf "Configuring and building the project.\n"

build_dir=$tarball_dir/build_tarball
install_dir=$tarball_dir/install_tarball

cd $tarball_dir

cmake -B $build_dir -S $source_dir \
    -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++-17 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$install_dir \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,--as-needed" \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    -DMRTRIX_BUILD_NON_CORE_STATIC=ON
    
cmake --build $build_dir
cmake --install $build_dir

printf "Running linuxdeploy.\n"

wget "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
wget "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
chmod +x linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
mkdir appdir && mkdir appdir/usr
cp -r $install_dir/* appdir/usr
./linuxdeploy-x86_64.AppImage --appdir appdir --plugin qt

printf "Creating the tarball.\n"

rm -rf appdir/usr/translations # Remove translations since we don't need them
cp $source_dir/install_mime_types.sh appdir/usr
cp $source_dir/set_path appdir/usr
tar -czf mrtrix.tar.gz -C appdir/usr .
cp mrtrix.tar.gz $running_dir
