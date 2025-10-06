## Ludens Framework

A data-driven game development framework.

### Early Work In Progress

Early WIP, the editor is currently under development. The codebase is organized into individual modules, module dependency should be acyclic.

### Core Modules

The CMake target `LDCoreLibs` contains core modules that are used from editor to game runtime.

Module public headers are in the `Include/Ludens` directory, and the module implementations are in the `LDCore` directory.

- Header

The header module is header-only. This is for templated classes and inline math.

- Profiler

CPU-side frame profiling is done via the Tracy profiler. Use the CMake option `LD_COMPILE_WITH_PROFILER` to enable or disable profiling.

- System

Heap memory management, filesystems, and I/O.

- JobSystem

A thread-based job system is employed as our current solution for CPU-side concurrency. Currently there are no plans for a fiber-based job system.

- CommandLine

Command line argument parsing.

- DSA

Data structure and algorithms.

- DSP

Digital signal processing.

- Serial

Binary serializtaion and compression schemes.

- Log

Console Logger.

- Application

The backbone of a windowed application. Handles user input and window events in an event-polling fasion.

- Lua

Lua state using LuaJIT. Note that this is a low-level module not concerned with scripting. This module mainly wraps the Lua state for a object-oriented API.

- Media

Multimedia processing. Parsers for common markup formats such as XML and JSON.

- Camera

Perspective and orthographic camera representation, and camera controllers.

- Gizmo

Gizmo control logic. Decoupled from rendering, this is a retained-mode controller for gizmo behaviour.

- RenderBackend

Graphics API abstraction layer, briefly abstracts Vulkan as the main driver. OpenGL integration is planned for backwards compatibility with older GPUs.

- RenderGraph

A custom render graph implementation that resolves the dependency and synchronization between render components. A render component has well defined input-output dimensions and contains render passes or compute passes.

- RenderComponent

Implementation of GPU algorithms as individual render components. First-party graphics pipelines and presets are also provided by this module. Instead of a monolithic renderer, we will be maintaining a wide array of render components for maximum flexibility. Users of this module still need to manage GPU synchronization by inserting proper pipeline barriers.

- RenderServer

Top level graphics abstraction, provides rendering service using combinations of render components. Users of this module should not have to worry about GPU-side synchronizations.

- UI

User interface solution. This is a retained-mode GUI library used for both the editor and the final in-game UI.

- Asset

Defines the format of a game asset, as well as how assets are loaded, unloaded, and imported. The Asset module is the primary user of the Media module.

- DataRegistry

A registry that maintains a hierarchy of components. We would mostly be traversing arrays of components for better cache locality, only tasks such as transform invalidation will use tree traversal. Note that this is the first "high level module" that needs to include a lot of subsystems since it defines all types of components. This module could be considered as the data model of a scene.

- Scene

The basic unit of game simulation, uses the DataRegistry and AssetManager to simulate a game scene. 

- Project

Defines a Ludens project. This should fully specify the assets, scenes, and any other meta data in a game project.

### Ludens Builder

The CMake target `LDBuilder` is a command line executable that serves as a stand-alone utility. The builder is not required by the end user to build a game, but it could be useful for advanced users since it exposes a lot of the framework's functionalities through the command line.

The CMake target `LDBuilderLibs` contains all builder modules and links with `LDCoreLibs`.

Public headers are in the `Include/LudensBuilder` directory, and the module implementations are in the `LDBulider` directory.

### Ludens Editor

The CMake target `LDEditor` is a GUI executable that is used to manage and edit game projects.

The CMake target `LDEditorLibs` contains all editor modules and links with `LDCoreLibs` and `LDBuilderLibs`.

Public headers are in the `Include/LudensEditor` directory, and the module implementations are in the `LDEditor` directory.

### License

The full source code of the framework is distributed under the permissive MIT license.