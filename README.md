# Shunei
Shunei (SHU Network Easy Implementation) is a portable, cross platform networking library.

It uses the [SHU](https://github.com/omerfuyar/shu) system. By defining `SHU` you can tell the library where to find `shu.h` or include it yourself  before any shu... library to prevent any complication. See [SHU](https://github.com/omerfuyar/shu) repo for more information.

Goal is to make networking easier and united in C. See examples to learn how to use.

On windows, user must link with library `ws2_32` to work with sockets.