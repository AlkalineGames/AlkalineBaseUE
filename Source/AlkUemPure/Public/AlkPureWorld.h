// Copyright 2021 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

namespace pure {

inline const float WorldRealTimeSeconds(const UWorld* world) {
  return world ? world->GetRealTimeSeconds() : 0.f;
}

inline const FVector2D WorldGameViewportMousePosition(const UWorld* world) {
  FVector2D result;
  if (world) {
    auto viewport = world->GetGameViewport();
    if (viewport) viewport->GetMousePosition(result);
  }
  return result;
}

inline const FVector2D WorldGameViewportSize(const UWorld* world) {
  FVector2D result;
  if (world) {
    auto viewport = world->GetGameViewport();
    if (viewport) viewport->GetViewportSize(result);
  }
  return result;
}

}; // end namespace pure
