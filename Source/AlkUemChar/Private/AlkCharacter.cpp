// Copyright 2015-2021 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#include "AlkCharacter.h"

#include "EngineMinimal.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "IXRTrackingSystem.h"
#include "Kismet/KismetSystemLibrary.h" // for PrintString(...)
//#include "VRNotificationsComponent.h"

#include "AlkPureWorld.h"

#if 0 // TODO: @@@ Pimpl idiom requires subclasses match constructors
struct AAlkCharacter::ImplData {
  FVector  HMDPosition;
  FRotator HMDOrientation;
};

AAlkCharacter::AAlkCharacter(FObjectInitializer const & ObjectInitializer) :
  Super(ObjectInitializer),
  implData(MakeUnique<AAlkCharacter::ImplData()>())
{}

AAlkCharacter::AAlkCharacter(FVTableHelper& Helper) :
  Super(Helper),
  implData(MakeUnique<AAlkCharacter::ImplData()>())
{}

AAlkCharacter::~AAlkCharacter() = default;
#endif

void
AAlkCharacter::completeConstruction(int const inOptions) {
  Options = inOptions;

  if (VRReplicatedCamera)
    VRReplicatedCamera->SetRelativeLocation(
      FVector(0.f, 0.f, 165.f)); // default human eye height

#if 0 // TODO: @@@ MAY BE MORE EFFICIENT THAN CHECKING IN TICK BELOW
  auto vrNotifComp = CreateDefaultSubobject<UVRNotificationsComponent>(TEXT("AlkVRNotifications"));
  vrNotifComp->SetupAttachment(RootComponent)
  vrNotifComp->HMDPutOnHeadDelegate = FVRNotificationsDelegate(
  vrNotifComp->HMDRemovedFromHeadDelegate = FVRNotificationsDelegate(
  vrNotifComp->VRControllerRecenteredDelegate = FVRNotificationsDelegate(
#endif

#if 0 // TODO: @@@ DEPRECATED, NOW SEEMS UNNECESSARY SUBCLASSING AVRCharacter
  // !!! insert HMD offset to adjust its incorrect location within the capsule [c4augustus]
  // !!! (see https://forums.unrealengine.com/development-discussion/vr-ar-development/84475-set-tracking-origin-floor-level-not-matching-up-with-real-floor?112313-Set-Tracking-Origin-Floor-Level-Not-matching-up-with-real-floor=)
  // !!! (see https://www.reddit.com/r/oculus/comments/4lqtbi/unreal_engine_4_has_a_significant_problem_with/)
  AlkNodeHmdOffset = CreateDefaultSubobject<USceneComponent>(TEXT("AlkNodeHmdOffset"));
  AlkNodeHmdOffset->SetupAttachment(GetCapsuleComponent());
#endif

#if 0 // TODO: @@@ DEPRECATED, NOW PROVIDED BY AVRCharacter
  AlkMotionControllerL = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("AlkMotionControllerL"));
  AlkMotionControllerL->MotionSource = FXRMotionControllerBase::LeftHandSourceId;
  AlkMotionControllerL->SetupAttachment(
    hasAnyOptions(OPTION_VR_3DOF)
    ? AlkNodeHmdOffset // !!! motion controller is relative to HMD
    : RootComponent
  );
  AlkMotionControllerR = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("AlkMotionControllerR"));
  AlkMotionControllerR->MotionSource = FXRMotionControllerBase::RightHandSourceId;
  AlkMotionControllerR->SetupAttachment(
    hasAnyOptions(OPTION_VR_3DOF)
    ? AlkNodeHmdOffset // !!! motion controller is relative to HMD
    : RootComponent
  );
#endif

  if (hasAnyOptions(OPTION_CAN_SHOOT)) {
    AlkNodeShootMotionControllerL = CreateDefaultSubobject<USceneComponent>(TEXT("AlkNodeShootMotionControllerL"));
    AlkNodeShootMotionControllerR = CreateDefaultSubobject<USceneComponent>(TEXT("AlkNodeShootMotionControllerR"));
    if (LeftMotionController)
      AlkNodeShootMotionControllerL->SetupAttachment(LeftMotionController);
    if (RightMotionController)
      AlkNodeShootMotionControllerR->SetupAttachment(RightMotionController);
    AlkShootOffset = FVector(0.0f, 0.0f, 0.0f);
  }
  // blueprintables
  AlkInputDragDegPerViewport = FVector2D(360.f, 144.f);
    // !!! ^ account for the UE PlayerController values of
    // !!! InputYawScale (default 2.5) and
    // !!! InputPitchScale (default -2.5)
  AlkInputDragThresholdPixels = 4.f;
  AlkInputHoldThresholdSeconds = 0.3f;
  AlkLookRateDegPerSec = 45.f;
  AlkTurnRateDegPerSec = 45.f;
  AlkTracing = false;
}

void AAlkCharacter::SetupPlayerInputComponent(
  class UInputComponent* PlayerInputComponent
) {
  if (!PlayerInputComponent)
    return; // TODO: @@@ LOG FAILURE
  PlayerInputComponent->BindAction("AlkFireOrHold", IE_Pressed, this, &AAlkCharacter::InputFireOrHoldPressed);
  PlayerInputComponent->BindAction("AlkFireOrHold", IE_Released, this, &AAlkCharacter::InputFireOrHoldReleased);
  PlayerInputComponent->BindAction("AlkRecenterXR", IE_Pressed, this, &AAlkCharacter::InputRecenterXR);
  if (!hasAnyOptions(OPTION_NO_JUMP)) {
    PlayerInputComponent->BindAction("AlkJump", IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction("AlkJump", IE_Released, this, &ACharacter::StopJumping);
  }
  if (!hasAnyOptions(OPTION_NO_MOVE)) {
    PlayerInputComponent->BindAxis("AlkMoveForward", this, &AAlkCharacter::InputMoveForward);
    PlayerInputComponent->BindAxis("AlkMoveRight", this, &AAlkCharacter::InputMoveRight);
    PlayerInputComponent->BindAction("AlkSnapMoveBackward", IE_Pressed, this, &AAlkCharacter::InputSnapMoveBackward);
    PlayerInputComponent->BindAction("AlkSnapMoveForward", IE_Pressed, this, &AAlkCharacter::InputSnapMoveForward);
    PlayerInputComponent->BindAction("AlkSnapMoveLeft", IE_Pressed, this, &AAlkCharacter::InputSnapMoveLeft);
    PlayerInputComponent->BindAction("AlkSnapMoveRight", IE_Pressed, this, &AAlkCharacter::InputSnapMoveRight);
  }
  PlayerInputComponent->BindAction("AlkSnapTurnBack", IE_Pressed, this, &AAlkCharacter::InputSnapTurnBack);
  PlayerInputComponent->BindAction("AlkSnapTurnLeft", IE_Pressed, this, &AAlkCharacter::InputSnapTurnLeft);
  PlayerInputComponent->BindAction("AlkSnapTurnRight", IE_Pressed, this, &AAlkCharacter::InputSnapTurnRight);

  PlayerInputComponent->BindAction("AlkRotateDragWhile", IE_Pressed, this, &AAlkCharacter::InputRotateDragEnable);
  PlayerInputComponent->BindAction("AlkRotateDragWhile", IE_Released, this, &AAlkCharacter::InputRotateDragDisable);
  PlayerInputComponent->BindAxis("AlkMouseX", this, &AAlkCharacter::InputMouseAxis);
  PlayerInputComponent->BindAxis("AlkMouseY", this, &AAlkCharacter::InputMouseAxis);

  PlayerInputComponent->BindAxis("AlkLook", this, &APawn::AddControllerPitchInput);
  PlayerInputComponent->BindAxis("AlkLookRate", this, &AAlkCharacter::InputLookRate);
  PlayerInputComponent->BindAxis("AlkTurn", this, &APawn::AddControllerYawInput);
  PlayerInputComponent->BindAxis("AlkTurnRate", this, &AAlkCharacter::InputTurnRate);

  if (FPlatformMisc::GetUseVirtualJoysticks()
      || GetDefault<UInputSettings>()->bUseMouseForTouch) {
    PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AAlkCharacter::InputTouchPressed);
    PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AAlkCharacter::InputTouchReleased);
    PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AAlkCharacter::InputTouchDragged);
  }
}

void AAlkCharacter::Tick(float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  UpdateHMDState(DeltaSeconds);
  UpdateHoldingState(DeltaSeconds);
}

#if 0 // TODO: ### FOR SCREEN TO WORLD COORDINATES
struct LocationRotation {
  FVector Location;
  FVector Rotation;
};

static LocationRotation GazeToWorld(
  UWorld* World,
  FVector const ScreenCoordinates
) {

}
#endif

void AAlkCharacter::AlkOnHoldEnter_Implementation(
  FVector const & ScreenCoordinates
) {
  // TODO: ### IMPLEMENT
}

void AAlkCharacter::AlkOnHoldLeave_Implementation(
  FVector const & ScreenCoordinates
) {
  // TODO: ### IMPLEMENT
}

void AAlkCharacter::AlkOnHoldMove_Implementation(
  FVector const & ScreenCoordinates
) {
  // TODO: ### IMPLEMENT
}

void AAlkCharacter::AlkOnShoot_Implementation(
  FVector const & ScreenCoordinates
) {
  if (AlkTracing)
    UKismetSystemLibrary::PrintString(this, FString(TEXT("AlkOnShoot_Implementation(...)")));
  if (!hasAnyOptions(OPTION_CAN_SHOOT))
    return;
  auto world = GetWorld();
  if (world && AlkProjectileClass) {
    FRotator const SpawnRotation = bAlkUsingMotionControllers
      ? (bAlkShootFromMotionControllerLeftNotRight
        ? AlkNodeShootMotionControllerL->GetComponentRotation()
        : AlkNodeShootMotionControllerR->GetComponentRotation())
      : GetControlRotation();
    FVector const SpawnLocation = (bAlkUsingMotionControllers
      ? (bAlkShootFromMotionControllerLeftNotRight
        ? AlkNodeShootMotionControllerL->GetComponentLocation()
        : AlkNodeShootMotionControllerR->GetComponentLocation())
      : (AlkNodeShootDefault)
        ? AlkNodeShootDefault->GetComponentLocation()
        : GetActorLocation()
     ) + SpawnRotation.RotateVector(AlkShootOffset);
    FActorSpawnParameters ActorSpawnParams;
    ActorSpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
    world->SpawnActor<AActor>(AlkProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
  }
  if (AlkShootSound)
    UGameplayStatics::PlaySoundAtLocation(
      this, AlkShootSound, GetActorLocation());
}

void AAlkCharacter::AlkOnFire_Implementation(
  FVector const & ScreenCoordinates
) {
  if (AlkTracing)
    UKismetSystemLibrary::PrintString(this, FString(TEXT("AlkOnFire_Implementation(...)")));
  if (hasAnyOptions(OPTION_CAN_SHOOT))
    AlkOnShoot(ScreenCoordinates);
}

constexpr int HMDUpdateFrequencySeconds = 1.f;

void AAlkCharacter::ApplyHMDState() {
   if (VRReplicatedCamera)
     VRReplicatedCamera->bUsePawnControlRotation = !HMDState.Worn;
   if (AlkTracing)
     UKismetSystemLibrary::PrintString(this,
       HMDState.Worn ? FString(TEXT("HMD worn"))
                     : FString(TEXT("HMD NOT worn")));
}

void AAlkCharacter::UpdateHMDState(float const DeltaSeconds) {
  HMDState.UpdateDeltaSeconds += DeltaSeconds;
  if (   (HMDState.UpdateTotalSeconds > 0.f)
      && (HMDState.UpdateDeltaSeconds < HMDUpdateFrequencySeconds))
    return;
  FRotator orientation;
  FVector position;
  UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(
    orientation, position);
  bool worn = (HMDState.Orientation != orientation)
              || (HMDState.Position != position);
  if (worn) {
    HMDState.Orientation = orientation;
    HMDState.Position = position;
  }
  if (   (HMDState.UpdateTotalSeconds == 0.f)
      || (HMDState.Worn != worn)) {
    HMDState.Worn = worn;
    ApplyHMDState();
  }
  HMDState.UpdateTotalSeconds += HMDState.UpdateDeltaSeconds;
  HMDState.UpdateDeltaSeconds = 0.f;
#if 0 // TODO: @@@ SteamVR DOES NOT PROPERLY INDICATE WornState
  static auto prevWornState = EHMDWornState::Unknown;
  auto nextWornState = UHeadMountedDisplayFunctionLibrary::GetHMDWornState();
  if (nextWornState != prevWornState) {
    prevWornState = nextWornState;
    bool worn = (nextWornState == EHMDWornState::Worn);
    UKismetSystemLibrary::PrintString(this, FString(TEXT("HMDWornState")));
    if (VRReplicatedCamera)
      VRReplicatedCamera->bUsePawnControlRotation = !worn;
  }
#endif
}

void AAlkCharacter::UpdateHoldingState(float const DeltaSeconds) {
  if (HoldMeasuring) {
    HoldSeconds += DeltaSeconds;
    if (!AlkHolding && HoldSeconds >= AlkInputHoldThresholdSeconds) {
      auto const mousePos = UpdateViewportMousePositionReturnDelta();
      EnterHolding(FVector(mousePos.X, mousePos.Y, 0));
        // TODO: ^ ### ASSUMING MOUSE
    }
  }
}

void AAlkCharacter::UpdateViewportState() {
  ViewportSize = pure::WorldGameViewportSize(GetWorld());
  ViewportDragThresholdRatio =
    FVector2D(AlkInputDragThresholdPixels,
              AlkInputDragThresholdPixels)
    / ViewportSize;
  if (AlkTracing)
    UKismetSystemLibrary::PrintString(this,
      FString::Printf(TEXT("ViewportSize(%f,%f)"),
        ViewportSize.X, ViewportSize.Y));
}

auto AAlkCharacter::UpdateViewportMousePositionReturnDelta() -> FVector2D{
  auto const mousePos = pure::WorldGameViewportMousePosition(GetWorld());
  auto const deltaPos = mousePos - ViewportMousePosition;
  ViewportMousePosition = mousePos;
  return deltaPos;
}

void AAlkCharacter::EnterHolding(FVector const & ScreenCoordinates) {
  AlkHolding = true;
  AlkOnHoldEnter(ScreenCoordinates);
}

void AAlkCharacter::LeaveHolding(FVector const & ScreenCoordinates) {
  AlkHolding = false;
  AlkOnHoldLeave(ScreenCoordinates);
}

void AAlkCharacter::StartHoldMeasuring() {
  HoldMeasuring = true;
  HoldSeconds = 0.f;
}

void AAlkCharacter::StopHoldMeasuring() {
  HoldMeasuring = false;
  HoldSeconds = 0.f;
}

void AAlkCharacter::InputFireOrHoldPressed() {
  if (AlkTracing)
    UKismetSystemLibrary::PrintString(this, FString(TEXT("InputFireOrHold()")));
  StartHoldMeasuring();
}

void AAlkCharacter::InputFireOrHoldReleased() {
  if (AlkTracing)
    UKismetSystemLibrary::PrintString(this, FString(TEXT("InputFireOrHold()")));
  StopHoldMeasuring();
  if (!AlkHolding)
    AlkOnFire(FVector()); // TODO: ### WE DON'T HAVE SCREEN COORDINATES
  else
    LeaveHolding(FVector()); // TODO: ### WE DON'T HAVE SCREEN COORDINATES
}

void AAlkCharacter::InputRecenterXR() {
  auto xrTrackingSystem = GEngine->XRSystem; // interface to HMD
  if (!xrTrackingSystem)
    return;
  auto posBefore = xrTrackingSystem->GetBasePosition();
  UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
    // !!! ^ in room scale, position Z will be offset to the floor
  auto posAfter = xrTrackingSystem->GetBasePosition();
  xrTrackingSystem->SetBasePosition(
    FVector(posAfter.X, posAfter.Y, posBefore.Z));
}

void AAlkCharacter::InputMoveForward(float const Value) {
  if (Value != 0.0f) AddMovementInput(
      // GetActorRightVector(), Value);
      // !!! use VRBaseCharacter::
      GetVRForwardVector(), Value);
}

void AAlkCharacter::InputMoveRight(float const Value) {
  if (Value != 0.0f) AddMovementInput(
      // GetActorRightVector(), Value);
      // !!! use VRBaseCharacter::
      GetVRRightVector(), Value);
}

void AAlkCharacter::InputTurnRate(float const Rate) {
  auto world = GetWorld();
  if (world && Rate != 0.0f) AddControllerYawInput(
    Rate * AlkTurnRateDegPerSec * world->GetDeltaSeconds());
}

void AAlkCharacter::InputLookRate(float const Rate) {
  auto world = GetWorld();
  if (world && Rate != 0.0f) AddControllerPitchInput(
    Rate * AlkLookRateDegPerSec * world->GetDeltaSeconds());
}

void AAlkCharacter::InputRotateDragDisable() {
  bRotateDragEnabled = false;
  if (AlkTracing)
    UKismetSystemLibrary::PrintString(this,
      FString(TEXT("InputRotateDragDisable()")));
}

void AAlkCharacter::InputRotateDragEnable() {
  bRotateDragEnabled = true;
  // !!! update whenever dragging starts in case the viewport changed
  UpdateViewportState();
  UpdateViewportMousePositionReturnDelta();
  if (AlkTracing)
    UKismetSystemLibrary::PrintString(this,
      FString(TEXT("InputRotateDragEnable()")));
}

void AAlkCharacter::InputMouseAxis(float const Value) {
  if (Value == 0.f)
    return;
  // !!! we are not using the passed in Value because it is inconsistent
  // !!! due to project settings: input axis mapping scale, FOVScaling
  auto const mousePos = UpdateViewportMousePositionReturnDelta();
  if (AlkHolding)
    AlkOnHoldMove(FVector(mousePos.X, mousePos.Y, 0));
  if (HoldMeasuring)
    StopHoldMeasuring();
  if (bRotateDragEnabled)
    RotateDrag(mousePos);
}

void AAlkCharacter::InputSnapMoveBackward() {
  // TODO: ### IMPLEMENT
}

void AAlkCharacter::InputSnapMoveForward() {
  // TODO: ### IMPLEMENT
}

void AAlkCharacter::InputSnapMoveLeft() {
  // TODO: ### IMPLEMENT
}

void AAlkCharacter::InputSnapMoveRight() {
  // TODO: ### IMPLEMENT
}

void AAlkCharacter::InputSnapTurnBack() {
  // TODO: ### IMPLEMENT
}

void AAlkCharacter::InputSnapTurnLeft() {
  // TODO: ### IMPLEMENT
}

void AAlkCharacter::InputSnapTurnRight() {
  // TODO: ### IMPLEMENT
}

void AAlkCharacter::InputTouchDragged(
  ETouchIndex::Type const FingerIndex,
  FVector const Location
) {
  if (FingerIndex > ETouchIndex::MAX_TOUCHES)
    return; // TODO: @@@ LOG FAILURE
  FVector const deltaLoc = Location
    - TouchFingerStates[FingerIndex].Location;
  TouchFingerStates[FingerIndex].Location = Location;
  if (deltaLoc.X == 0.f && deltaLoc.Y == 0.f)
    return;
  if (FingerIndex == FingerIndexFire) {
    if (AlkHolding)
      AlkOnHoldMove(Location);
    else if (HoldMeasuring)
      StopHoldMeasuring();
  }
  if (FingerIndex == FingerIndexRotate) {
    if (!TouchFingerStates[FingerIndex].bDragged)
      // !!! update whenever dragging starts in case the viewport changed
      UpdateViewportState();
    else
      TouchFingerStates[FingerIndex].bDragged = true;
    RotateDrag(FVector2D(deltaLoc.X, deltaLoc.Y));
  }
}

void AAlkCharacter::InputTouchPressed(
  ETouchIndex::Type const FingerIndex,
  FVector const Location
) {
  if (AlkTracing)
    UKismetSystemLibrary::PrintString(this, FString(TEXT("InputTouchPressed(...)")));
  if (FingerIndex > ETouchIndex::MAX_TOUCHES)
    return; // TODO: @@@ LOG FAILURE
  if (TouchFingerStates[FingerIndex].bPressed)
    return; // TODO: @@@ LOG FAILURE
  TouchFingerStates[FingerIndex].Location = Location;
  TouchFingerStates[FingerIndex].bDragged = false;
  TouchFingerStates[FingerIndex].bPressed = true;
  TouchFingerStates[FingerIndex].PressedRealTimeSeconds =
    pure::WorldRealTimeSeconds(GetWorld());
  if (FingerIndex == FingerIndexFire)
    StartHoldMeasuring();
}

void AAlkCharacter::InputTouchReleased(
  ETouchIndex::Type const FingerIndex,
  FVector const Location
) {
  if (AlkTracing)
    UKismetSystemLibrary::PrintString(this, FString(TEXT("InputTouchReleased(...)")));
  if (FingerIndex > ETouchIndex::MAX_TOUCHES)
    return; // TODO: @@@ LOG FAILURE
  if (!TouchFingerStates[FingerIndex].bPressed)
    return; // TODO: @@@ LOG FAILURE
  TouchFingerStates[FingerIndex].Location = Location;
  TouchFingerStates[FingerIndex].bPressed = false;
  // TODO: @@@ ProjectSettings> Engine> Input> Mouse Properties>
  //       @@@ Use Mouse for Touch [x] will always generate InputTouchDragged
  //if (!TouchFingerStates[FingerIndex].bDragged &&
  if (FingerIndex == FingerIndexFire) {
    StopHoldMeasuring();
    if (AlkHolding)
      LeaveHolding(Location);
  }
  if (pure::WorldRealTimeSeconds(GetWorld())
        - TouchFingerStates[FingerIndex].PressedRealTimeSeconds
      < AlkInputHoldThresholdSeconds)
    InputTouchTapped(FingerIndex, Location);
}

void AAlkCharacter::InputTouchTapped(
  ETouchIndex::Type const FingerIndex,
  FVector const Location
) {
  if (AlkTracing)
    UKismetSystemLibrary::PrintString(this, FString(TEXT("InputTouchTapped(...)")));
  if (FingerIndex == FingerIndexFire)
    AlkOnFire(Location);
}

void AAlkCharacter::RotateDrag(FVector2D const & deltaPos) {
  if (   (deltaPos.X != 0 || deltaPos.Y != 0)
      && (ViewportSize.X > 0.f)
      && (ViewportSize.Y > 0.f)) {
    auto const vpRatio = deltaPos / ViewportSize;
    auto const degrees = vpRatio * AlkInputDragDegPerViewport;
    if (degrees.X != 0.f)
      AddControllerYawInput(degrees.X);
        // !!! ^ UE PlayerController applies InputYawScale
    if (degrees.Y != 0.f)
      AddControllerPitchInput(degrees.Y);
        // !!! ^ UE PlayerController applies InputPitchScale
    if (AlkTracing)
      UKismetSystemLibrary::PrintString(this,
        FString::Printf(TEXT("RotateDrag((%f,%f)): vpRatio (%f,%f), degrees (%f,%f)"),
          deltaPos.X, deltaPos.Y, vpRatio.X, vpRatio.Y, degrees.X, degrees.Y));
  }
}
