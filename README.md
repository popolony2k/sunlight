# sunlight

<img src="https://github.com/Nikkochocho/sunlight/blob/main/resources/sunny.png" alt="sunlight logo" title="sunny" align="center">

`sunlight` is an open source library written in C++ to make 2D games. Overall, its present main features include a manager that can deal with map, collision, sprites and graphic primitives.

[raylib](https://www.raylib.com/) was used for graphic backend, but the project aims to be extended to SDL and many others.

## Table of Contents :pushpin:
* [Requirements](#requirements-memo)
    - [CMake 3.24](#cmake-324)
    - [LibXML2](#libxml2)
    - [zlib](#zlib)
* [HowTo](#howto-bulb)
    - [Compiling the library using the samples](#compiling-the-library-using-the-samples)
* [Samples](#samples-sunny)

## Requirements :memo:
In order to make `sunlight` run as desired, it is necessary to install the dependencies listed below beforehand:

### CMake 3.24 
You may install the package on its official [page](https://cmake.org/). Higher versions are also supported.

- For **Linux**
Ubuntu/Debian users can install the package by using the command `apt-get` on their Linux shell.

It is also recommended to install by using the standard package manager offered by your prefered distro.

- For **MacOS**
Mac users may obtain the package through [MacPorts](https://www.macports.org/) or [Homebrew](https://brew.sh/).

- **WINDOWS ONLY**
Use [vcpkg](https://github.com/microsoft/vcpkg) to integrate C/C++ libraries using Visual Studio.

(this could be done for Linux or even MacOSX - needs confirmation).

### LibXML2
By default, the project's CMakeLists will automatically search LibXML and its dependencies (LibIconv). For those who might want to use the library installed on your system, the following parameter should be used to call CMake:
```
cmake -DUSE_LOCAL_LIBXML2
```

### zlib
zlib can be installed by using your standard package manager.

The installation steps for which OS are listed below.

- **Ubuntu/Debian family**
```console
apt-get install zlib1g-dev
```

- **Windows** (vcpkg)
```console
.\vcpkg\vcpkg install zlib:x64-windows
```

- **MacOS**
Install zlib through [MacPorts](https://www.macports.org/) or [Homebrew](https://brew.sh/).

* `raylib` dependencies on **Linux** 
You may find the detailed instructions by clicking [here](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux) and following the steps regarding the operational system you're using.

## HowTo :bulb:

### Compiling the library using the samples
By default, `sunlight` doesn't build the projects' samples. 

In order to make it compile with samples, it is necessary to open the CMakeLists on the project's root to change the option from **OFF** to **ON**, as shown below:
```
option(BUILD_LIBRARY_SAMPLES "Build SunLight library samples" ON)
```

>[!NOTE]
>You may also use the following command line on your system's shell:
>```console
>cmake -DBUILD_LIBRARY_SAMPLES=ON
>```

## Samples :sunny: