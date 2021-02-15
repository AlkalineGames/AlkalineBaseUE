// Copyright 2015-2021 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#pragma once

#include <memory>

#include "CoreMinimal.h"
#include "VRCharacter.h"

#include "AlkCharacter.generated.h"

UCLASS()
class ALKUEMCHAR_API AAlkCharacter : public AVRCharacter
{
  GENERATED_BODY()

public:
  static constexpr int OPTION_CAN_SHOOT  = 1 << 0;
  static constexpr int OPTION_NO_JUMP    = 1 << 1;
  static constexpr int OPTION_NO_MOVE    = 1 << 2;
  static constexpr int OPTION_VR_3DOF    = 1 << 3;

  auto HasAllOptions(int const inOptions) const -> bool;
  auto HasAnyOptions(int const inOptions) const -> bool;

protected: // TODO: @@@ ABSTRACT CLASS, SUPERCLASSES MUSt CALL:
  void completeConstruction(int const inOptions = 0); // TODO: @@@ CAPITALIZE

public:
  virtual void SetupPlayerInputComponent(
    class UInputComponent*) override; // APawn::

  virtual void Tick(float const DeltaSeconds) override; // AActor::

public: // blueprintables
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    FVector AlkInputDragMoveMetersPerViewport;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    FVector AlkInputDragTurnDegreesPerViewport;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    int AlkFireRapidLimit;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    float AlkInputDragThresholdPixels;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    float AlkInputFireRapidThresholdSeconds;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    float AlkInputHoldThresholdSeconds;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=AlkCharacter)
    float AlkLookRateDegPerSec;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=AlkCharacter)
    float AlkTurnRateDegPerSec;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    bool AlkHoldEnabled;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    bool AlkHolding;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlkCharacter)
    bool AlkTracing;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AlkCharacter)
    void AlkOnHoldEnter(FVector const & ScreenCoordinates);
    virtual void AlkOnHoldEnter_Implementation(FVector const & ScreenCoordinates);
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AlkCharacter)
    void AlkOnHoldLeave(FVector const & ScreenCoordinates);
    virtual void AlkOnHoldLeave_Implementation(FVector const & ScreenCoordinates);
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AlkCharacter)
    void AlkOnHoldMove(FVector const & ScreenCoordinates);
    virtual void AlkOnHoldMove_Implementation(FVector const & ScreenCoordinates);

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
    void AlkOnFire(FVector const & ScreenCoordinates,
      int RapidCount); // positive value when pressed, negative when released
  virtual void AlkOnFire_Implementation(
    FVector const & ScreenCoordinates, int RapidCount);
      // ^ calls AlkOnShoot() if has OPTION_CAN_SHOOT

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AlkCharacter)
    void AlkOnShoot(FVector const & ScreenCoordinates);
  virtual void AlkOnShoot_Implementation(
    FVector const & ScreenCoordinates);
      // ^ spawns projectile

  struct Impl { virtual ~Impl() = 0; };
private:
  std::unique_ptr<struct Impl> impl;

// TODO: @@@ REFACTOR THESE BINDINGS METHODS TO BE HIDDEN IN THE IMPL
  void InputFireOrHoldPressed();
  void InputFireOrHoldReleased();
  void InputRecenterXR();

  void InputMoveForward(float const);
  void InputMoveRight(float const);
  void InputTurnRate(float const);
  void InputLookRate(float const);

  void InputMouseMovingDisable();
  void InputMouseMovingEnable();
  void InputMouseTurningDisable();
  void InputMouseTurningEnable();
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
};
