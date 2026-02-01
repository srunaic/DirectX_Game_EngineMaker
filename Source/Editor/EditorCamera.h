#pragma once
#include <DirectXMath.h>

namespace Forge {

class EditorCamera {
public:
  EditorCamera();

  void Zoom(float delta);

  DirectX::XMMATRIX GetViewMatrix() const;
  DirectX::XMMATRIX GetProjectionMatrix(float aspectRatio) const;

  float GetDistance() const { return m_Distance; }
  void SetDistance(float distance) { m_Distance = distance; }

  DirectX::XMFLOAT3 GetPosition() const { return m_Position; }
  DirectX::XMFLOAT3 GetTarget() const { return m_Target; }

private:
  void UpdateCameraPosition();

  DirectX::XMFLOAT3 m_Position = {0.0f, 3.0f, -5.0f};
  DirectX::XMFLOAT3 m_Target = {0.0f, 2.7f, -4.0f}; // (0,3,-5) + (0, -0.3, 1)
  DirectX::XMFLOAT3 m_Up = {0.0f, 1.0f, 0.0f};

  float m_Distance = 1.04f;
  float m_MinDistance = 0.1f;
  float m_MaxDistance = 100.0f;
  float m_FOV = 60.0f;
  float m_NearPlane = 0.1f;
  float m_FarPlane = 1000.0f;
};

} // namespace Forge
