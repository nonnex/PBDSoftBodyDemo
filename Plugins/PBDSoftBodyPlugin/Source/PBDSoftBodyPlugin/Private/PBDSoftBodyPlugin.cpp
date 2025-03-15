#include "PBDSoftBodyPlugin.h"

#define LOCTEXT_NAMESPACE "FPBDSoftBodyPluginModule"

void FPBDSoftBodyPluginModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyPlugin: Module started."));
}

void FPBDSoftBodyPluginModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyPlugin: Module shut down."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPBDSoftBodyPluginModule, PBDSoftBodyPlugin)