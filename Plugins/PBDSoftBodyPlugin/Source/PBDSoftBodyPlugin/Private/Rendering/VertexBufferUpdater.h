#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PBDSoftBodyComponent.h"
#include "VertexBufferUpdater.generated.h"

UCLASS()
class PBDSOFTBODYPLUGIN_API UVertexBufferUpdater : public UObject
{
    GENERATED_BODY()

public:
    void ApplyPositions(UPBDSoftBodyComponent* Component);
};