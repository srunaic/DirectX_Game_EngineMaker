#pragma once
#include <DirectXMath.h>

namespace Forge {

using namespace DirectX;

struct TransformComponent {
  XMFLOAT3 Position = {0.0f, 0.0f, 0.0f};
  XMFLOAT3 Rotation = {0.0f, 0.0f, 0.0f};
  XMFLOAT3 Scale = {1.0f, 1.0f, 1.0f};

  XMMATRIX GetTransform() const {
    return XMMatrixScaling(Scale.x, Scale.y, Scale.z) *
           XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z) *
           XMMatrixTranslation(Position.x, Position.y, Position.z);
  }
};

} // namespace Forge
