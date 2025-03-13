# PBDSoftBodyPlugin Development Plan

## Project: SoftBodyDemo
- Base: Unreal Engine 5.5.4 Third Person Template (C++), using the shipped template starter Content like Mannequins Material etc.
- Goal: Simulate skin/muscle soft body dynamics on a 15,000-vertex skeletal mesh for 5-10 actors at 60 FPS (<16.6 ms), targeting 6-11 ms total.

## Plugin: PBDSoftBodyPlugin
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

## Tasks

### Task 1: Setup Plugin Structure
- [x] Create `PBDSoftBodyPlugin` with basic component (`UPBDSoftBodyComponent`).

### Task 2: Core Simulation Logic
- [x] 2.1: Vertex position retrieval (`GetVertexPositions`).
- [x] 2.2: Cluster generation (`GenerateClusters`).
- [x] 2.3: Blending logic (`UpdateBlendedPositions`).
- [ ] 2.4: Apply positions to mesh.
  - [ ] 2.4.1: Update vertex buffer with `SimulatedPositions`.
  - [ ] 2.4.2: Test mesh deformation with `SKM_Quinn`.

### Task 3: Animation Integration
- [x] 3.1: Handle animated meshes with skinning (`ComputeSkinnedPositions`).
- [x] 3.2: Add fallback to reference pose for no-animation case.
- [ ] 3.3: Integrate with Animation Blueprint.
  - [ ] 3.3.1: Link `UPBDSoftBodyComponent` to Animation Blueprint events.
  - [ ] 3.3.2: Test with animated sequence (e.g., `ThirdPersonIdle`).

### Task 4: Physics Simulation
- [ ] 4.1: Implement PBD constraints.
  - [ ] 4.1.1: Add stretch constraints between vertices.
  - [ ] 4.1.2: Add bend constraints for cluster stability.
- [ ] 4.2: Simulate velocities and positions.
  - [ ] 4.2.1: Update `Velocities` with gravity and constraints.
  - [ ] 4.2.2: Integrate positions using PBD solver.
- [ ] 4.3: Test physics simulation.
  - [ ] 4.3.1: Verify deformation with simple drop test.

### Task 5: Optimization and Debugging
- [x] 5.1: Add logging (`bEnableDebugLogging`, `bVerboseDebugLogging`).
- [ ] 5.2: Optimize cluster performance.
  - [ ] 5.2.1: Profile clustering algorithm for large meshes.
  - [ ] 5.2.2: Reduce memory usage in `SimulatedPositions`.
- [ ] 5.3: Add debug visualization.
  - [ ] 5.3.1: Draw cluster centroids in editor.

### Task 6: Testing and Documentation
- [ ] 6.1: Unit testing.
  - [ ] 6.1.1: Test vertex position retrieval with static mesh.
  - [ ] 6.1.2: Test clustering with varying `NumClusters`.
- [ ] 6.2: Integration testing.
  - [ ] 6.2.1: Verify simulation with animated and static meshes.
- [ ] 6.3: Documentation.
  - [ ] 6.3.1: Document `UPBDSoftBodyComponent` properties and methods.
  - [ ] 6.3.2: Create usage guide for plugin.

## Current State
- Task 2.3 completed with logging and animation fallback (tested with `SKM_Quinn`, 45993 vertices).
- Next: Task 2.4 (apply `SimulatedPositions` to mesh).

## Milestones
- **Milestone 1**: Basic clustering and blending (Done).
- **Milestone 2**: Full simulation with physics (Pending).
- **Milestone 3**: Optimized and documented plugin (Pending).


## Tasks in Detail
### Task 1: Setup Plugin Structure
Objective: Establish a robust plugin foundation optimized for Unreal Engine 5.5.3.
- Details: Configure .uplugin and Build.cs, set up directories (/Source/, /Resources/, /Content/), and ensure basic compilation.
- Test: Verify the plugin loads in the UE Editor with no errors.

### Task 2: Implement Core Component with Blending and Optimizations
Objective: Build UPBDSoftBodyComponent incrementally with high performance, stability, and editor-friendly features.

### Subtask 2.1: Define Component Structure and Basic Properties
- Objective: Set up the class skeleton with essential properties and minimal functionality.
- Details:
  - Create PBDSoftBodyComponent.h:
    ```cpp
    #pragma once
    #include "CoreMinimal.h"
    #include "Components/SkeletalMeshComponent.h"
    #include "PBDSoftBodyComponent.generated.h"
          
    UCLASS(ClassGroup = (Simulation), meta = (BlueprintSpawnableComponent))
    class PBDSOFTBODYPLUGIN_API UPBDSoftBodyComponent : public USkeletalMeshComponent {
        GENERATED_BODY()
    public:
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soft Body", meta = (ClampMin = "0.0", ClampMax = "1.0"))
        float SoftBodyBlendWeight = 0.5f;
          
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soft Body")
        bool bAutoRegister = true;
          
        virtual void BeginPlay() override;
        virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    };
    ```
  - Create PBDSoftBodyComponent.cpp:
    ```cpp
    #include "PBDSoftBodyComponent.h"
          
    void UPBDSoftBodyComponent::BeginPlay() {
        Super::BeginPlay();
    }
          
    void UPBDSoftBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    }
    ```
- Test: Add the component to a blueprint (BP_SoftBodyActor) with SK_Mannequin, compile, and ensure it appears in the editor with no crashes.

### Subtask 2.2: Add Simulation Data Structures
- Objective: Introduce data arrays for simulation (positions, velocities, etc.) and ensure memory efficiency.
- Details:
  - Update PBDSoftBodyComponent.h:
    ```cpp
    UPROPERTY(VisibleAnywhere, Category = "Soft Body")
    TArray<FVector> Velocities;
          
    UPROPERTY(VisibleAnywhere, Category = "Soft Body")
    TArray<FVector> SimulatedPositions;
          
    void InitializeSimulationData();
    TArray<FVector> GetVertexPositions() const;
    ```
  - Update PBDSoftBodyComponent.cpp:
    ```cpp
    void UPBDSoftBodyComponent::InitializeSimulationData() {
        if (SkeletalMesh) {
            int32 VertexCount = SkeletalMesh->GetVertexCountImported();
            Velocities.Reserve(VertexCount);
            SimulatedPositions.Reserve(VertexCount);
            Velocities.SetNum(VertexCount, false);
            SimulatedPositions.SetNum(VertexCount, false);
            for (int32 i = 0; i < VertexCount; i++) {
                Velocities[i] = FVector::ZeroVector;
                SimulatedPositions[i] = FVector::ZeroVector;
            }
        }
    }
          
    TArray<FVector> UPBDSoftBodyComponent::GetVertexPositions() const {
        TArray<FVector> Positions;
        if (SkeletalMesh) {
            Positions = SkeletalMesh->GetSkinnedVertexPositions(/*LODIndex*/ 0);
        }
        return Positions;
    }
          
    void UPBDSoftBodyComponent::BeginPlay() {
        Super::BeginPlay();
        InitializeSimulationData();
    }
    ```
- Test: Compile, attach to BP_SoftBodyActor, and use a debug log (UE_LOG) to verify Velocities and SimulatedPositions match the mesh’s vertex count.

### Subtask 2.3: Implement Basic Animation Blending
- Objective: Blend skeletal animation with simulated positions using SoftBodyBlendWeight.
- Details:
  - Update PBDSoftBodyComponent.h:
    ```cpp
    void UpdateBlendedPositions();
    ```
  - Update PBDSoftBodyComponent.cpp:
    ```cpp
    void UPBDSoftBodyComponent::UpdateBlendedPositions() {
        TArray<FVector> AnimatedPositions = GetVertexPositions();
        if (AnimatedPositions.Num() != SimulatedPositions.Num()) return;
          
        for (int32 i = 0; i < AnimatedPositions.Num(); i++) {
            SimulatedPositions[i] = FMath::Lerp(AnimatedPositions[i], SimulatedPositions[i], SoftBodyBlendWeight);
        }
    }
          
    void UPBDSoftBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
        Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
        UpdateBlendedPositions();
    }
    ```
- Test: Compile, run in-editor, adjust SoftBodyBlendWeight in BP_SoftBodyActor, and visually confirm blending affects the mesh (even if simulation isn’t active yet).

### Subtask 2.4: Add Clustering for Optimization
- Objective: Implement constraint clustering to optimize simulation performance.
- Details:
  - Update PBDSoftBodyComponent.h:
    ```cpp
    USTRUCT()
    struct FCluster {
        GENERATED_BODY()
        int32 CentroidIdx;
        TArray<int32> VertexIndices;
        float Stiffness = 1.0f;
    };
          
    UPROPERTY(VisibleAnywhere, Category = "Soft Body")
    TArray<FCluster> Clusters;
          
    void ComputeClusters(int32 ClusterCount = 2000);
    ```
  - Update PBDSoftBodyComponent.cpp:
    ```cpp
    void UPBDSoftBodyComponent::ComputeClusters(int32 ClusterCount) {
        if (!SkeletalMesh) return;
        TArray<FVector> Positions = GetVertexPositions();
        if (Positions.Num() == 0) return;
          
        Clusters.Reset();
        Clusters.Reserve(ClusterCount);
        int32 VerticesPerCluster = Positions.Num() / ClusterCount;
          
        for (int32 i = 0; i < ClusterCount; i++) {
            FCluster Cluster;
            Cluster.CentroidIdx = i * VerticesPerCluster;
            for (int32 j = 0; j < VerticesPerCluster && (i * VerticesPerCluster + j) < Positions.Num(); j++) {
                Cluster.VertexIndices.Add(i * VerticesPerCluster + j);
            }
            Clusters.Add(Cluster);
        }
    }
          
    void UPBDSoftBodyComponent::BeginPlay() {
        Super::BeginPlay();
        InitializeSimulationData();
        ComputeClusters(2000);
    }
    ```
- Test: Compile, log cluster data (e.g., Clusters.Num()), and ensure clusters cover all vertices without overlap or crashes.

### Subtask 2.5: Integrate with Subsystem and Finalize Optimizations
- Objective: Connect the component to the subsystem and add performance optimizations.
- Details:
  - Update PBDSoftBodyComponent.h:
    ```cpp
        UPROPERTY(VisibleAnywhere, Category = "Soft Body")
        bool bUseDoubleBuffering = true;
          
        TArray<FVector> PreviousPositions;
    ```
  - Update PBDSoftBodyComponent.cpp:
    ```cpp
    #include "PBDSoftBodySubsystem.h"
          
    void UPBDSoftBodyComponent::InitializeSimulationData() {
        if (SkeletalMesh) {
            int32 VertexCount = SkeletalMesh->GetVertexCountImported();
            Velocities.SetNum(VertexCount, false);
            SimulatedPositions.SetNum(VertexCount, false);
            if (bUseDoubleBuffering) {
                PreviousPositions.SetNum(VertexCount, false);
            }
        }
    }
          
    void UPBDSoftBodyComponent::BeginPlay() {
        Super::BeginPlay();
        InitializeSimulationData();
        ComputeClusters(2000);
        if (bAutoRegister) {
            if (UPBDSoftBodySubsystem* Subsystem = GetWorld()->GetGameInstance()->GetSubsystem<UPBDSoftBodySubsystem>()) {
                Subsystem->RegisterComponent(this);
            }
        }
    }
          
    void UPBDSoftBodyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
        if (bAutoRegister) {
            if (UPBDSoftBodySubsystem* Subsystem = GetWorld()->GetGameInstance()->GetSubsystem<UPBDSoftBodySubsystem>()) {
                Subsystem->UnregisterComponent(this);
            }
        }
        Super::EndPlay(EndPlayReason);
    }
    ```
- Test: Compile with Task 3 (Subsystem) partially implemented, verify registration/unregistration works, and check double buffering doesn’t crash (even if simulation isn’t fully active).

### Task 3: Implement Subsystem
- Objective: Develop UPBDSoftBodySubsystem to manage components efficiently.
- Details: Use UGameInstanceSubsystem, implement RegisterComponent, UnregisterComponent, and SimulateSoftBodies.
- Test: Ensure components register and simulation runs without errors (placeholder simulation logic initially).

### Task 4: Implement Muscle Shader with Blending and Optimizations
- Objective: Build MuscleDeform.usf for GPU-accelerated simulation.
- Details: Use SM5.0, define buffers, implement DQS/Implicit/XPBD modes.
- Test: Compile shader, hook it to the component, and verify basic position updates.

### Task 5: Region-Based Control and Editor Preview
- Objective: Add region control via vertex colors and editor preview.
- Details: Extend component with bUseRegionControl and bEnableEditorPreview.
- Test: Paint vertex colors on SK_Mannequin, confirm region effects, and check editor simulation.

### Task 6: Build and Test with UE 5.5.3 Binary
- Objective: Validate the plugin with Unreal Engine 5.5.3 binary.
- Details: Test with BP_SoftBodyActor, profile performance (6-11 ms target).
- Test: Ensure stability and compatibility with no engine source modifications.


