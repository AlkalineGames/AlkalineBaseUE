// Copyright 2021 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

namespace pure {

inline auto WorldRealTimeSeconds(
  UWorld const *const world
) -> float {
  return world ? world->GetRealTimeSeconds() : 0.f;
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

}; // end namespace pure
