#include "Scene.h"
#include <algorithm>

namespace Forge {

Scene::Scene() = default;
Scene::~Scene() = default;

Entity *Scene::CreateEntity(const std::string &name) {
  auto entity = std::make_unique<Entity>(m_NextID++, name);
  Entity *ptr = entity.get();
  m_Entities.push_back(std::move(entity));
  m_RootEntities.push_back(ptr);
  return ptr;
}

void Scene::DestroyEntity(Entity *entity) {
  if (!entity)
    return;

  // Handle children: set their parent to null (or we could destroy them too)
  // For simplicity, let's just detach them to root
  auto children = entity->GetChildren();
  for (auto *child : children) {
    child->SetParent(nullptr);
  }

  if (entity->GetParent()) {
    entity->GetParent()->RemoveChild(entity);
  } else {
    auto it = std::find(m_RootEntities.begin(), m_RootEntities.end(), entity);
    if (it != m_RootEntities.end()) {
      m_RootEntities.erase(it);
    }
  }

  auto it = std::find_if(
      m_Entities.begin(), m_Entities.end(),
      [entity](const std::unique_ptr<Entity> &e) { return e.get() == entity; });

  if (it != m_Entities.end()) {
    m_Entities.erase(it);
  }
}

} // namespace Forge
