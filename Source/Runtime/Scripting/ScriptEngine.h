#pragma once
#include <string>
#include <unordered_map>
#include <vector>


extern "C" {
typedef struct _MonoDomain MonoDomain;
typedef struct _MonoAssembly MonoAssembly;
typedef struct _MonoImage MonoImage;
typedef struct _MonoClass MonoClass;
typedef struct _MonoObject MonoObject;
typedef struct _MonoMethod MonoMethod;
}

namespace Forge {

class Entity;

struct ScriptComponent {
  std::string ClassName;
  MonoObject *Instance = nullptr;
  bool Initialized = false;
};

class ScriptEngine {
public:
  static void Init();
  static void Shutdown();

  static void LoadAssembly(const std::string &path);

  static void InstantiateEntity(Entity *entity);
  static void OnUpdateEntity(Entity *entity, float deltaTime);

  static MonoDomain *GetRootDomain();
  static MonoImage *GetCoreAssemblyImage();

private:
  static MonoObject *InstantiateClass(MonoClass *monoClass);
  static MonoClass *GetClassInImage(MonoImage *image,
                                    const std::string &namespaceName,
                                    const std::string &className);
};

} // namespace Forge
