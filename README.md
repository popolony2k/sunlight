# SunLight

<div align="center">
<img src="/resources/sunny.png" alt="sunlight logo" title="sunny">

`sunlight` is an open source library written in C++ to make 2D games. Overall, its present main features include a manager that can deal with map, collision, sprites and graphic primitives.

[raylib](https://www.raylib.com/) was used for graphic backend, but the project aims to be extended to SDL and many others.
</div>

## Table of Contents :pushpin:
* [Requirements](#requirements-memo)
    - [CMake 3.24](#cmake-324)
    - [vcpkg](#vcpkg-windows-only)
    - [LibXML2](#libxml2)
    - [zlib](#zlib)
    - [raylib on linux](#raylib-dependencies-on-linux)
* [HowTo](#howto-bulb)
    - [Using samples](#using-samples)
    - [Configuring VsCode](#configuring-vscode)
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

### vcpkg (Windows only)

Use [vcpkg](https://github.com/microsoft/vcpkg) to integrate C/C++ libraries using Visual Studio.

### LibXML2

By default, the project's CMakeLists will automatically search LibXML and its dependencies (LibIconv). For those who might want to use the library installed on your system, the following parameter should be used to call CMake:
```shell
cmake -DUSE_LOCAL_LIBXML2
```

### zlib

zlib can be installed by using your standard package manager.

The installation steps for which OS are listed below.

- **Ubuntu/Debian family**

```shell
apt-get install zlib1g-dev
```

- **Windows** (vcpkg)

```bat
.\vcpkg\vcpkg install zlib:x64-windows
```

- **MacOS**

    Install zlib through [MacPorts](https://www.macports.org/) or [Homebrew](https://brew.sh/).

### raylib dependencies on Linux

You may find the detailed instructions by clicking [here](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux) and following the steps regarding the operational system you're using.

## HowTo :bulb:

### Using samples

By default, `sunlight` doesn't build the projects' samples. 

In order to make it compile with samples, it is necessary to open the CMakeLists on the project's root to change the option from **OFF** to **ON**, as shown below:
```cmake
option(BUILD_LIBRARY_SAMPLES "Build SunLight library samples" ON)
```

>[!NOTE]
>You may also use the following command line on your system's shell:
>```shell
>cmake -DBUILD_LIBRARY_SAMPLES=ON
>```

### Configuring VsCode

If you intend to use VsCode as your IDE, follow the instructions shown [here](/doc/vscode/README.md).

## Samples :sunny:

In order to get proper info on the samples usage, you may find more on [here](/samples/tilemaprenderer/docs/README.md).