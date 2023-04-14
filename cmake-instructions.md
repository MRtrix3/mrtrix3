# Instructions to build mrtrix with CMake

1. Install CMake. Current minimum version required is 3.19. If you have an older version, you can use `pip install cmake` to install the latest version (note that you can use `pip show cmake` to figure out where pip has installed the executable).

2. Clone the repo and run cmake:
    
        $ git clone -b cmake https://github.com/daljit46/mrtrix3-cmake --recursive
        $ cd mrtrix3
        $ mkdir cmake-build && cd cmake-build
        $ cmake .. && cmake --build . --target install


Test on Ubuntu 20.04, but should work on Windows too (not sure about MacOS).
