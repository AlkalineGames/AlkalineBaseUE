// Copyright 2021 - 2025 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"

namespace pure {

inline auto WorldRealTimeSeconds(
  UWorld const *const world
) -> float {
  return world ? world->GetRealTimeSeconds() : 0.f;
}

inline auto WorldGameViewportIsMouseOverClient(
  UWorld const *const world
) -> bool {
  if (world) {
    auto viewport = world->GetGameViewport();
#ifdef ALK_UE_ENHANCED
    if (viewport) return viewport->IsMouseOverClient();
#endif
  }
  return false;
}

inline auto WorldGameViewportMousePosition(
  UWorld const *const world
) -> FVector2D {
  FVector2D result;
  if (world) {
    auto viewport = world->GetGameViewport();
    if (viewport) viewport->GetMousePosition(result);
  }
  return result;
}

inline auto WorldGameViewportSize(
  UWorld const *const world
) -> FVector2D {
  FVector2D result;
  if (world) {
    auto viewport = world->GetGameViewport();
    if (viewport) viewport->GetViewportSize(result);
  }
  return result;
}

inline auto WorldObjectLineTraceSingle(
  const UObject*  WorldContextObject,
  const FVector   Start,
  const FVector   End,
  FHitResult&     OutHit,
  bool            bIgnoreSelf
)-> bool {
  //static bool LineTraceSingle(const UObject* WorldContextObject, const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);
  return UKismetSystemLibrary::LineTraceSingle(
    WorldContextObject, Start, End,
    ETraceTypeQuery::TraceTypeQuery1, // in EngineTypes.h, Visibility?
    false,              // bTraceComplex
    TArray<AActor*>(),  // ActorsToIgnore
    EDrawDebugTrace::None,
    OutHit, bIgnoreSelf);
}

}; // end namespace pure
