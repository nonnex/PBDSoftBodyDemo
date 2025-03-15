#include "PBDSoftBodyPlugin/Private/Simulation/ClusterManager.h"
#include "SoftBodyCluster.h"

void UClusterManager::GenerateClusters(UPBDSoftBodyComponent* Component, const TArray<FVector>& VertexPositions)
{
    if (!Component || VertexPositions.Num() == 0 || Component->NumClusters <= 0)
    {
        if (Component && Component->bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("ClusterManager: GenerateClusters - Invalid input: %d vertices, %d clusters."),
                VertexPositions.Num(), Component->NumClusters);
        }
        return;
    }

    int32 VerticesPerCluster = VertexPositions.Num() / Component->NumClusters;
    Component->Clusters.SetNum(Component->NumClusters, EAllowShrinking::No);

    if (Component->bVerboseDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("ClusterManager: Generating %d clusters with ~%d vertices each."),
            Component->NumClusters, VerticesPerCluster);
    }

    for (int32 ClusterIdx = 0; ClusterIdx < Component->NumClusters; ClusterIdx++)
    {
        FSoftBodyCluster& Cluster = Component->Clusters[ClusterIdx];
        Cluster.VertexIndices.Reset();
        Cluster.VertexOffsets.Reset();

        int32 StartIdx = ClusterIdx * VerticesPerCluster;
        int32 EndIdx = (ClusterIdx == Component->NumClusters - 1) ? VertexPositions.Num() : (ClusterIdx + 1) * VerticesPerCluster;

        for (int32 i = StartIdx; i < EndIdx; i++)
        {
            Cluster.VertexIndices.Add(i);
        }

        FVector Centroid = FVector::ZeroVector;
        for (int32 VertexIdx : Cluster.VertexIndices)
        {
            Centroid += VertexPositions[VertexIdx];
        }
        if (Cluster.VertexIndices.Num() > 0)
        {
            Centroid /= Cluster.VertexIndices.Num();
        }
        else
        {
            if (Component->bEnableDebugLogging)
            {
                UE_LOG(LogTemp, Warning, TEXT("ClusterManager: Cluster %d has no vertices assigned."), ClusterIdx);
            }
        }
        Cluster.CentroidPosition = Centroid;
        Cluster.CentroidVelocity = FVector::ZeroVector;

        Cluster.VertexOffsets.SetNum(Cluster.VertexIndices.Num(), EAllowShrinking::No);
        for (int32 i = 0; i < Cluster.VertexIndices.Num(); i++)
        {
            Cluster.VertexOffsets[i] = VertexPositions[Cluster.VertexIndices[i]] - Centroid;
        }

        if (Component->bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("ClusterManager: Cluster %d created with %d vertices."), ClusterIdx, Cluster.VertexIndices.Num());
        }
        if (Component->bVerboseDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("ClusterManager: Cluster %d centroid at (%.2f, %.2f, %.2f)."),
                ClusterIdx, Centroid.X, Centroid.Y, Centroid.Z);
        }
    }
}