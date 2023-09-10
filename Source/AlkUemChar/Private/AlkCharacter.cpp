// Copyright 2015-2023 Alkaline Games, LLC.
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

#include "GripMotionControllerComponent.h"

#include "AlkPureMath.h"
#include "AlkPureWorld.h"

constexpr int HMDUpdateFrequencySeconds = 1.f;

struct AAlkCharacterImpl: AAlkCharacter::Impl {
  int Options = 0;
  bool bMouseMovingEnabled = false;
  bool bMouseTurningEnabled = false;
  struct HMDState {
    FRotator Orientation;
    FVector Position;
    float UpdateDeltaSeconds = 0.f;
    float UpdateTotalSeconds = 0.f;
    bool Worn = false;
  };
  struct HMDState HMDState;
  bool FireMeasuring = false;
  int FireRapidCount = 0;
  int FireRapidCurrent = 0;
  float FireSeconds = 0.f;
  bool HoldMeasuring = false;
  float HoldSeconds = 0.f;
  ETouchIndex::Type FingerIndexFire = ETouchIndex::Touch1;
  ETouchIndex::Type FingerIndexMove = ETouchIndex::Touch2;
  ETouchIndex::Type FingerIndexMoveOnly = ETouchIndex::Touch3;
  ETouchIndex::Type FingerIndexTurn = ETouchIndex::Touch1;
  struct TouchFingerState {
    ETouchIndex::Type FingerIndex = ETouchIndex::CursorPointerIndex;
    FVector Location = FVector::ZeroVector;
    bool bDragged = false;
    bool bPressed = false;
    float PressedRealTimeSeconds = 0.f;
  };
  struct TouchFingerState TouchFingerStates[ETouchIndex::MAX_TOUCHES];
  FVector2D ViewportDragThresholdRatio;
  FVector2D ViewportDivisor;
  FVector2D ViewportMousePosition;

  AAlkCharacter const & face;
  AAlkCharacter & face_mut;

  AAlkCharacterImpl(AAlkCharacter& face)
    : face(face), face_mut(face) {}

  ~AAlkCharacterImpl() {}

  void ApplyHMDState() {
    if (face_mut.VRReplicatedCamera)
      face_mut.VRReplicatedCamera->bUsePawnControlRotation = !HMDState.Worn;
    if (face.AlkTracing)
      UKismetSystemLibrary::PrintString(&face_mut,
        HMDState.Worn ? FString(TEXT("HMD worn"))
                      : FString(TEXT("HMD NOT worn")));
  }

  void UpdateHMDState(float const DeltaSeconds) {
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
    static auto const prevWornState = EHMDWornState::Unknown;
    auto const nextWornState = UHeadMountedDisplayFunctionLibrary::GetHMDWornState();
    if (nextWornState != prevWornState) {
      prevWornState = nextWornState;
      bool worn = (nextWornState == EHMDWornState::Worn);
      UKismetSystemLibrary::PrintString(&face_mut, FString(TEXT("HMDWornState")));
      if (VRReplicatedCamera)
        VRReplicatedCamera->bUsePawnControlRotation = !worn;
    }
#endif
  }

  void UpdateInputState(float const DeltaSeconds) {
    if (FireMeasuring) {
      FireSeconds += DeltaSeconds;
      if (FireSeconds >= face.AlkInputFireRapidThresholdSeconds)
        StopFireMeasuring();
    }
    if (HoldMeasuring) {
      HoldSeconds += DeltaSeconds;
      if (!face.AlkHolding && HoldSeconds >= face.AlkInputHoldThresholdSeconds) {
        EnterHolding(pure::VectorFromVector2D( // TODO: ### ASSUMING MOUSE
          UpdateViewportMousePositionReturnDelta()));
      }
    }
  }

  void UpdateViewportState() {
    auto const vpSize = pure::WorldGameViewportSize(face.GetWorld());
    ViewportDivisor = (vpSize.X > vpSize.Y)
      ?  FVector2D(vpSize.X, vpSize.X)
      :  FVector2D(vpSize.Y, vpSize.Y);
    ViewportDragThresholdRatio = // TODO: @@@ NOT YET USED
      FVector2D(face.AlkInputDragThresholdPixels,
                face.AlkInputDragThresholdPixels)
      / vpSize;
    if (face.AlkTracing)
      UKismetSystemLibrary::PrintString(&face_mut,
        FString::Printf(TEXT("viewport size %f x %f"), vpSize.X, vpSize.Y));
  }

  auto UpdateViewportMousePositionReturnDelta() -> FVector2D{
    auto const mousePos = pure::WorldGameViewportMousePosition(face.GetWorld());
    auto const deltaPos = mousePos - ViewportMousePosition;
    ViewportMousePosition = mousePos;
    return deltaPos;
  }

  void UpdatePointerWorldFromViewport(FVector2D vpPos) {
    auto const pc = face.GetLocalViewingPlayerController();
    if (pc) {
      FVector worldPos;
      pc->DeprojectScreenPositionToWorld(
        vpPos.X, vpPos.Y, worldPos, face_mut.AlkPointerWorldDirection);
    }
  }

  void UndoMouseDeltaPosition(FVector2D deltaPos) {
    SetMousePosition(ViewportMousePosition - deltaPos);
  }

  void SetMousePosition(FVector2D pos) {
    auto const pc = face.GetLocalViewingPlayerController();
    if (pc) {
      pc->SetMouseLocation(pos.X, pos.Y);
      UpdateViewportMousePositionReturnDelta();
    }
  }

  void EnterHolding(FVector const & ScreenCoordinates) {
    face_mut.AlkHolding = true;
    face_mut.AlkOnHoldEnter(ScreenCoordinates);
  }

  void LeaveHolding(FVector const & ScreenCoordinates) {
    face_mut.AlkHolding = false;
    face_mut.AlkOnHoldLeave(ScreenCoordinates);
  }

  void KeepFireMeasuring() {
    FireMeasuring = true;
    MaintainFireMeasuring();
  }

  void MaintainFireMeasuring() {
    FireSeconds = 0.f;
  }

  void StopFireMeasuring() {
    FireMeasuring = false;
    FireRapidCount = 0;
  }

  void StartHoldMeasuring() {
    HoldMeasuring = true;
    HoldSeconds = 0.f;
  }

  void StopHoldMeasuring() {
    HoldMeasuring = false;
    HoldSeconds = 0.f;
  }

  void HandleFireOrHoldPressed(
    FVector const & screenCoordinates
  ) {
    if (face.AlkFireRapidLimit > FireRapidCount)
      KeepFireMeasuring();
    else
      StopFireMeasuring();
    if (face.AlkHoldEnabled)
      StartHoldMeasuring();
    else {
      FireRapidCurrent = ++FireRapidCount;
      face_mut.AlkOnFire(screenCoordinates, FireRapidCurrent);
    }
  }

  void HandleFireOrHoldReleased(
    FVector const & screenCoordinates
  ) {
    if (face.AlkFireRapidLimit > FireRapidCount)
      MaintainFireMeasuring();
    if (face.AlkHoldEnabled) {
      StopHoldMeasuring();
      if (!face.AlkHolding) {
        FireRapidCurrent = ++FireRapidCount;
        face_mut.AlkOnFire(screenCoordinates, ++FireRapidCurrent);
      } else
        LeaveHolding(screenCoordinates);
    } else {
      face_mut.AlkOnFire(screenCoordinates, - FireRapidCurrent);
      FireRapidCurrent = 0;
    }
  }

  void InputFireOrHoldPressed() {
    if (face.AlkTracing)
      UKismetSystemLibrary::PrintString(&face_mut,
        FString(TEXT("InputFireOrHoldPressed()")));
    HandleFireOrHoldPressed(pure::VectorFromVector2D(
      // TODO: @@@ ASSUMING MOUSE, BUT WHAT ABOUT MOTIONCONTROLLERS?
      pure::WorldGameViewportMousePosition(face.GetWorld())));
  }

  void InputFireOrHoldReleased() {
    if (face.AlkTracing)
      UKismetSystemLibrary::PrintString(&face_mut,
        FString(TEXT("InputFireOrHoldReleased()")));
    HandleFireOrHoldReleased(pure::VectorFromVector2D(
      // TODO: @@@ ASSUMING MOUSE, BUT WHAT ABOUT MOTIONCONTROLLERS?
      pure::WorldGameViewportMousePosition(face.GetWorld())));
  }

  void InputRecenterXR() {
    auto const xrTrackingSystem = GEngine->XRSystem; // interface to HMD
    if (!xrTrackingSystem)
      return;
    auto const posBefore = xrTrackingSystem->GetBasePosition();
    UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
      // !!! ^ in room scale, position Z will be offset to the floor
    auto const posAfter = xrTrackingSystem->GetBasePosition();
    xrTrackingSystem->SetBasePosition(
      FVector(posAfter.X, posAfter.Y, posBefore.Z));
  }

  void InputMoveForward(float const Value) {
    if (Value != 0.0f) face_mut.AddMovementInput(
        // face.GetActorRightVector(), Value);
        // !!! use VRBaseCharacter::
        face.GetVRForwardVector(), Value);
  }

  void InputMoveRight(float const Value) {
    if (Value != 0.0f) face_mut.AddMovementInput(
        // face.GetActorRightVector(), Value);
        // !!! use VRBaseCharacter::
        face.GetVRRightVector(), Value);
  }

  void InputTurnRate(float const Rate) {
    auto const world = face.GetWorld();
    if (world && Rate != 0.0f) face_mut.AddControllerYawInput(
      Rate * face.AlkTurnRateDegPerSec * world->GetDeltaSeconds());
  }

  void InputLookRate(float const Rate) {
    auto const world = face.GetWorld();
    if (world && Rate != 0.0f) face_mut.AddControllerPitchInput(
      Rate * face.AlkLookRateDegPerSec * world->GetDeltaSeconds());
  }

  void InputMouseMovingDisable() {
    bMouseMovingEnabled = false;
    if (face.AlkTracing)
      UKismetSystemLibrary::PrintString(&face_mut,
        FString(TEXT("InputMouseMovingDisable()")));
  }

  void InputMouseMovingEnable() {
    bMouseMovingEnabled = true;
    // !!! update whenever enabled in case the viewport changed
    UpdateViewportState();
    UpdateViewportMousePositionReturnDelta();
    if (face.AlkTracing)
      UKismetSystemLibrary::PrintString(&face_mut,
        FString(TEXT("InputMouseMovingEnable()")));
  }

  void InputMouseTurningDisable() {
    bMouseTurningEnabled = false;
    if (face.AlkTracing)
      UKismetSystemLibrary::PrintString(&face_mut,
        FString(TEXT("InputMouseTurningDisable()")));
  }

  void InputMouseTurningEnable() {
    bMouseTurningEnabled = true;
    // !!! update whenever enabled in case the viewport changed
    UpdateViewportState();
    UpdateViewportMousePositionReturnDelta();
    if (face.AlkTracing)
      UKismetSystemLibrary::PrintString(&face_mut,
        FString(TEXT("InputMouseTurningEnable()")));
  }

  void InputMouseAxis(float const Value) {
    if (Value == 0.f)
      return;
    // !!! we are not using the passed in Value because it is inconsistent
    // !!! due to project settings: input axis mapping scale, FOVScaling
    auto const deltaPos = UpdateViewportMousePositionReturnDelta();
    if (face.AlkHolding)
      face_mut.AlkOnHoldMove(pure::VectorFromVector2D(deltaPos));
    if (HoldMeasuring)
      StopHoldMeasuring();
    if (bMouseMovingEnabled) {
      DragMoveByViewportDelta(deltaPos);
      if (bMouseTurningEnabled)
        DragTurnByViewportDelta(FVector2D(deltaPos.X, 0.f));
      UndoMouseDeltaPosition(deltaPos);
    } else if (bMouseTurningEnabled) {
      DragTurnByViewportDelta(deltaPos);
      UndoMouseDeltaPosition(deltaPos);
    }
    if (pure::WorldGameViewportIsMouseOverClient(face.GetWorld()))
      UpdatePointerWorldFromViewport(ViewportMousePosition);
  }

  void InputSnapMoveBackward() {
    // TODO: ### IMPLEMENT
  }

  void InputSnapMoveForward() {
    // TODO: ### IMPLEMENT
  }

  void InputSnapMoveLeft() {
    // TODO: ### IMPLEMENT
  }

  void InputSnapMoveRight() {
    // TODO: ### IMPLEMENT
  }

  void InputSnapTurnBack() {
    auto const world = face.GetWorld();
    if (world) face_mut.AddControllerYawInput(180);
  }

  void InputSnapTurnLeft() {
    auto const world = face.GetWorld();
    if (world) face_mut.AddControllerYawInput(- face.AlkTurnSnapDeg);
  }

  void InputSnapTurnRight() {
    auto const world = face.GetWorld();
    if (world) face_mut.AddControllerYawInput(face.AlkTurnSnapDeg);
  }

  void InputTouchDragged(
    ETouchIndex::Type const FingerIndex,
    FVector const Location
  ) {
    if (FingerIndex > ETouchIndex::MAX_TOUCHES)
      return; // TODO: @@@ LOG FAILURE
    FVector const locDelta = Location
      - TouchFingerStates[FingerIndex].Location;
    TouchFingerStates[FingerIndex].Location = Location;
    if (locDelta.X == 0.f && locDelta.Y == 0.f)
      return;
    if (FingerIndex == FingerIndexFire) {
      if (face.AlkHolding)
        face_mut.AlkOnHoldMove(Location);
      else if (HoldMeasuring)
        StopHoldMeasuring();
      UpdatePointerWorldFromViewport(
        pure::Vector2DFromVector(Location));
    }
    if (!TouchFingerStates[FingerIndex].bDragged)
      // !!! update whenever dragging starts in case the viewport changed
      UpdateViewportState();
    else
      TouchFingerStates[FingerIndex].bDragged = true;
    // TODO: ### GENERALIZE FINGER EXCLUSION LOGIC BECAUSE WE
    //       ### RECEIVE SEPARATE CALLS FOR ALL FINGERS PRESSED
    if (FingerIndex == FingerIndexMove) { // TODO: ### assuming Touch2
      DragMoveByViewportDelta(pure::Vector2DFromVector(locDelta));
    }
    if (FingerIndex == FingerIndexTurn // TODO: ### assuming Touch1
        && !TouchFingerStates[FingerIndexMoveOnly].bPressed) { // TODO: ### assuming Touch3
      if (TouchFingerStates[FingerIndexMove].bPressed)
        DragTurnByViewportDelta(FVector2D(locDelta.X, 0.f));
      else
        DragTurnByViewportDelta(pure::Vector2DFromVector(locDelta));
    }
  }

  void InputTouchPressed(
    ETouchIndex::Type const FingerIndex,
    FVector const Location
  ) {
    if (face.AlkTracing)
      UKismetSystemLibrary::PrintString(&face_mut, FString(TEXT("InputTouchPressed(...)")));
    if (FingerIndex > ETouchIndex::MAX_TOUCHES)
      return; // TODO: @@@ LOG FAILURE
    if (TouchFingerStates[FingerIndex].bPressed)
      return; // TODO: @@@ LOG FAILURE
    TouchFingerStates[FingerIndex].Location = Location;
    TouchFingerStates[FingerIndex].bDragged = false;
    TouchFingerStates[FingerIndex].bPressed = true;
    TouchFingerStates[FingerIndex].PressedRealTimeSeconds =
      pure::WorldRealTimeSeconds(face.GetWorld());
    if (FingerIndex == FingerIndexFire) {
      HandleFireOrHoldPressed(Location);
      UpdatePointerWorldFromViewport(
        pure::Vector2DFromVector(Location));
    }
  }

  void InputTouchReleased(
    ETouchIndex::Type const FingerIndex,
    FVector const Location
  ) {
    if (face.AlkTracing)
      UKismetSystemLibrary::PrintString(&face_mut, FString(TEXT("InputTouchReleased(...)")));
    if (FingerIndex > ETouchIndex::MAX_TOUCHES)
      return; // TODO: @@@ LOG FAILURE
    if (!TouchFingerStates[FingerIndex].bPressed)
      return; // TODO: @@@ LOG FAILURE
    TouchFingerStates[FingerIndex].Location = Location;
    TouchFingerStates[FingerIndex].bPressed = false;
    if (FingerIndex == FingerIndexFire)
      HandleFireOrHoldReleased(Location);
  }

  void DragMoveByViewportDelta(FVector2D const & deltaPos) {
    if (   (deltaPos.X != 0 || deltaPos.Y != 0)
        && (ViewportDivisor.X > 0.f)
        && (ViewportDivisor.Y > 0.f)) {
      auto const vpRatio = deltaPos / ViewportDivisor;
      auto const meters = pure::VectorFromVector2D(vpRatio) *
        face.AlkInputDragMoveMetersPerViewport;
      if (meters.X != 0.f)
        InputMoveRight(meters.X * 100.f);
      if (meters.Y != 0.f)
        InputMoveForward(meters.Y * -100.f);
      if (face.AlkTracing)
        UKismetSystemLibrary::PrintString(&face_mut,
          FString::Printf(TEXT("DragMoveByViewportDelta((%f,%f)): vpRatio (%f,%f), meters (%f,%f)"),
            deltaPos.X, deltaPos.Y, vpRatio.X, vpRatio.Y, meters.X, meters.Y));
    }
  }

  void DragTurnByViewportDelta(FVector2D const & deltaPos) {
    if (   (deltaPos.X != 0 || deltaPos.Y != 0)
        && (ViewportDivisor.X > 0.f)
        && (ViewportDivisor.Y > 0.f)) {
      auto const vpRatio = deltaPos / ViewportDivisor;
      auto const degrees = pure::VectorFromVector2D(vpRatio) *
        face.AlkInputDragTurnDegreesPerViewport;
      if (degrees.X != 0.f)
        face_mut.AddControllerYawInput(degrees.X);
          // !!! ^ UE PlayerController applies InputYawScale
      if (degrees.Y != 0.f)
        face_mut.AddControllerPitchInput(degrees.Y);
          // !!! ^ UE PlayerController applies InputPitchScale
      if (face.AlkTracing)
        UKismetSystemLibrary::PrintString(&face_mut,
          FString::Printf(TEXT("DragTurnByViewportDelta((%f,%f)): vpRatio (%f,%f), degrees (%f,%f)"),
            deltaPos.X, deltaPos.Y, vpRatio.X, vpRatio.Y, degrees.X, degrees.Y));
    }
  }

}; // end of struct AAlkCharacterImpl

static inline AAlkCharacterImpl const & downcast(
  std::unique_ptr<AAlkCharacter::Impl> const & impl
) {
  return static_cast<AAlkCharacterImpl const&>(*impl);
}

static inline AAlkCharacterImpl& downcast_mut(
  std::unique_ptr<AAlkCharacter::Impl> const & impl
) {
  return static_cast<AAlkCharacterImpl&>(*impl);
}

auto AAlkCharacter::HasAllOptions(int const Options) const -> bool {
  return (downcast(impl).Options & Options) == Options;
}

auto AAlkCharacter::HasAnyOptions(int const Options) const -> bool {
  return (downcast(impl).Options & Options) != 0;
}

void AAlkCharacter::completeConstruction(int const inOptions) {
  impl.reset(new AAlkCharacterImpl(*this));
  downcast_mut(impl).Options = inOptions;

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
    HasAnyOptions(OPTION_VR_3DOF)
    ? AlkNodeHmdOffset // !!! motion controller is relative to HMD
    : RootComponent
  );
  AlkMotionControllerR = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("AlkMotionControllerR"));
  AlkMotionControllerR->MotionSource = FXRMotionControllerBase::RightHandSourceId;
  AlkMotionControllerR->SetupAttachment(
    HasAnyOptions(OPTION_VR_3DOF)
    ? AlkNodeHmdOffset // !!! motion controller is relative to HMD
    : RootComponent
  );
#endif

  if (HasAnyOptions(OPTION_CAN_SHOOT)) {
    AlkNodeShootMotionControllerL =
      CreateDefaultSubobject<UGripMotionControllerComponent>(
        TEXT("AlkNodeShootMotionControllerL"));
    AlkNodeShootMotionControllerR =
      CreateDefaultSubobject<UGripMotionControllerComponent>(
        TEXT("AlkNodeShootMotionControllerR"));
    if (LeftMotionController)
      AlkNodeShootMotionControllerL->SetupAttachment(LeftMotionController);
    if (RightMotionController)
      AlkNodeShootMotionControllerR->SetupAttachment(RightMotionController);
    AlkShootOffset = FVector(0.0f, 0.0f, 0.0f);
  }
  // blueprintables
  AlkFireRapidLimit = 0.f;
  AlkInputDragMoveMetersPerViewport = FVector(10.f, 10.f, 10.f);
  AlkInputDragTurnDegreesPerViewport = FVector(360.f, 144.f, 0.f);
  AlkPointerWorldDirection = FVector(1.f, 0.f, 0.f);
    // !!! ^ account for the UE PlayerController values of
    // !!! InputYawScale (default 2.5) and
    // !!! InputPitchScale (default -2.5)
  AlkInputDragThresholdPixels = 4.f;
  AlkInputFireRapidThresholdSeconds = 0.2f;
  AlkInputHoldThresholdSeconds = 0.3f;
  AlkLookRateDegPerSec = 45.f;
  AlkTurnRateDegPerSec = 45.f;
  AlkTurnSnapDeg = 5.f;
}

void AAlkCharacter::SetupPlayerInputComponent(
  class UInputComponent* PlayerInputComponent
) {
  if (!PlayerInputComponent)
    return; // TODO: @@@ LOG FAILURE
  // TODO: @@@ REFACTOR THESE BINDINGS TO DELEGATE THROUGH UOBJECT DELEGATE
  PlayerInputComponent->BindAction("AlkFireOrHold", IE_Pressed, this, &AAlkCharacter::InputFireOrHoldPressed);
  PlayerInputComponent->BindAction("AlkFireOrHold", IE_Released, this, &AAlkCharacter::InputFireOrHoldReleased);
  PlayerInputComponent->BindAction("AlkRecenterXR", IE_Pressed, this, &AAlkCharacter::InputRecenterXR);
  if (!HasAnyOptions(OPTION_NO_JUMP)) {
    PlayerInputComponent->BindAction("AlkJump", IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction("AlkJump", IE_Released, this, &ACharacter::StopJumping);
  }
  if (!HasAnyOptions(OPTION_NO_MOVE)) {
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

  PlayerInputComponent->BindAction("AlkMouseMoving", IE_Pressed, this, &AAlkCharacter::InputMouseMovingEnable);
  PlayerInputComponent->BindAction("AlkMouseMoving", IE_Released, this, &AAlkCharacter::InputMouseMovingDisable);
  PlayerInputComponent->BindAction("AlkMouseTurning", IE_Pressed, this, &AAlkCharacter::InputMouseTurningEnable);
  PlayerInputComponent->BindAction("AlkMouseTurning", IE_Released, this, &AAlkCharacter::InputMouseTurningDisable);
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
  downcast_mut(impl).UpdateHMDState(DeltaSeconds);
  downcast_mut(impl).UpdateInputState(DeltaSeconds);
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
  if (!HasAnyOptions(OPTION_CAN_SHOOT))
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
  FVector const & ScreenCoordinates,
  int RapidCount
) {
  if (AlkTracing)
    UKismetSystemLibrary::PrintString(this, FString(TEXT("AlkOnFire_Implementation(...)")));
  if (HasAnyOptions(OPTION_CAN_SHOOT))
    AlkOnShoot(ScreenCoordinates);
}

// TODO: @@@ REFACTOR THESE BINDINGS TO DELEGATE THROUGH UOBJECT DELEGATE
void AAlkCharacter::InputFireOrHoldPressed() {
  downcast_mut(impl).InputFireOrHoldPressed();
}

void AAlkCharacter::InputFireOrHoldReleased() {
  downcast_mut(impl).InputFireOrHoldReleased();
}

void AAlkCharacter::InputRecenterXR() {
  downcast_mut(impl).InputRecenterXR();
}

void AAlkCharacter::InputMoveForward(float const Value) {
  downcast_mut(impl).InputMoveForward(Value);
}

void AAlkCharacter::InputMoveRight(float const Value) {
  downcast_mut(impl).InputMoveRight(Value);
}

void AAlkCharacter::InputTurnRate(float const Rate) {
  downcast_mut(impl).InputTurnRate(Rate);
}

void AAlkCharacter::InputLookRate(float const Rate) {
  downcast_mut(impl).InputLookRate(Rate);
}

void AAlkCharacter::InputMouseMovingDisable() {
  downcast_mut(impl).InputMouseMovingDisable();
}

void AAlkCharacter::InputMouseMovingEnable() {
  downcast_mut(impl).InputMouseMovingEnable();
}

void AAlkCharacter::InputMouseTurningDisable() {
  downcast_mut(impl).InputMouseTurningDisable();
}

void AAlkCharacter::InputMouseTurningEnable() {
  downcast_mut(impl).InputMouseTurningEnable();
}

void AAlkCharacter::InputMouseAxis(float const Value) {
  downcast_mut(impl).InputMouseAxis(Value);
}

void AAlkCharacter::InputSnapMoveBackward() {
  downcast_mut(impl).InputSnapMoveBackward();
}

void AAlkCharacter::InputSnapMoveForward() {
  downcast_mut(impl).InputSnapMoveForward();
}

void AAlkCharacter::InputSnapMoveLeft() {
  downcast_mut(impl).InputSnapMoveLeft();
}

void AAlkCharacter::InputSnapMoveRight() {
  downcast_mut(impl).InputSnapMoveRight();
}

void AAlkCharacter::InputSnapTurnBack() {
  downcast_mut(impl).InputSnapTurnBack();
}

void AAlkCharacter::InputSnapTurnLeft() {
  downcast_mut(impl).InputSnapTurnLeft();
}

void AAlkCharacter::InputSnapTurnRight() {
  downcast_mut(impl).InputSnapTurnRight();
}

void AAlkCharacter::InputTouchDragged(
  ETouchIndex::Type const FingerIndex,
  FVector const Location
) {
  downcast_mut(impl).InputTouchDragged(FingerIndex, Location);
}

void AAlkCharacter::InputTouchPressed(
  ETouchIndex::Type const FingerIndex,
  FVector const Location
) {
  downcast_mut(impl).InputTouchPressed(FingerIndex, Location);
}

void AAlkCharacter::InputTouchReleased(
  ETouchIndex::Type const FingerIndex,
  FVector const Location
) {
  downcast_mut(impl).InputTouchReleased(FingerIndex, Location);
}

AAlkCharacter::Impl::~Impl() {}
