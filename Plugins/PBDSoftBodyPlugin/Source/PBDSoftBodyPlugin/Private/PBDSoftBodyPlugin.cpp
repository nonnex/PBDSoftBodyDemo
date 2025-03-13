#include "PBDSoftBodyPlugin.h"

#define LOCTEXT_NAMESPACE "FPBDSoftBodyPluginModule"

void FPBDSoftBodyPluginModule::StartupModule() {
    // No startup logic needed yet
}

void FPBDSoftBodyPluginModule::ShutdownModule() {
    // No shutdown logic needed yet
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPBDSoftBodyPluginModule, PBDSoftBodyPlugin)