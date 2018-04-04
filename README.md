# holorabbit

This library is a thin C wrapper around [RabbitMQ](http://www.rabbitmq.com) meant to provide simple functions that can be accessed via a C# DLLImport command in Unity.  This is pretty barebones but has been used extensively for my various projects that require RabbitMQ on the [Hololens](https://www.microsoft.com/en-us/hololens).  It goes without saying that this library requires the C RabbitMQ client library to compile.  I usually compile the entire thing statically into this library as it avoids DLL import issues in Unity where you are having to link two DLLs instead of this single wrapper.

## Basic Usage
0. Everything must be built in x86 32bit!  Hololens doesn't support 64 bit so don't even try.
1. Download and build RabbitMQ C Client for static linkage (https://github.com/alanxz/rabbitmq-c)
2. Link this Visual Studio project to your RabbitMQ C Client STATICALLY, don't use dynamic (DLL) unless you really love debugging weird DLL import issues.
3. Build this project and get an output DLL.
4. Import this DLL into the Unity project within a folder named Plugins
5. Import the C# scripts in the UnitySource folder into your Unity project.
6. The RabbitClient is the main interface that is used within your project.

## Good to knows
1. Everything needs to be 32 bits!
2. RabbitClient is multithreaded and has a built in message pump, so only instantiate one of this class.
3. Please contribute to this as I built this in desperation but got it to work so I figured I would share in the hopes of making it better.

Good luck and reach out for help.
