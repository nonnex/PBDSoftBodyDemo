# PBDSoftBodyPlugin Development Plan

## Project Overview
The PBDSoftBodyDemo project aims to implement an Unreal PlugIn that provides an advanced soft body simulation using Position-Based Dynamics (PBD) within Unreal Engine 5.5. 
This system will support real-time deformable objects with high vertex counts, suitable for applications like character muscle simulation, cloth dynamics, or destructible environments.

## Unreal Project: SoftBodyDemo (.uproject)
- **Purpose**: Demonstrates the PBDSoftBodyPlugin features and a Position-Based Dynamics (PBD) soft body simulation integrated into Unreal Engine. This .uproject uses the standard "Third Person Template as a C++ project" and the "Starter Content".
It embeds the PBDSoftBodyPlugin (which is the focus of this development) to test the plugin's capabilities in a sample environment. The .uproject serves merely as a "playground" for the plugin and can be expanded as needed.
- **Technologies**: Unreal Engine 5.5.4 (binary installation), C++, Git for version control.
- **Knowledge**: Skeletal mesh manipulation, vertex position retrieval, clustering algorithms, animation skinning, physics simulation with PBD constraints.
- **Repository**: [https://github.com/nonnex/PBDSoftBodyDemo.git](https://github.com/nonnex/PBDSoftBodyDemo.git)
- **Current Milestone**: Basic clustering and blending completed (Milestone 1).
- Base: Unreal Engine 5.5.4 Third Person Template (C++), using the shipped template starter Content like Mannequins Material etc.

## Unreal Plugin: PBDSoftBodyPlugin (.uplugin)
- **Purpose**: Provide a reusable PBD soft body simulation component for Unreal Engine projects.
- **Core Component**: `UPBDSoftBodyComponent` (inherits `USkeletalMeshComponent`).
- **Technologies**: Unreal Engine 5.5.4 C++ API (e.g., `FSkeletalMeshLODRenderData`, `FPositionVertexBuffer`, `ComputeSkinnedPositions`), Blueprint integration.
- **Knowledge**: Dynamic vertex position updates, cluster-based simulation, animation fallback, debug logging, planned physics with stretch/bend/collision constraints.
- **Features**: Vertex clustering, blending with animation, logging (`bEnableDebugLogging`, `bVerboseDebugLogging`), future physics and optimization.
- Component: UPBDSoftBodyComponent (inherits USkeletalMeshComponent), addable via Unreal Editor.
- Manager: Self Registering
- Features:
  - Skin Shader: Substrate material, ~0.5-1 ms per actor with LODs (15,000 → 7,500 → 3,000 verts).
  - Muscle Shader: MuscleDeform.usf with DQS (~2-3 ms), Implicit (~2-4 ms), XPBD (~2-4 ms), thread group 64, SM5.0.
  - Modes: DQS (0), Implicit (1), XPBD (2).
  - Animation Blending: SoftBodyBlendWeight (0.0-1.0) blends animated pose with simulation.
  - Optimizations: Constraint clustering, async buffers, velocity smoothing, dynamic stiffness.
  - Editor Enhancements:
    - Region-Based Control: Vertex colors define per-region blend weight, stiffness, and mode.
    - Editor Preview: Real-time simulation in Editor viewport.
- Status: Tasks 1 - 2.4 completed
- Target: Binary UE 5.5.4 compatibility, testable with SKM_QUINN Mannequin in a new project.

### Goals
- Achieve real-time performance (60 FPS) for 5-10 soft body actors, each with 45k+ vertices (225k–450k total).
- Implement a custom GPU-accelerated solution using compute shaders (`MuscleSim.usf`) and vertex shaders (`MuscleDeform.usf`) targeting Shader Model 5.0 (SM5.0).
- Integrate Dual Quaternion Skinning (DQS), Extended PBD (XPBD), Implicit deformations, and Substrate materials for a muscle simulation system.
- Maintain flexibility for future enhancements (e.g., animation, morphs, collision handling, LOD, etc).

### Scope
- First and foremost focus on Performace with as less as possible of visual Quality loss.
- Focus on GPU-based simulation to handle high vertex counts efficiently.
- Target mid-range SM5.0 hardware (e.g., GTX 970, ~1.5 TFLOPS). // TODO: Alter to RTX class Hardware, The scope and makes no sense for lesser GPU's
- Leverage Unreal Engine 5.5’s rendering and physics capabilities.


## Research / Science
- Study PBD fundamentals (e.g., Müller 2007 paper on Position-Based Dynamics).
- Review XPBD enhancements (Macklin et al., 2016) for improved stability and performance.
- Investigate Unreal Engine 5.5 GPU capabilities (compute shaders, Niagara, vertex shaders).

## Github Link
https://github.com/nonnex/PBDSoftBodyDemo

### Github Project Structure
```
PBDSoftBodyDemo
    │   PBDSoftBodyDemo.uproject
    │   PBDSoftBody_Plan_7.md
    │
    ├───Config
    │       DefaultEditor.ini
    │       DefaultEditorPerProjectUserSettings.ini
    │       DefaultEngine.ini
    │       DefaultGame.ini
    │       DefaultInput.ini
    │
    ├───Content
    │       README.md
    │
    ├───Plugins
    │   └───PBDSoftBodyPlugin
    │       │   PBDSoftBodyPlugin.uplugin
    │       │
    │       ├───Resources
    │       │       Icon128.png
    │       │
    │       └───Source
    │           └───PBDSoftBodyPlugin
    │               │   PBDSoftBodyPlugin.Build.cs
    │               │
    │               ├───Private
    │               │       PBDSoftBodyComponent.cpp
    │               │       PBDSoftBodyPlugin.cpp
    │               │
    │               └───Public
    │                       PBDSoftBodyComponent.h
    │                       PBDSoftBodyPlugin.h
    │
    └───Source
        │   PBDSoftBodyDemo.Target.cs
        │   PBDSoftBodyDemoEditor.Target.cs
        │
        └───PBDSoftBodyDemo
                PBDSoftBodyDemo.Build.cs
                PBDSoftBodyDemo.cpp
                PBDSoftBodyDemo.h
                PBDSoftBodyDemoCharacter.cpp
                PBDSoftBodyDemoCharacter.h
                PBDSoftBodyDemoGameMode.cpp
                PBDSoftBodyDemoGameMode.h
```

## Tasks

### Task 1: Setup Plugin Structure
- [x] Create `PBDSoftBodyPlugin` with basic component (`UPBDSoftBodyComponent`).
- [ ] 1.1: Add config file for default settings (e.g., `PBDSoftBodyConfig.ini`).

### Task 2: Core Simulation Logic
- [x] 2.1: Vertex position retrieval (`GetVertexPositions`).
- [x] 2.2: Cluster generation (`GenerateClusters`).
- [x] 2.3: Blending logic (`UpdateBlendedPositions`).
- [ ] 2.4: Apply positions to mesh.
  - [ ] 2.4.1: Update vertex buffer with `SimulatedPositions`.
  - [ ] 2.4.2: Test mesh deformation with `SKM_Quinn`.
- [ ] 2.5: Add dynamic cluster sizing based on mesh vertex count.
- [ ] 2.6: Implement vertex grouping by material.

### Task 3: Animation Integration
- [x] 3.1: Handle animated meshes with skinning (`ComputeSkinnedPositions`).
- [x] 3.2: Add fallback to reference pose for no-animation case.
- [ ] 3.3: Integrate with Animation Blueprint.
  - [ ] 3.3.1: Link `UPBDSoftBodyComponent` to Animation Blueprint events (e.g., override `SoftBodyBlendWeight` per cluster).
  - [ ] 3.3.2: Test with animated sequence (e.g., `ThirdPersonIdle`).

### Task 4: Physics Simulation
- [ ] 4.1: Implement PBD constraints.
  - [ ] 4.1.1: Add stretch constraints between vertices.
  - [ ] 4.1.2: Add bend constraints for cluster stability.
  - [ ] 4.1.3: Implement collision constraints with environment.
  - [ ] 4.1.4: Implement self-collision constraints.
  - [ ] 4.1.5: Implement volume preservation constraints for organic shapes.
- [ ] 4.2: Simulate velocities and positions.
  - [ ] 4.2.1: Update `Velocities` with gravity and constraints.
  - [ ] 4.2.2: Integrate positions using PBD solver.
- [ ] 4.3: Test physics simulation.
  - [ ] 4.3.1: Verify deformation with simple drop test (include collision and self-collision).

### Task 5: Optimization and Debugging
- [x] 5.1: Add logging (`bEnableDebugLogging`, `bVerboseDebugLogging`).
- [ ] 5.2: Optimize cluster performance.
  - [ ] 5.2.1: Profile clustering algorithm for large meshes.
  - [ ] 5.2.2: Reduce memory usage in `SimulatedPositions`.
- [ ] 5.3: Add debug visualization.
  - [ ] 5.3.1: Draw cluster centroids and vertex positions in editor (toggle with `bDebugMode`).

### Task 6: Testing and Documentation
- [ ] 6.1: Unit testing.
  - [ ] 6.1.1: Test vertex position retrieval with static mesh and zero vertices.
  - [ ] 6.1.2: Test clustering with varying `NumClusters`.
- [ ] 6.2: Integration testing.
  - [ ] 6.2.1: Verify simulation with animated and static meshes.
- [ ] 6.3: Documentation.
  - [ ] 6.3.1: Document `UPBDSoftBodyComponent` properties and methods.
  - [ ] 6.3.2: Create usage guide with Blueprint example (e.g., `SKM_Quinn` setup).
  - [ ] 6.3.3: Create video tutorial demonstrating plugin setup and simulation.

## Current State
- Task 2.3 completed with logging and animation fallback (tested with `SKM_Quinn`, 45993 vertices).
- Next: Task 2.4 (apply `SimulatedPositions` to mesh).

## Milestones
- **Milestone 1**: Basic clustering and blending (Done).
- **Milestone 2**: Full simulation with physics and collision (Pending).
- **Milestone 3**: Optimized and documented plugin (Pending).

## Tasks in Detail
- **Task 1: Setup Plugin Structure**
  - *Note*: Basic setup is solid; config file added as 1.1 for flexibility.
  - *Details*: Use `GConfig` to load defaults (e.g., `SoftBodyBlendWeight`, `NumClusters`) from `Config/PBDSoftBodyConfig.ini`.
- **Task 2: Core Simulation Logic**
  - *Note*: Clustering works; dynamic `NumClusters` added as 2.5 for scalability; vertex grouping by material added as 2.6 for realism.
  - *Details*: Dynamic clusters via `NumClusters = VertexCount / 1000` (clamped); material grouping uses `FSkeletalMeshLODRenderData::RenderSections`.
- **Task 3: Animation Integration**
  - *Note*: Fallback is great; Animation Blueprint weight override included in 3.3.1, now per cluster for precision.
  - *Details*: Add `ClusterBlendWeight` to `FSoftBodyCluster`; expose via `UFUNCTION(BlueprintCallable) void SetClusterBlendWeight(int32 ClusterIndex, float Weight)`.
- **Task 4: Physics Simulation**
  - *Note*: Stretch and bend are key; collision added as 4.1.3, self-collision as 4.1.4, volume preservation as 4.1.5 for enhanced realism.
  - *Details*: Stretch/bend use PBD distance constraints; collision via `FCollisionQueryParams`; self-collision with spatial hash; volume preservation maintains tetrahedron volumes.
- **Task 5: Optimization and Debugging**
  - *Note*: Logging is good; `bDebugMode` toggle in 5.3.1 now includes vertex positions for better debugging.
  - *Details*: Use `DrawDebugPoint` for centroids and vertices when `bDebugMode` is true; profile with UE’s `FScopedDurationTimer`.
- **Task 6: Testing and Documentation**
  - *Note*: Edge case (zero vertices) in 6.1.1; example docs in 6.3.2; video tutorial added as 6.3.3 for accessibility.
  - *Details*: Test zero vertices with empty `USkeletalMesh`; docs include Blueprint nodes; video covers setup, debug, and `SKM_Quinn` test.