#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PBDSoftBodyComponent.h"
#include "AnimationBlender.generated.h"

UCLASS()
class PBDSOFTBODYPLUGIN_API UAnimationBlender : public UObject
{
    GENERATED_BODY()

public:
    // Changed from const to non-const since it modifies Component state
    TArray<FVector> GetVertexPositions(UPBDSoftBodyComponent* Component) const;
    void UpdateBlendedPositions(UPBDSoftBodyComponent* Component);
};