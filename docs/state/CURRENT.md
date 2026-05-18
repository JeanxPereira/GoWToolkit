# Estado Atual

**Última atualização**: 2026-05-18 por Claude (sessão 3)
**Sessão #**: 3

## Milestone ativa
M1 — Structural Cleanup

## Task em progresso
nenhuma — M1.T2 fechada, próxima é M1.T3

## Próxima task no pipeline
M1.T3 — Decouple `MeshData` de Rendering. Depende de M1.T2 ✓.

## Blockers
nenhum

## Notas para o próximo agente
- M1.T2 entregue: `BoundingBox` movido para `src/core/domain/BoundingBox.h`. `Camera.h` e `GpuMesh.h` apontam para o novo header. `MeshData.h` ainda inclui `rendering/GpuMesh.h` — isso sai em M1.T3 (decouple GpuVertex também).
- Layer linter: 12 violações estáveis (mesmas de M1.T1). `MeshData.h → GpuMesh.h` continua até M1.T3.
- ctest 6/6 verde.
- **Hotfixes desta sessão** (antes de M1.T2):
  - Crash `ImFontFlags_ImplicitRefSize` em `SettingsWindow::RebuildFontAtlas`: agora passa `ImFontConfig{SizePixels=m_fontSize}` em ambos paths (AddFontDefault + AddFontFromFileTTF) — destination font explícito permite merge com icon font explícito (TitleBar::loadIconFont).
  - Icone macOS quebrado: `actool` gated em Release-only no CMakeLists. Agora detecta actool via `xcrun --find` com fallback direto pra `/Applications/Xcode.app/Contents/Developer/usr/bin/actool` (necessário quando `xcode-select -p` aponta CLT). Roda em todos build types.

## Arquivos tocados nesta sessão
- Hotfix font:
  - `src/ui/TitleBar.cpp` (já estava modificado de sessão anterior — incluído no commit)
  - `src/ui/SettingsWindow.cpp` (RebuildFontAtlas explicit SizePixels)
- Hotfix actool:
  - `CMakeLists.txt` (detect actool via xcrun + Xcode.app fallback; roda em Debug também)
- M1.T2:
  - `src/core/domain/BoundingBox.h` (NEW)
  - `src/rendering/Camera.h` (remove struct, include domain header)
  - `src/rendering/GpuMesh.h` (substitui `#include "Camera.h"` por include do domain header)
