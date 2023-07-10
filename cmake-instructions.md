# Build MRtrix3 using CMake

## Requirements
- CMake (>= 3.19)
- A compiler that supports C++17
- Qt >=5.9 (base and OpenGL components)
- Eigen 3
- FFTW
- Ninja (Optional)
- ccache or sccache (Optional)

## Build instructions
1. Clone the repo: `git clone https://github.com/mrtrix3/mrtrix3 -b cmake_experimental_shared`
2. Create a build directory and configure cmake:

        $ mkdir build
        $ cmake -B build mrtrix3
    
    If all the required dependencies are install, `CMake` should correctly configure the project.
    It's highly recommended that you use `Ninja` and `ccache` (or `sccache`) to configure the project,
    to do this run `cmake -G Ninja -D CMAKE_CXX_COMPILER_LAUNCHER=ccache -B build mrtrix3` instead of
    second step above. This will provide you faster compilations speeds. You can install `Ninja` and 
    `ccache` using your system's package manager (e.g. `brew install ninja ccache` or `apt install ninja ccache`).

    If you wish, we provide some default CMake presets that automatically configure the project using predefined
    settings. You can view the list of available presets in [CMakePresets.json](https://github.com/MRtrix3/mrtrix3/blob/cmake_experimental_shared/CMakePresets.json). To configure the project using a given preset run
    
        $ cmake --preset name_of_configure_preset
    
    in the source directory.

3. Run the build:

        $ cmake --build build

    CMake will build all commands inside your build directory (the executables will be inside `cmd`).
    If you'd like to build just a single command, you can do so by specifying `--target name_of_command`.


## Configure options

You can enable/disable the following options (passing `-D MYOPTION=ON/OFF` to CMake at the configure stage).

- `USE_QT6` to enable building the project using Qt 6.

- `BUILD_GUI` to choose whether you'd like to build the gui commands (e.g. `mrview`).

- `WARNING_AS_ERRORS` for compilation to fail when a compiler warning is generated.

- `BUILD_TESTS` to build tests.


## Testing

This is very much WIP. Currently, basic support for some binary tests has been added. To run tests,
you need to build the project as described above and then, from the build directory, run `ctest --output-on-failure`.

## Known issues

- If you're on MacOS and have installed Qt via brew, you may need to link the correct version of Qt
you intend to use to build MRtrix3. So if for example, you want to use Qt 5 and you have both Qt 5 and Qt 6
installed via brew, you will need to run `brew unlink qt && brew link qt5`.
