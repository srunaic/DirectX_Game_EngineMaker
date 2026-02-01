#pragma once
#include "../Scene/Entity.h"
#include <iostream>
#include <mono/metadata/loader.h>
#include <mono/metadata/object.h>


namespace Forge {

class ScriptGlue {
public:
  static void RegisterInternalCalls() {
    // Entity
    mono_add_internal_call("Forge.InternalCalls::Entity_GetName",
                           (void *)Entity_GetName);
    mono_add_internal_call("Forge.InternalCalls::Entity_SetName",
                           (void *)Entity_SetName);

    // Transform
    mono_add_internal_call("Forge.InternalCalls::Transform_GetPosition",
                           (void *)Transform_GetPosition);
    mono_add_internal_call("Forge.InternalCalls::Transform_SetPosition",
                           (void *)Transform_SetPosition);
    mono_add_internal_call("Forge.InternalCalls::Transform_GetRotation",
                           (void *)Transform_GetRotation);
    mono_add_internal_call("Forge.InternalCalls::Transform_SetRotation",
                           (void *)Transform_SetRotation);
    mono_add_internal_call("Forge.InternalCalls::Transform_GetScale",
                           (void *)Transform_GetScale);
    mono_add_internal_call("Forge.InternalCalls::Transform_SetScale",
                           (void *)Transform_SetScale);
  }

private:
  // Helper to get Entity from ID (This needs access to the active Scene)
  static Entity *GetEntity(uint32_t id);

  static MonoString *Entity_GetName(uint32_t id) {
    // Entity* entity = GetEntity(id);
    // return mono_string_new(mono_domain_get(), entity->GetName().c_str());
    return mono_string_new(mono_domain_get(), "MockEntityName");
  }

  static void Entity_SetName(uint32_t id, MonoString *name) {
    char *nameStr = mono_string_to_utf8(name);
    // GetEntity(id)->SetName(nameStr);
    mono_free(nameStr);
  }

  static void Transform_GetPosition(uint32_t id, XMFLOAT3 *outPos) {
    // *outPos = GetEntity(id)->GetTransform().Position;
    *outPos = {0, 0, 0};
  }

  static void Transform_SetPosition(uint32_t id, XMFLOAT3 *inPos) {
    // GetEntity(id)->GetTransform().Position = *inPos;
  }

  static void Transform_GetRotation(uint32_t id, XMFLOAT3 *outRot) {
    *outRot = {0, 0, 0};
  }

  static void Transform_SetRotation(uint32_t id, XMFLOAT3 *inRot) {}

  static void Transform_GetScale(uint32_t id, XMFLOAT3 *outScale) {
    *outScale = {1, 1, 1};
  }

  static void Transform_SetScale(uint32_t id, XMFLOAT3 *inScale) {}
};

} // namespace Forge
