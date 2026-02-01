#include "EditorCamera.h"
#include <algorithm>

namespace Forge {

using namespace DirectX;

EditorCamera::EditorCamera() { UpdateCameraPosition(); }

void EditorCamera::Zoom(float delta) {
  // Delta is usually 1.0 or -1.0 from mouse wheel
  float zoomSpeed = m_Distance * 0.1f; // Exponential zoom feel
  m_Distance -= delta * zoomSpeed;
  m_Distance = std::clamp(m_Distance, m_MinDistance, m_MaxDistance);

  UpdateCameraPosition();
}

void EditorCamera::UpdateCameraPosition() {
  // For now, a simple fixed orbit camera looking at target
  // We'll calculate position based on target and distance
  // Direction from target to camera (assuming fixed angle for now)
  XMVECTOR targetVec = XMLoadFloat3(&m_Target);
  XMVECTOR forward = XMVectorSet(0.0f, 0.5f, -1.0f, 0.0f); // Fixed angle look
  forward = XMVector3Normalize(forward);

  XMVECTOR posVec = targetVec - (forward * m_Distance);
  XMStoreFloat3(&m_Position, posVec);
}

XMMATRIX EditorCamera::GetViewMatrix() const {
  return XMMatrixLookAtLH(XMLoadFloat3(&m_Position), XMLoadFloat3(&m_Target),
                          XMLoadFloat3(&m_Up));
}

XMMATRIX EditorCamera::GetProjectionMatrix(float aspectRatio) const {
  return XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FOV), aspectRatio,
                                  m_NearPlane, m_FarPlane);
}

} // namespace Forge
