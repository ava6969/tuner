# Tuner
This library provides hyperparameter tuning via configuration files.

## Prerequisites
### [CMake](https://cmake.org/)
CMake is used to build the system. A comprehensive installation guide can be found [here.](https://cmake.org/install/) Alternatively, paste the lines below into your terminal:
```
wget https://github.com/Kitware/CMake/releases/download/v3.23.0-rc2/cmake-3.23.0-rc2.tar.gz
tar -xf cmake-3.23.0-rc2.tar.gz
cd cmake-3.23.0-rc2
./bootstrap -- -DCMAKE_BUILD_TYPE:STRING=Release
make
make install
```
If any permission errors are encountered, try appending `sudo` in front of each command (e.g. `sudo make` instead of `make`).

### [vcpkg](https://vcpkg.io/en/index.html)
vcpkg makes managing external libraries easy. A comprehensive installation guide can be found [here.](https://github.com/microsoft/vcpkg) Alternatively, paste the lines below into your terminal:
```
cd ~
git clone https://github.com/microsoft/vcpkg
./vcpkg/bootstrap-vcpkg.sh
```
If any permission errors are encountered, try appending `sudo` in front of each command (e.g. `sudo make` instead of `make`).

## Setting up Tuner
Before getting started, it should be known that this library has been primarily tested and built in a Linux environment, so directly building on a Windows system is inadvisable. If you are using Windows, it's best to use [WSL (Version 2)](https://docs.microsoft.com/en-us/windows/wsl/install).

To begin, clone the repo and enter it:
```
git clone https://github.com/ava6969/tuner.git
cd tuner
```
Install dependencies:
```
~/vcpkg/vcpkg install "@vcpkg_deps.txt"
```
Build the library:
```
mkdir cmake-build-debug
cd cmake-build-debug
cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_MAKE_PROGRAM=make ..
cmake --build .
```
And you're done!