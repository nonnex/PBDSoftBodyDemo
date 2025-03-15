# PBDSoftBodyPlugin Development Plan

## Project Overview
The PBDSoftBodyDemo project aims to implement an Unreal PlugIn that provides an advanced soft body simulation using Position-Based Dynamics (PBD) within Unreal Engine 5.5. 
This system will support real-time deformable objects with high vertex counts, suitable for applications like character muscle simulation, cloth dynamics, or destructible environments.

## Unreal Project: SoftBodyDemo (.uproject)
- **Purpose**: Demonstrates the PBDSoftBodyPlugin features and a Position-Based Dynamics (PBD) soft body simulation integrated into Unreal Engine. This .uproject uses the standard "Third Person Template as a C++ project" and the "Starter Content". It embeds the PBDSoftBodyPlugin (which is the focus of this development) to test the plugin's capabilities in a sample environment. The .uproject serves merely as a "playground" for the plugin and can be expanded as needed.
- **Technologies**: Unreal Engine 5.5.4 (binary installation), C++, Git for version control.
- **Knowledge**: Skeletal mesh manipulation, vertex position retrieval, clustering algorithms, animation skinning, physics simulation with PBD constraints.
- **Repository**: [https://github.com/nonnex/PBDSoftBodyDemo](https://github.com/nonnex/PBDSoftBodyDemo)
- **Current Milestone**: Basic clustering, blending, and vertex buffer updates completed (Milestone 1).
- **Base**: Unreal Engine 5.5.4 Third Person Template (C++), using the shipped template starter Content like Mannequins Material etc.

## Unreal Plugin: PBDSoftBodyPlugin (.uplugin)
- **Purpose**: Provide a reusable, GPU-accelerated, real-time PBD soft body simulation component for Unreal Engine projects, targeting 60 FPS for 225k–450k vertices.
- **Core Component**: `UPBDSoftBodyComponent` (inherits `USkeletalMeshComponent`).
- **Technologies**: Unreal Engine 5.5.4 C++ API (e.g., `FSkeletalMeshLODRenderData`, `FPositionVertexBuffer`, `ComputeSkinnedPositions`, `FRHIVertexBuffer`, compute shaders), Blueprint integration.
- **Knowledge**: Dynamic vertex buffer updates, cluster-based simulation, animation blending, shader-based skinning integration, debug logging, GPU-accelerated physics with stretch/bend/collision constraints.
- **Features**: Vertex clustering, blending with animation, GPU-driven simulation, physics with constraint-based realism, and performance optimizations.
- **Component**: `UPBDSoftBodyComponent` (inherits `USkeletalMeshComponent`), addable via Unreal Editor.
- **Manager**: Self Registering
- **Features**:
  - **Skin Shader**: Substrate material, ~0.5-1 ms per actor with LODs (15k verts), scalable to 45k+ verts with GPU acceleration.
  - **Muscle Shader**: MuscleDeform.usf with DQS (~1-2 ms), Implicit (~1-3 ms), XPBD (~1-3 ms) for 45k+ verts, thread group 64+, SM5.0.
  - **Modes**: DQS (0), Implicit (1), XPBD (2).
  - **Animation Blending**: `SoftBodyBlendWeight` (0.0-1.0) blends animated pose with simulation via GPU, per-cluster control planned.
  - **Optimizations**: Constraint clustering, async buffers, velocity smoothing, dynamic stiffness, dynamic vertex buffers.
  - **Editor Enhancements**:
    - **Region-Based Control**: Vertex colors define per-region blend weight, stiffness, and mode.
    - **Editor Preview**: Real-time simulation in Editor viewport at 60 FPS with optimized GPU pipeline.
- **Status**: Tasks 1 - 2.4 completed.
- **Target**: Binary UE 5.5.4 compatibility, testable with `SKM_Quinn`, scalable to 5-10 actors (225k–450k verts) at 60 FPS.

### Goals
- Achieve real-time performance (60 FPS) for 5-10 soft body actors, each with 45k+ vertices (225k–450k total).
- Implement a custom GPU-accelerated solution using compute shaders (`MuscleSim.usf`) and vertex shaders (`MuscleDeform.usf`) targeting Shader Model 5.0 (SM5.0).
- Integrate Dual Quaternion Skinning (DQS), Extended PBD (XPBD), Implicit deformations, and Substrate materials for a muscle simulation system.
- Maintain flexibility for future enhancements (e.g., animation, morphs, collision handling, LOD, etc).

### Scope
- First and foremost focus on Performance with as little visual Quality loss as possible.
- Focus on GPU-based simulation to handle high vertex counts efficiently.
- Target RTX-class hardware (e.g., RTX 3060, ~12 TFLOPS); scope is impractical for lesser GPUs like GTX 970.
- Leverage Unreal Engine 5.5’s rendering and physics capabilities.

## Research / Science
- Study PBD fundamentals (e.g., Müller 2007 paper on Position-Based Dynamics).
- Review XPBD enhancements (Macklin et al., 2016) for improved stability and performance.
- Investigate Unreal Engine 5.5 GPU capabilities (compute shaders, Niagara, vertex shaders, dynamic vertex buffers).

## Tasks

### Task 1: Setup Plugin Structure
- [x] Create `PBDSoftBodyPlugin` with basic component (`UPBDSoftBodyComponent`).
- [x] 1.1: Add config file for default settings (e.g., `PBDSoftBodyConfig.ini`).

### Task 2: Core Simulation Logic
- [x] 2.1: Vertex position retrieval (`GetVertexPositions`).
- [x] 2.2: Cluster generation (`GenerateClusters`).
- [x] 2.3: Blending logic (`UpdateBlendedPositions`).
- [x] 2.4: Apply positions to mesh.
  - [x] 2.4.1: Update vertex buffer with `SimulatedPositions`.
  - [x] 2.4.2: Test mesh deformation with `SKM_Quinn`.
- [ ] 2.5: Add dynamic cluster sizing based on mesh vertex count.
  - [ ] 2.5.1: Implement `NumClusters = VertexCount / 1000` with clamping (e.g., min 1, max 100).
  - [ ] 2.5.2: Test scalability with meshes of varying vertex counts (e.g., 10k, 45k, 100k).
  - [ ] 2.5.3: Profile clustering performance to ensure <1 ms overhead for 450k verts.
- [ ] 2.6: Implement vertex grouping by material.
  - [ ] 2.6.1: Use `FSkeletalMeshLODRenderData::RenderSections` to group vertices by material.
  - [ ] 2.6.2: Validate clustering respects material boundaries with `SKM_Quinn`.
  - [ ] 2.6.3: Ensure material-based grouping supports region-based control (vertex colors).

### Task 3: Animation Integration
- [x] 3.1: Handle animated meshes with skinning (`ComputeSkinnedPositions`).
- [x] 3.2: Add fallback to reference pose for no-animation case.
- [ ] 3.3: Integrate with Animation Blueprint.
  - [ ] 3.3.1: Link `UPBDSoftBodyComponent` to Animation Blueprint events (e.g., override `SoftBodyBlendWeight` per cluster).
    - [ ] 3.3.1.1: Add `ClusterBlendWeight` to `FSoftBodyCluster` and expose via `SetClusterBlendWeight`.
    - [ ] 3.3.1.2: Blend `SimulatedPositions` with skinned positions post-skinning in `UpdateBlendedPositions`.
    - [ ] 3.3.1.3: Optimize blending for GPU execution (<1 ms for 45k verts).
  - [ ] 3.3.2: Test with animated sequence (e.g., `ThirdPersonIdle`).
    - [ ] 3.3.2.1: Verify animation blending with `SKM_Quinn` using per-cluster weights.
    - [ ] 3.3.2.2: Profile blending realism and performance with 5 actors (225k verts).
  - [ ] 3.3.3: Adjust skinning pipeline for PBD integration.
    - [ ] 3.3.3.1: Research custom vertex buffer vs. shader-based blending for post-skinning PBD.
    - [ ] 3.3.3.2: Implement GPU-based blending solution (e.g., `MuscleDeform.usf`).
    - [ ] 3.3.3.3: Validate realism with DQS/Implicit modes on animated meshes.

### Task 4: Physics Simulation
- [ ] 4.1: Implement PBD constraints.
  - [ ] 4.1.1: Add stretch constraints between vertices.
    - [ ] 4.1.1.1: Implement GPU-based stretch solver in `MuscleSim.usf`.
    - [ ] 4.1.1.2: Test stretch realism with 45k verts (<1 ms).
  - [ ] 4.1.2: Add bend constraints for cluster stability.
    - [ ] 4.1.2.1: Implement GPU-based bend solver.
    - [ ] 4.1.2.2: Validate cluster stability with dynamic stiffness.
  - [ ] 4.1.3: Implement environment collision constraints using SDFs.
    - [ ] 4.1.3.1: Integrate Unreal’s precomputed SDFs for static meshes.
    - [ ] 4.1.3.2: Query SDFs in `MuscleSim.usf` for GPU-accelerated checks.
    - [ ] 4.1.3.3: Test collision accuracy with 5 actors (225k verts).
  - [ ] 4.1.4: Implement self-collision constraints.
    - [ ] 4.1.4.1: Add broad-phase spatial hashing for vertex pair detection.
    - [ ] 4.1.4.2: Refine with narrow-phase BVH for critical regions (e.g., skin folds).
    - [ ] 4.1.4.3: Implement Verlet sphere proxies as a fallback for coarse checks.
    - [ ] 4.1.4.4: Optimize self-collision for <2 ms with 450k verts.
  - [ ] 4.1.5: Implement volume preservation constraints for organic shapes.
    - [ ] 4.1.5.1: Add GPU-based volume solver.
    - [ ] 4.1.5.2: Verify organic deformation realism (e.g., muscle bulge).
- [ ] 4.2: Simulate velocities and positions.
  - [ ] 4.2.1: Update `Velocities` with gravity and constraints.
  - [ ] 4.2.2: Integrate positions using PBD solver.
    - [ ] 4.2.2.1: Port PBD solver to GPU via `MuscleSim.usf`.
    - [ ] 4.2.2.2: Test solver stability with XPBD mode (<2 ms for 45k verts).
  - [ ] 4.2.3: Transition to dynamic `FRHIVertexBuffer` for GPU updates.
    - [ ] 4.2.3.1: Create dynamic buffer with `RHICreateVertexBuffer` for `SimulatedPositions`.
    - [ ] 4.2.3.2: Update buffer via `RHICmdList.UpdateBuffer` instead of `LockBuffer`.
    - [ ] 4.2.3.3: Profile GPU buffer updates for <1 ms with 450k verts.
- [ ] 4.3: Test physics simulation.
  - [ ] 4.3.1: Verify deformation with simple drop test (environment + self-collision).
  - [ ] 4.3.2: Profile collision performance with 5 actors (225k vertices total).
  - [ ] 4.3.3: Validate 60 FPS with 10 actors (450k verts) on RTX 3060.

### Task 5: Optimization and Debugging
- [x] 5.1: Add logging (`bEnableDebugLogging`, `bVerboseDebugLogging`).
- [ ] 5.2: Optimize cluster performance.
  - [ ] 5.2.1: Profile clustering algorithm for large meshes.
  - [ ] 5.2.2: Reduce memory usage in `SimulatedPositions`.
  - [ ] 5.2.3: Implement async buffer updates for clustering (<0.5 ms for 450k verts).
- [ ] 5.3: Add debug visualization.
  - [ ] 5.3.1: Draw cluster centroids, vertex positions, and collision proxies in editor (toggle with `bDebugMode`).
  - [ ] 5.3.2: Visualize vertex buffer updates to confirm deformation.
  - [ ] 5.3.3: Add real-time FPS overlay for editor preview.

### Task 6: Testing and Documentation
- [ ] 6.1: Unit testing.
  - [ ] 6.1.1: Test vertex position retrieval with static mesh and zero vertices.
  - [ ] 6.1.2: Test clustering with varying `NumClusters`.
  - [ ] 6.1.3: Test GPU shader modes (DQS, Implicit, XPBD) for correctness.
- [ ] 6.2: Integration testing.
  - [ ] 6.2.1: Verify simulation with animated and static meshes.
  - [ ] 6.2.2: Test scalability with 5-10 actors (225k–450k verts) at 60 FPS.
- [ ] 6.3: Documentation.
  - [ ] 6.3.1: Document `UPBDSoftBodyComponent` properties and methods.
  - [ ] 6.3.2: Create usage guide with Blueprint example (e.g., `SKM_Quinn` setup).
  - [ ] 6.3.3: Create video tutorial demonstrating plugin setup and simulation.

## Current State
- Task 2.4 completed with vertex buffer updates and testing on `SKM_Quinn` (45993 vertices).
- Next: Task 2.5 (dynamic cluster sizing).

## Milestones
- **Milestone 1**: Basic clustering, blending, and vertex buffer updates (Done).
- **Milestone 2**: Full simulation with physics and hybrid collision (Pending).
- **Milestone 3**: Optimized and documented plugin with GPU acceleration (Pending).

## Tasks in Detail
- **Task 1: Setup Plugin Structure**
  - *Note*: Basic setup is solid; config file added as 1.1 for flexibility.
  - *Details*: Use `GConfig` to load defaults (e.g., `SoftBodyBlendWeight`, `NumClusters`) from `Config/PBDSoftBodyConfig.ini`.
- **Task 2: Core Simulation Logic**
  - *Note*: Clustering works; dynamic `NumClusters` added as 2.5 for scalability; vertex grouping by material added as 2.6 for realism.
  - *Details*: Dynamic clusters via `NumClusters = VertexCount / 1000` (clamped); material grouping uses `FSkeletalMeshLODRenderData::RenderSections`; 2.5.3 ensures performance scalability.
- **Task 3: Animation Integration**
  - *Note*: Fallback is great; Animation Blueprint weight override in 3.3.1 now per cluster; skinning pipeline adjustment added as 3.3.3 for compatibility with GPU blending.
  - *Details*: Add `ClusterBlendWeight` to `FSoftBodyCluster`; expose via `UFUNCTION(BlueprintCallable) void SetClusterBlendWeight(int32 ClusterIndex, float Weight)`; shader-based blending in 3.3.3.2 targets <1 ms.
- **Task 4: Physics Simulation**
  - *Note*: Stretch and bend are key; hybrid collision system added for realism; dynamic buffer in 4.2.3 and GPU solvers in 4.1/4.2 ensure performance.
  - *Details*: 
    - Stretch/bend use PBD distance constraints in `MuscleSim.usf`.
    - Environment collisions use SDFs via `FCollisionQueryParams` and compute shaders.
    - Self-collisions use spatial hashing (broad-phase), BVH (narrow-phase), and Verlet proxies (fallback); optimized in 4.1.4.4.
    - Volume preservation maintains tetrahedron volumes.
    - Dynamic `FRHIVertexBuffer` reduces CPU-GPU overhead (4.2.3).
- **Task 5: Optimization and Debugging**
  - *Note*: Logging is good; `bDebugMode` toggle in 5.3.1 includes collision proxies; 5.3.2 adds vertex buffer visualization; 5.3.3 aids editor performance tuning.
  - *Details*: Use `DrawDebugPoint` for centroids, vertices, and proxies when `bDebugMode` is true; profile with UE’s `FScopedDurationTimer`; async buffers in 5.2.3.
- **Task 6: Testing and Documentation**
  - *Note*: Edge case (zero vertices) in 6.1.1; example docs in 6.3.2; video tutorial added as 6.3.3; 6.2.2 validates target goals.
  - *Details*: Test zero vertices with empty `USkeletalMesh`; docs include Blueprint nodes; video covers setup, debug, and `SKM_Quinn` test.