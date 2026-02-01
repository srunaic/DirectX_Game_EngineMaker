---
description: How to rebuild the DirectXForge Editor after code changes
---

# DirectXForge Engine - Build Workflow

**중요: 기존 빌드 파일을 덮어쓰지 않습니다. 새로운 빌드 폴더에 추가합니다.**

## Build Steps

1. Configure CMake with timestamped build directory:
```powershell
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$buildDir = "Build_$timestamp"
cmake -B $buildDir
```

2. Build the project:
```powershell
cmake --build $buildDir --config Release
```

3. The new executable will be at:
```
Build_[timestamp]\Release\DirectXForgeEditor.exe
```

## Notes

- 기존 빌드 폴더(`Build`, `Build_*`)는 사용자가 수동으로 삭제
- 각 빌드는 별도의 타임스탬프 폴더에 생성됨
- 충돌 없이 여러 버전 동시 보유 가능
