#pragma once
#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "PBDSoftBodyComponent.generated.h"

USTRUCT(BlueprintType)
struct FSoftBodyCluster {
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soft Body Simulation")
    FVector CentroidPosition = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soft Body Simulation")
    FVector CentroidVelocity = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soft Body Simulation")
    TArray<int32> VertexIndices;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soft Body Simulation")
    TArray<FVector> VertexOffsets;
};

UCLASS(ClassGroup = (Simulation), meta = (BlueprintSpawnableComponent))
class PBDSOFTBODYPLUGIN_API UPBDSoftBodyComponent : public USkeletalMeshComponent {
    GENERATED_BODY()
public:
    UPBDSoftBodyComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soft Body", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SoftBodyBlendWeight = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soft Body")
    bool bAutoRegister = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soft Body Debugging")
    bool bEnableDebugLogging = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soft Body Debugging")
    bool bVerboseDebugLogging = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soft Body Clustering", meta = (ClampMin = "1"))
    int32 NumClusters = 10;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soft Body Simulation")
    TArray<FVector> Velocities;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soft Body Simulation")
    TArray<FVector> SimulatedPositions;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Soft Body Simulation")
    TArray<FSoftBodyCluster> Clusters;

    mutable bool bHasActiveAnimation = false;
    mutable bool bHasLoggedVertexCount = false;
    mutable bool bHasLoggedBlending = false;
    mutable bool bHasLoggedBlendingVerbose = false;

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    bool InitializeSimulationData();
    TArray<FVector> GetVertexPositions() const;
    void UpdateBlendedPositions();
    void GenerateClusters(const TArray<FVector>& VertexPositions);

    void InitializeConfig();
};