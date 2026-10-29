#pragma once
#include <cstdint>
#include <cstddef>
struct ModernState {
    uint32_t base, backInput, backBasis, backPitch, backYaw;
    int32_t enabled;
    float sensitivity, yaw, pitch;
    uint32_t hero;
    int32_t active, frames;
    float lastDx,lastDy;
    float basis[9];
    int32_t wasAim, initialized;
    uint32_t backFacing;
    uint32_t backDistance,backRun,backAlternateAim;
    float explorationDistance,transitionSeconds,distanceCurrent,blendStart,blendElapsed;
    int32_t distanceInitialized,distanceAim,modernMode,holdShift;
    uint32_t backReload;
    int32_t scene,returnPending;
    float returnSeconds,resumeElapsed;
    int32_t reloadHeld,rReload;
    uint32_t backPadFrame,backPadHeld,backPadPressed,backPadAxis;
    int32_t controllerEnabled,padIndex,padConnected,padActive;
    float moveDeadzone,lookDeadzone,lookSpeed,aimScale;
    float padLX,padLY,padRX,padRY;
    uint32_t padButtons,padPrevious;
    float padLT,padRT;
    uint32_t backMelee,keyButtons,keyPrevious;
    int32_t meleeRequest,meleeCount;
    uint32_t backMenuFrame,backRawPressed,backRawHeld,backRawReleased,backPadReleased;
    uint32_t menuButtons,menuPrevious,menuFrames;
    int32_t scriptCamera;
    int32_t snapAfterDoors,doorLocked,doorSnapPending,doorSnapCount;
    uint32_t cameraHero;
    int32_t lastController;
};
static_assert(offsetof(ModernState,backMenuFrame)==280 && offsetof(ModernState,scriptCamera)==312,"Menu hook layout");
static_assert(offsetof(ModernState,backMelee)==260,"Melee hook layout");
static_assert(offsetof(ModernState,backPadFrame)==180 && offsetof(ModernState,padActive)==208,"Controller hook layout");
static_assert(offsetof(ModernState,backReload)==152 && offsetof(ModernState,scene)==156,"Scene hook layout");
static_assert(offsetof(ModernState,backDistance)==104 && offsetof(ModernState,backRun)==108 && offsetof(ModernState,backAlternateAim)==112,"Additional hook layout");
static_assert(offsetof(ModernState,backFacing)==100,"Facing return layout");
static_assert(offsetof(ModernState,enabled)==20 && offsetof(ModernState,pitch)==32 && offsetof(ModernState,active)==40,"Assembly layout");
