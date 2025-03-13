#include "PBDSoftBodyComponent.h"

void UPBDSoftBodyComponent::BeginPlay() {
    Super::BeginPlay();
}

void UPBDSoftBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}