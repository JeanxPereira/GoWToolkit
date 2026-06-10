# Extração do Onyx SDK — desacoplamento engine/app do GoWToolkit

**Data:** 2026-06-10
**Status:** Aprovado (design); pronto para plano de implementação
**Nome do SDK:** `Onyx` — **placeholder trocável** (ver §0). Namespace e target derivam dele.

---

## Contexto e objetivo

O GoWToolkit tem uma arquitetura em camadas já bem separada: o `core/` é
praticamente livre de UI, com interfaces limpas (`IGameProfile`,
`IVirtualFileSystem`/`IFile`, schema `StructDef`+`NodeInstance`,
`TypeRegistry`+`ITypeHandler`) e registries plugáveis na UI (`IPanel`+
`PanelRegistry`, `IDocumentContent`+`ViewerRegistry`). O alvo
`gowtoolkit_parser_min` já prova que o parser separa da UI.

O objetivo é **extrair essa base genérica num SDK reutilizável** que sirva de
fundação para outros exploradores de assets de jogo (o próximo é o
**SCUMMRedux**, para arquivos de jogos SCUMM). A meta é uma base "tipo Noesis,
mas como framework para *construir* ferramentas" — não uma ferramenta pronta e
genérica, e sim um SDK sobre o qual cada jogo é um app fino.

### Decisões fundadoras (definidas no brainstorming)

1. **Modelo de distribuição:** híbrido tendendo a **SDK** — uma biblioteca
   compartilhada (`engine/`) que os apps consomem, mais um template/exemplo fino
   por cima.
2. **Estratégia de prova:** **extrair de dentro do GoWToolkit** (dogfooding). O
   GoWToolkit vira o primeiro consumidor e a prova viva do SDK; os golden tests
   existentes são a rede de segurança.
3. **Sistema de tipos:** **registro aberto** — o engine conhece só `MediaKind`
   (categoria que decide o viewer); cada app registra seus próprios tipos no
   `TypeRegistry`.
4. **Sequência física:** **Approach C** — layering in-place primeiro (provar o
   boundary no repo atual), split físico do repo depois.

---

## §0 — Nome (placeholder trocável)

`Onyx` é provisório. O custo de troca é mínimo por construção:

- Nome do target CMake numa **variável única**: `set(SDK_NAME Onyx)`.
- Namespace é um **token único** (`Onyx::`) — troca = find-replace global que o
  compilador verifica.
- **Nenhuma semântica amarrada ao nome.** Pastas são genéricas (`engine/`,
  `apps/`), não `onyx/`. O nome só aparece em namespace e rótulos de build.

Antes de travar o nome definitivo: checar disponibilidade de domínio/crate/npm.

**Convenção de estilo:** PascalCase em diretórios **e** namespace
(`Onyx::Vfs::IVirtualFileSystem`, `Onyx::App::IPanel`). Arquivos já são
PascalCase.

---

## §1 — Boundary: `engine/` (SDK) vs `app/` (jogo)

Regra de corte: **se não menciona um conceito de jogo específico, é engine.**

### `engine/` — o SDK genérico

| Camada | Conteúdo | Estado |
|---|---|---|
| Shell de janela | Window (GLFW+ImGui), `WindowDecorator`, `TitleBar`, `NativeWindow` (titlebar borderless), `NativeMenuBar`, `SystemTheme` | já genérico |
| Shell de app | DockSpace coordinator, `PanelRegistry`/`IPanel`, `ViewerRegistry`/`IDocumentContent`, `DocumentWindow` | `IPanel` genérico; `ViewerRegistry` já roteia por `MediaKind` |
| Serviços | `ThemeManager`, `FontManager`, `ScaleManager`, `Logger`, `Metrics`, `TaskManager`, `Threading`, `AppConfig`, `RecentFiles`, `EventManager`, `PathUtils` | já genéricos |
| Núcleo de dados | VFS (`IVirtualFileSystem`, `IFile`, `IsoFileSystem`, `OsFile`, `MemoryFile`, `SliceFile`), schema (`StructDef`, `NodeInstance`, `AssetReader`, `AssetNode`), modelo genérico (`AssetContainer`←`OpenWad`, `AssetEntry`←`ParsedEntry`, `MediaKind`), `TypeRegistry` aberto, plugin (`IAssetProfile`←`IGameProfile`, `ProfileManager`), `AssetDatabase` | renomear/abstrair |
| Viewers genéricos | `Viewport3D` + render stack (`SceneRenderer`, `GpuMesh`, `Camera`, `GridRenderer`, `ShaderManager`), `ImageViewer`, `SoundPlayer`, `VideoPlayer`, `TextEditorViewer`, HexView, `Inspector` schema-driven | já format-agnostic |
| Panels genéricos | Tree browser genérico (padrão `WadBrowser`/`IsoBrowser`), `StatusBar`, `SettingsWindow` | generalizar colunas/ações |

### `app/` — específico do jogo (GoWToolkit; depois SCUMMRedux)

| Camada | Conteúdo |
|---|---|
| Profiles | `ProfileGOW2`, `ProfileGOWR` (→ profiles SCUMM) |
| Parsers | `parsers/gow2/*`, `parsers/gowr/*`, taxonomia (`GowrTaxonomy`, `WadAssetName`) |
| Tipos | registrações de tipo do jogo + `ITypeHandler`s |
| Viewers de domínio | `MaterialViewer`, `MapViewer`, etc. |
| Wiring | `Main.cpp` registra profiles + tipos + viewers no engine |

### Zona cinzenta (decisão: a favor do engine, app customiza)

- **Browsers:** engine fornece tree browser genérico; app injeta colunas, ícones,
  ações de contexto. Não duplica a árvore.
- **Inspector:** engine renderiza `NodeInstance` via `StructDef`
  automaticamente; viewers de jogo podem sobrescrever (`DrawInspector`).
- **`Viewport3D` é engine**; `MaterialViewer`/`MapViewer` são app (carregam
  semântica de formato).

**Contrato fundamental:** um app é *engine + N profiles + N type-registrations +
N viewers customizados*, plugados via os registries existentes.

---

## §2 — Mudanças de genericização

### A. Renome + namespace neutro

| Hoje (GoW) | Engine (genérico) |
|---|---|
| `namespace GOW` | namespace `Onyx` (+ sub-namespaces) |
| `OpenWad` | `AssetContainer` |
| `ParsedEntry` | `AssetEntry` |
| `IGameProfile` | `IAssetProfile` |
| `IGameProfile::ParseWad(file, OpenWad&)` | `IAssetProfile::ParseContainer(file, AssetContainer&)` |
| `core/WadTypes.h` (umbrella legado) | aposentado |

Nomes "Wad" continuam livres **no app** (`ProfileGOW2` fala "WAD" à vontade); só
o engine fica neutro.

### B. `TypeId` fechado → registro aberto (o coração)

Separa duas responsabilidades hoje misturadas no `TypeRegistry`:

1. **"bytes → qual tipo é" (dispatch)** → **sai do engine, vira responsabilidade
   do profile/app.** Cada jogo identifica tipos diferente (GoW: tag+magic no WAD
   via `GameVersion`/`TAG_SERVER_INSTANCE`; SCUMM: block tags `ROOM`/`SCRP`/
   `COST`). O engine não pode saber isso.
2. **"tipo → metadados" (ícone, label, `MediaKind`, handler)** → **fica no
   engine**, como registro aberto:

```cpp
// engine: MediaKind continua o ÚNICO enum fechado — decide qual viewer abre
enum class MediaKind { Unknown, Image, Sound, Video, Mesh, Material, Text, Binary, Container, /* … */ };

// engine: TypeId vira handle opaco interno (0 = Unknown)
struct TypeId { uint32_t id = 0; /* ==, hash */ };

struct TypeInfo {
    std::string key;     // "GOW2_MESH", "SCUMM_COSTUME" (estável, do app)
    std::string label;   // nome humano
    MediaKind   media;   // ← roteia pro viewer via ViewerRegistry
    const char* icon;    // codicon
};

// app registra no startup; engine devolve um TypeId interno e indexa
TypeId          TypeRegistry::Register(const TypeInfo&);
const TypeInfo& TypeRegistry::Info(TypeId) const;
```

O `ViewerRegistry` **não muda de mecânica** — continua `MediaKind → viewer`.

### C. Profile e config

- `IAssetProfile` mantém `Detect`/`MountArchive`/`LoadFromArchive`; troca
  `ParseWad`→`ParseContainer` e os tipos de saída. `ProfileManager` já é registry
  limpo — só sai do namespace.
- **`AppConfig`:** engine fica dono da config genérica (janela, tema, recents,
  layout de panels); o app registra um **blob de config próprio** que o engine
  persiste junto, sem conhecer o conteúdo. (Hoje é o formato binário "GTKC"
  GoW-específico.)

---

## §3 — Estrutura de pastas e build

### Fase 1 — monorepo in-place (layout já final)

```
GoWToolkit/                          (repo atual; renomeia/divide depois)
├── CMakeLists.txt                   ← topo: define SDK_NAME, busca deps, agrega subdirs
├── CMake/                           ← helpers (Dependencies, CopyRuntime, MacBundle)
├── ThirdParty/                      ← deps compartilhadas (imgui, glfw, glm, lz4, glad,
│                                       miniaudio, implot, ffmpeg…)
│
├── Engine/                          ← O SDK → target estático ${SDK_NAME}
│   ├── CMakeLists.txt
│   ├── Include/Onyx/                ← API PÚBLICA (apps: #include <Onyx/…>)
│   │   ├── Vfs/  Schema/  Types/
│   │   ├── App/  (Shell, IPanel, registries, IDocumentContent)
│   │   ├── Viewers/  Services/
│   └── Source/                      ← implementação (privada)
│       ├── Window/  App/  Core/  Viewers/  Rendering/  Platform/
│
├── Apps/
│   └── GoWToolkit/                  ← primeiro consumidor → executável
│       ├── CMakeLists.txt
│       ├── Source/
│       │   ├── Main.cpp             ← WIRING: registra profiles + tipos + viewers
│       │   ├── Profiles/  Parsers/  Types/  Viewers/
│       └── Dist/                    (Info.plist, ícone, fontes do app)
│
└── Tests/
    ├── Engine/                      (unit: vfs, schema, threading, theme, métricas)
    └── GoWToolkit/                  (golden GOW2/GOWR — nível app)
```

**Targets CMake:**
- Topo: `set(SDK_NAME Onyx)`, busca deps `ThirdParty` uma vez, `add_subdirectory`
  de `Engine` e `Apps/GoWToolkit`.
- `Engine/`: `add_library(${SDK_NAME} STATIC …)` +
  `target_include_directories(${SDK_NAME} PUBLIC Include/)`. Deps viram **PUBLIC**
  (apps herdam). 
- `Apps/GoWToolkit/`: `add_executable` + `target_link_libraries(… PRIVATE
  ${SDK_NAME})`. Bundle macOS, cópia de DLL do FFmpeg, ícone, `Info.plist` moram
  aqui (ou em `CMake/`).
- **`gowtoolkit_parser_min` deixa de existir** — o `Engine/` real vira a lib que
  os testes consomem.

**Faseamento do `Include/` público:** na Fase 1 o `Engine/` começa com **um único
include root** (tudo visível) só pra provar que compila/roda separado; a separação
fina público vs privado entra **incremental depois**, guiada pelo que o GoWToolkit
de fato precisou incluir.

### Fase 2 — repo do SDK desacoplado

```
OnyxSDK/                             ← repo próprio, versionado e publicável
├── CMakeLists.txt                   (lib OnyxSDK + install/export)
├── CMakePresets.json   README.md   LICENSE   CHANGELOG.md
├── CMake/                           (Dependencies, CopyRuntime, MacBundle)
├── ThirdParty/
├── Include/Onyx/                    ← API PÚBLICA: #include <Onyx/…>
│   ├── Onyx.h  (umbrella)
│   ├── Vfs/  Schema/  Types/  App/  Viewers/  Rendering/  Services/
├── Source/
│   ├── Window/  App/  Core/  Viewers/  Rendering/  Platform/
├── Tests/
├── Examples/
│   └── MinimalViewer/               ← app de referência mínimo (2º consumidor da API)
└── Docs/
    ├── GettingStarted.md  Architecture.md
    ├── WritingAProfile.md           ← "como plugar um jogo novo"
    └── RegisteringTypes.md
```

**Consumo pelo app (modern CMake, alias namespaced `Onyx::Onyx`):**

```cmake
include(FetchContent)
FetchContent_Declare(OnyxSDK
    GIT_REPOSITORY https://github.com/<voce>/OnyxSDK.git
    GIT_TAG        v0.1.0)
FetchContent_MakeAvailable(OnyxSDK)

add_executable(GoWToolkit Source/Main.cpp …)
target_link_libraries(GoWToolkit PRIVATE Onyx::Onyx)
```

Habilita também: `install()` + `OnyxConfig.cmake` (consumo via
`find_package(Onyx)`) — mas só quando houver demanda (ver não-objetivos).

---

## §4 — Plano de migração faseado (dogfooding)

**Invariante de todo passo:** ao final de cada milestone, `GoWToolkit` compila
**e** `ctest` (golden `GOW2`/`GOWR`) passa verde. Quebrou → o passo era grande
demais; desfaz e fatia menor.

**Lógica da ordem:** mudança de lógica arriscada (sistema de tipos) **primeiro,
com tudo numa árvore só** (lugar mais barato, golden cobre); churn física
(mover/renomear/dividir) **depois**, com semântica já estável.

### M0 — Rede de segurança
Branch dedicada. Roda golden, confirma baseline verde, congela snapshots.
*Checkpoint:* `ctest` verde no estado atual.

### M1 — Genericizar no lugar (sem mover arquivo)
- **M1a** — `namespace GOW` → `Onyx`. Mecânico.
- **M1b** — `OpenWad`→`AssetContainer`, `ParsedEntry`→`AssetEntry`,
  `IGameProfile`→`IAssetProfile`, `ParseWad`→`ParseContainer`. Aposenta
  `WadTypes.h`.
- **M1c** *(único trabalho de lógica real)* — abre o `TypeId` (enum → handle
  registrado); move dispatch de WAD (tag/magic, `GameVersion`,
  `TAG_SERVER_INSTANCE`) pro profile GOW; engine fica só com `TypeId → TypeInfo →
  MediaKind`.
*Checkpoint após cada sub-passo:* build + golden verde.

### M2 — Partir em dois targets CMake
Cria `Engine/` + `Apps/GoWToolkit/`, move arquivos pelo boundary da §1. Aposenta
`gowtoolkit_parser_min`. Conserta includes; **vazamentos de boundary aparecem
aqui** e são consertados/expostos de propósito.
*Checkpoint:* GoWToolkit linka contra `Engine/` como lib separada + golden verde.

### M3 — PascalCase + superfície pública incremental
Renomeia diretórios pra PascalCase. Introduz `Include/Onyx/` na medida em que o
app precisa.
*Checkpoint:* build + golden verde.

### M4 — `MinimalViewer`: o 2º consumidor (prova do desacoplamento)
App mínimo que **não sabe nada de GoW**: abre arquivo, mostra árvore + hex, usando
só a API pública. Se roda sem linkar `Apps/GoWToolkit`, o SDK está provadamente
desacoplado.
*Checkpoint:* `MinimalViewer` roda standalone. **Marco "o SDK existe de verdade".**

### M5 — Split físico do repo (Fase 2)
Extrai `Engine/` pro repo `OnyxSDK` com `git filter-repo` (preserva histórico).
GoWToolkit consome via `FetchContent` numa tag. Move `MinimalViewer` pra
`Examples/`. Tag `OnyxSDK v0.1.0`.
*Checkpoint:* GoWToolkit builda puxando OnyxSDK por FetchContent + golden verde.
**SCUMMRedux pode nascer.**

### Propriedades
- Cada milestone é independente e deixa a `main` verde — pode parar em qualquer
  um.
- Reversível: M0–M1 são find-replace; risco real só no M2, com semântica estável.
- SCUMMRedux só depende do M5, mas pode ser prototipado contra o `Engine/`
  in-repo já no M4.

---

## §5 — Rede de segurança e testes

- **Golden tests (`GOW2`/`GOWR`) — rede principal.** Capturam snapshot do parse;
  qualquer regressão de M1 quebra na hora. Guardião do M1c.
- **Unit tests do engine** (`vfs`, `schema`, `threading`, `theme contrast`,
  `metrics`, `logger`) — migram pra `Tests/Engine`, validam o engine isolado.
- **`MinimalViewer` — guarda da API pública.** Não conhece GoW; teste de fumaça
  vivo da superfície do SDK.
- **Portão de CI por milestone:** `build + ctest` verde para fundir.
- **Checagem estrutural de boundary** (a partir do M2): `Engine/` não linka
  `Apps/` nem referencia identificadores de jogo (check de grep no CI).
- **Preservação de histórico no M5:** `git filter-repo`.

### Não-objetivos (YAGNI)

- Não generalizar para formatos que nenhum app real usa ainda.
- Não versionar/estabilizar a API pública **antes do M4** — ela emerge do uso.
- Não suportar `find_package`/install no M5 — FetchContent basta; entra só quando
  houver um terceiro app pedindo.

---

## Questões em aberto

- **Nome definitivo do SDK** (`Onyx` é placeholder) — checar
  domínio/crate/npm antes de travar.
- **Generalização do tree browser** (colunas/ações injetáveis) — detalhar a
  interface de customização no plano de implementação.
- **Formato do blob de config do app** dentro do `AppConfig` do engine —
  definir o mecanismo de serialização opaca.
