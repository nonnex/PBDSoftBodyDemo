#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PBDSoftBodyComponent.h"
#include "ClusterManager.generated.h"

UCLASS()
class PBDSOFTBODYPLUGIN_API UClusterManager : public UObject
{
    GENERATED_BODY()

public:
    void GenerateClusters(UPBDSoftBodyComponent* Component, const TArray<FVector>& VertexPositions);
};