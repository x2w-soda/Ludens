# Ludens

A data driven game development framework.

## Core

Ludens Core Library.

LDHeader Module:
- Ludens Version
- Platform Detection
- Codebase Types

LDSystem Module: OS, IO
- filesystem, file watcher
- hot reloading?
- IO and Sockets
- profiler integration with tracey?

LDJob
- thread based job system
- let user define Jobs

LDDSA
- data structure and algorithms

LDSerial Module: data serialization, format conversions
- serialization API for any class with public static Serialize and Deserialize methods
- defines two framework file types `.lda` and `.ldb` for text and binary serialization
- asset versioning!
- multithreaded serialization as Jobs
- parse and write common text formats .ini, .md, .json, .xml etc.

LDMedia Module: multi-media processing
- defines serializable formats for 3D models, audio, textures, etc.
- defines jobs for offline baking, etc.

LDRender Module
- Ludens Render Backend
- let user define a Renderer?
- generate Frame Graph for automatic resource state tracking
- particle system prefab
- skeletal mesh and animation
- RenderData

LDAudio Module:
- Miniaudio integration, use custom solution later
- AudioData

LDPhysics Module:
- Jolt Physics integration
- PhysicsData

LDUI Module:
- screen space UI design

LDApplication Module:
- Window system, Graphics API bootstrap
- Input polling 
- a Scene is the base unit of simulation; SceneData, SceneDriver...

LDSol Module:
- SolLang scripting API

LDScript Module:
- collects public APIs from other modules that are scriptable
- Node based language?