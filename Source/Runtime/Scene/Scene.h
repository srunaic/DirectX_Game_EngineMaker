#pragma once
#include "Entity.h"
#include <memory>
#include <vector>


namespace Forge {

class Scene {
public:
  Scene();
  ~Scene();

  Entity *CreateEntity(const std::string &name = "NewGameObject");
  void DestroyEntity(Entity *entity);

  const std::vector<std::unique_ptr<Entity>> &GetEntities() const {
    return m_Entities;
  }
  const std::vector<Entity *> &GetRootEntities() const {
    return m_RootEntities;
  }

private:
  std::vector<std::unique_ptr<Entity>> m_Entities;
  std::vector<Entity *> m_RootEntities; // Entities with no parent
  uint32_t m_NextID = 0;
};

} // namespace Forge
