#pragma once
#include "../Scripting/ScriptEngine.h"
#include "TransformComponent.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Forge {

class Entity {
public:
  Entity(uint32_t id, const std::string &name);
  ~Entity() = default;

  uint32_t GetID() const { return m_ID; }
  const std::string &GetName() const { return m_Name; }
  void SetName(const std::string &name) { m_Name = name; }

  TransformComponent &GetTransform() { return m_Transform; }

  // Hierarchy
  Entity *GetParent() const { return m_Parent; }
  void SetParent(Entity *parent);
  const std::vector<Entity *> &GetChildren() const { return m_Children; }
  void AddChild(Entity *child);
  void RemoveChild(Entity *child);

  // Scripting
  void AddScript(const std::string &className) { m_Script = {className}; }
  ScriptComponent *GetScript() {
    return m_Script.has_value() ? &m_Script.value() : nullptr;
  }

  void OnUpdate(float deltaTime);

private:
  uint32_t m_ID;
  std::string m_Name;
  TransformComponent m_Transform;

  Entity *m_Parent = nullptr;
  std::vector<Entity *> m_Children;

  std::optional<ScriptComponent> m_Script;
};

} // namespace Forge
