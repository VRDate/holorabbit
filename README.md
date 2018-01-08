# holorabbit

This library is a thin C wrapper around RabbitMQ meant to provide simple functions that can be accessed via a C# DLLImport command in Unity.  This is pretty barebones but has been used extensively for my various projects that require RabbitMQ on the Hololens.  It goes without saying that this library requires the C RabbitMQ client library to compile.  I usually compile the entire thing statically into this library as it avoids DLL import issues in Unity where you are having to link two DLLs instead of this single wrapper.

Good luck and reach out for help.