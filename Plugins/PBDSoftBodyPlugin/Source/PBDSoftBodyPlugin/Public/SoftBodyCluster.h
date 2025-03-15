#pragma once

#include "CoreMinimal.h"
#include "SoftBodyCluster.generated.h"

USTRUCT(BlueprintType)
struct PBDSOFTBODYPLUGIN_API FSoftBodyCluster
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PBD Soft Body")
    FVector CentroidPosition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PBD Soft Body")
    FVector CentroidVelocity;

    // Internal, not exposed to Blueprint
    TArray<int32> VertexIndices;
    TArray<FVector> VertexOffsets;
};