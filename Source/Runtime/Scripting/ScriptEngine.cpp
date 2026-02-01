#include "ScriptEngine.h"
#include "../Scene/Entity.h"
#include <iostream>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>


namespace Forge {

struct ScriptEngineData {
  MonoDomain *RootDomain = nullptr;
  MonoDomain *AppDomain = nullptr;
  MonoAssembly *CoreAssembly = nullptr;
  MonoImage *CoreAssemblyImage = nullptr;

  MonoMethod *UpdateMethod = nullptr;
  MonoMethod *StartMethod = nullptr;

  std::unordered_map<uint32_t, ScriptComponent *> EntityScripts;
};

static ScriptEngineData *s_Data = nullptr;

void ScriptEngine::Init() {
  s_Data = new ScriptEngineData();

  mono_set_assemblies_path("mono/lib");
  s_Data->RootDomain = mono_jit_init("ForgeJIT");

  // In a real engine, we'd create a separate app domain for reloading
  s_Data->AppDomain =
      mono_domain_create_appdomain((char *)"ForgeAppDomain", nullptr);
  mono_domain_set(s_Data->AppDomain, true);

  std::cout << "[ScriptEngine] Initialized Mono JIT" << std::endl;
}

void ScriptEngine::Shutdown() {
  mono_jit_cleanup(s_Data->RootDomain);
  delete s_Data;
  s_Data = nullptr;
}

void ScriptEngine::LoadAssembly(const std::string &path) {
  s_Data->CoreAssembly =
      mono_domain_assembly_open(s_Data->AppDomain, path.c_str());
  s_Data->CoreAssemblyImage = mono_assembly_get_image(s_Data->CoreAssembly);
  std::cout << "[ScriptEngine] Loaded assembly: " << path << std::endl;
}

MonoDomain *ScriptEngine::GetRootDomain() { return s_Data->RootDomain; }
MonoImage *ScriptEngine::GetCoreAssemblyImage() {
  return s_Data->CoreAssemblyImage;
}

MonoClass *ScriptEngine::GetClassInImage(MonoImage *image,
                                         const std::string &namespaceName,
                                         const std::string &className) {
  return mono_class_from_name(image, namespaceName.c_str(), className.c_str());
}

MonoObject *ScriptEngine::InstantiateClass(MonoClass *monoClass) {
  MonoObject *instance = mono_object_new(s_Data->AppDomain, monoClass);
  mono_runtime_object_init(instance);
  return instance;
}

void ScriptEngine::InstantiateEntity(Entity *entity) {
  // Assume for simplicity that we check if entity has a script component
  // Typically this would be integrated into Entity/Component system
}

void ScriptEngine::OnUpdateEntity(Entity *entity, float deltaTime) {
  uint32_t id = entity->GetID();
  if (s_Data->EntityScripts.find(id) != s_Data->EntityScripts.end()) {
    ScriptComponent *script = s_Data->EntityScripts[id];

    // Call OnUpdate(float dt)
    // MonoMethod* method = ... find method in class ...
    // void* params[] = { &deltaTime };
    // mono_runtime_invoke(method, script->Instance, params, nullptr);
  }
}

} // namespace Forge
