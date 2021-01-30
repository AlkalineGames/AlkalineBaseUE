// Copyright 2015-2021 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "AlkCharacter.generated.h"

UCLASS()
class ALKUEMCHAR_API AAlkCharacter : public AVRCharacter
{
  GENERATED_BODY()

#if 0 // TODO: @@@ Pimpl idiom requires subclasses match constructors
  struct ImplData;
  TUniquePtr<struct ImplData> implData;

public:
  /** required for TUniquePtr<> **/
  AAlkCharacter(FObjectInitializer const &);
  AAlkCharacter(FVTableHelper&);
  ~AAlkCharacter();
#endif

public:
  static constexpr int OPTION_CAN_SHOOT  = 1 << 0;
  static constexpr int OPTION_NO_JUMP    = 1 << 1;
  static constexpr int OPTION_NO_MOVE    = 1 << 2;
  static constexpr int OPTION_VR_3DOF    = 1 << 3;

  auto hasAllOptions(int const inOptions) const -> bool { return (Options & inOptions) == inOptions; }
  auto hasAnyOptions(int const inOptions) const -> bool { return (Options & inOptions) != 0; }

protected:
  void completeConstruction(int const inOptions = 0);

public:
  virtual void SetupPlayerInputComponent(
    class UInputComponent*) override; // APawn::

  virtual void Tick(float const DeltaSeconds) override; // AActor::

public: // blueprintables
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    FVector2D AlkInputDragDegPerViewport;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    float AlkInputDragThresholdPixels;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    float AlkInputTapThresholdSeconds;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=AlkCharacter)
    float AlkLookRateDegPerSec;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=AlkCharacter)
    float AlkTurnRateDegPerSec;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    bool AlkTracing;

#if 0 // TODO: @@@ DEPRECATED, NOW PROVIDED BY AVRCharacter
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AlkCharacter, meta = (AllowPrivateAccess = "true"))
    class UMotionControllerComponent* AlkMotionControllerL;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AlkCharacter, meta = (AllowPrivateAccess = "true"))
    class UMotionControllerComponent* AlkMotionControllerR;
  // TODO: ### DO WE STILL NEED THIS WITH AVRCharacter SUPERCLASS
  // !!! insert HMD offset to adjust its incorrect location within the capsule [c4augustus]
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AlkCharacter, meta = (AllowPrivateAccess = "true"))
    class USceneComponent* AlkNodeHmdOffset;
#endif
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    bool bAlkUsingMotionControllers;

  // @@@ shoot specifics (optional)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    TSubclassOf<class AActor> AlkProjectileClass;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    class USoundBase* AlkShootSound;
  UPROPERTY(VisibleDefaultsOnly, Category = AlkCharacter)
    class USceneComponent* AlkNodeShootDefault;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AlkCharacter, meta = (AllowPrivateAccess = "true"))
    class USceneComponent* AlkNodeShootMotionControllerL;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AlkCharacter, meta = (AllowPrivateAccess = "true"))
    class USceneComponent* AlkNodeShootMotionControllerR;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    FVector AlkShootOffset;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    bool bAlkShootFromMotionControllerLeftNotRight;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AlkCharacter)
    void AlkOnFire(FVector const & ScreenCoordinates);
    virtual void AlkOnFire_Implementation(FVector const & ScreenCoordinates);
      // ^ calls AlkOnShoot() if has OPTION_CAN_SHOOT

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AlkCharacter)
    void AlkOnShoot(FVector const & ScreenCoordinates);
    virtual void AlkOnShoot_Implementation(FVector const & ScreenCoordinates);
      // ^ spawns projectile

private: // !!! TODO: @@@ everything below should be in ImplData
  int Options = 0;
  bool bRotateDragEnabled = false;
  struct HMDState {
    FRotator Orientation;
    FVector Position;
    float UpdateDeltaSeconds = 0.f;
    float UpdateTotalSeconds = 0.f;
    bool Worn = false;
  };
  struct HMDState HMDState;
  ETouchIndex::Type FingerIndexFire = ETouchIndex::Touch1;
  ETouchIndex::Type FingerIndexRotate = ETouchIndex::Touch1;
  struct TouchFingerState {
    ETouchIndex::Type FingerIndex = ETouchIndex::CursorPointerIndex;
    FVector Location = FVector::ZeroVector;
    bool bDragged = false;
    bool bPressed = false;
    float PressedRealTimeSeconds = 0.f;
  };
  struct TouchFingerState TouchFingerStates[ETouchIndex::MAX_TOUCHES];
  FVector2D ViewportSize;
  FVector2D ViewportDragThresholdRatio;
  FVector2D ViewportMousePosition;

  void ApplyHMDState();
  void UpdateHMDState(float const DeltaSeconds);

  void UpdateViewportState();
  auto UpdateViewportMousePositionReturnDelta() -> FVector2D;

  void InputFire();
  void InputRecenterXR();

  void InputMoveForward(float const);
  void InputMoveRight(float const);
  void InputTurnRate(float const);
  void InputLookRate(float const);

  void InputRotateDragDisable();
  void InputRotateDragEnable();
  void InputMouseAxis(float const);

  void InputSnapMoveBackward();
  void InputSnapMoveForward();
  void InputSnapMoveLeft();
  void InputSnapMoveRight();
  void InputSnapTurnBack();
  void InputSnapTurnLeft();
  void InputSnapTurnRight();

  void InputTouchDragged(ETouchIndex::Type const FingerIndex, FVector const Location);
  void InputTouchPressed(ETouchIndex::Type const FingerIndex, FVector const Location);
  void InputTouchReleased(ETouchIndex::Type const FingerIndex, FVector const Location);
  void InputTouchTapped(ETouchIndex::Type const FingerIndex, FVector const Location);

  void RotateDrag(FVector2D const &);
};
