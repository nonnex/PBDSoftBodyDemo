#include "PBDSoftBodyPlugin/Private/Rendering/VertexBufferUpdater.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SoftBodyCluster.h"

void UVertexBufferUpdater::ApplyPositions(UPBDSoftBodyComponent* Component)
{
    if (!Component || Component->SimulatedPositions.Num() == 0)
    {
        if (Component && Component->bEnableDebugLogging && Component->GetOwner())
        {
            UE_LOG(LogTemp, Warning, TEXT("VertexBufferUpdater: ApplyPositions - No simulated positions for %s."), *Component->GetOwner()->GetName());
        }
        return;
    }

    USkeletalMesh* Mesh = Component->GetSkeletalMeshAsset();
    if (!Mesh || !Mesh->GetResourceForRendering())
    {
        if (Component->bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("VertexBufferUpdater: Failed to get skeletal mesh or rendering resource for %s."), *Component->GetOwner()->GetName());
        }
        return;
    }

    FSkeletalMeshLODRenderData* LODRenderData = Mesh->GetResourceForRendering()->LODRenderData.IsValidIndex(0)
        ? &Mesh->GetResourceForRendering()->LODRenderData[0]
        : nullptr;
    if (!LODRenderData)
    {
        if (Component->bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("VertexBufferUpdater: No LODRenderData for applying positions in %s."), *Mesh->GetName());
        }
        return;
    }

    FPositionVertexBuffer& PositionBuffer = LODRenderData->StaticVertexBuffers.PositionVertexBuffer;
    if (PositionBuffer.GetNumVertices() != Component->SimulatedPositions.Num())
    {
        if (Component->bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("VertexBufferUpdater: Vertex count mismatch when applying positions. Buffer: %d, Simulated: %d."),
                PositionBuffer.GetNumVertices(), Component->SimulatedPositions.Num());
        }
        return;
    }

    ENQUEUE_RENDER_COMMAND(UpdateSoftBodyPositions)(
        [Component, &PositionBuffer](FRHICommandListImmediate& RHICmdList)
        {
            FBufferRHIRef& VertexBuffer = PositionBuffer.VertexBufferRHI;
            if (!VertexBuffer.IsValid())
            {
                if (Component->bEnableDebugLogging)
                {
                    UE_LOG(LogTemp, Warning, TEXT("VertexBufferUpdater: Vertex buffer not valid for %s."), *Component->GetOwner()->GetName());
                }
                return;
            }

            void* VertexData = RHICmdList.LockBuffer(VertexBuffer.GetReference(), 0, PositionBuffer.GetNumVertices() * sizeof(FVector3f), RLM_WriteOnly);
            if (VertexData)
            {
                FVector3f* Positions = static_cast<FVector3f*>(VertexData);
                for (int32 i = 0; i < Component->SimulatedPositions.Num(); i++)
                {
                    Positions[i] = FVector3f(Component->SimulatedPositions[i].X, Component->SimulatedPositions[i].Y, Component->SimulatedPositions[i].Z);
                }
                RHICmdList.UnlockBuffer(VertexBuffer.GetReference());

                if (Component->bEnableDebugLogging && !Component->bHasLoggedBlending && Component->GetSkeletalMeshAsset()->GetName().Contains(TEXT("SKM_Quinn")))
                {
                    UE_LOG(LogTemp, Log, TEXT("VertexBufferUpdater: Successfully applied %d simulated positions to SKM_Quinn."), Component->SimulatedPositions.Num());
                }
            }
            else
            {
                if (Component->bEnableDebugLogging)
                {
                    UE_LOG(LogTemp, Warning, TEXT("VertexBufferUpdater: Failed to lock vertex buffer for %s."), *Component->GetOwner()->GetName());
                }
            }
        });

    Component->MarkRenderStateDirty();
}