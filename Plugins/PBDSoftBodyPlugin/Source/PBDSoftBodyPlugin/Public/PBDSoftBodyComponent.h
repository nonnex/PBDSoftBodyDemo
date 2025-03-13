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