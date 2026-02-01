#include "Entity.h"
#include "../Scripting/ScriptEngine.h"

namespace Forge {

Entity::Entity(uint32_t id, const std::string &name) : m_ID(id), m_Name(name) {}

void Entity::SetParent(Entity *parent) {
  if (m_Parent == parent)
    return;

  if (m_Parent) {
    m_Parent->RemoveChild(this);
  }

  m_Parent = parent;
  if (m_Parent) {
    m_Parent->AddChild(this);
  }
}

void Entity::AddChild(Entity *child) {
  if (!child)
    return;
  m_Children.push_back(child);
}

void Entity::RemoveChild(Entity *child) {
  m_Children.erase(std::remove(m_Children.begin(), m_Children.end(), child),
                   m_Children.end());
}

void Entity::OnUpdate(float deltaTime) {
  if (m_Script.has_value()) {
    ScriptEngine::OnUpdateEntity(this, deltaTime);
  }
}

} // namespace Forge
