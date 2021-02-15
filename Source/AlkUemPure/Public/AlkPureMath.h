// Copyright 2021 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

namespace pure {

inline auto VectorFromVector2D(
  FVector2D const & vec2D,
  float z = 0.f
) -> FVector {
  return FVector(vec2D.X, vec2D.Y, z);
}

inline auto Vector2DFromVector(
  FVector const & vec
) -> FVector2D {
  return FVector2D(vec.X, vec.Y);
}

}; // end namespace pure
