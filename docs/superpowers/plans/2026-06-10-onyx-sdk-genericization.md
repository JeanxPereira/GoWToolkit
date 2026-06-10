# Onyx SDK — Genericization (Plano 1: M0–M1b) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Neutralizar a nomenclatura GoW-específica do GoWToolkit (namespace + tipos do modelo de dados) sem mover arquivos nem mudar lógica, deixando o build e os golden tests verdes — a fundação para o split engine/app posterior.

**Architecture:** Apenas renomes mecânicos verificados pelo compilador e pela suíte golden existente. `namespace GOW` → `Onyx`; `OpenWad`→`AssetContainer`, `ParsedEntry`→`AssetEntry`, `IGameProfile`→`IAssetProfile`, `ParseWad`→`ParseContainer`. Nenhum arquivo muda de lugar; nenhuma interface muda de forma. A rede de segurança é o build completo + `ctest` (golden GOW2/GOWR + unit tests).

**Tech Stack:** C++20, CMake + Ninja, doctest (unit), golden snapshot tests (nlohmann/json). Plataforma de execução: Windows (PowerShell 5.1) com toolchain MSVC.

**Escopo deste plano:** SÓ M0, M1a, M1b. M1c (abrir o sistema de tipos) e M2–M5 são planos subsequentes — ver "Roadmap dos planos seguintes" no fim. A razão da fronteira: M1c está entrelaçado com o split engine/app (o mapa `TypeId→MediaKind` em `MediaKind.cpp` só "abre" naturalmente quando o app sai do engine), então é planejado junto com M2.

**Convenção de verificação (vale para todas as tasks):** como isto é refactor puro, o "teste" de cada passo é *build limpo + `ctest` verde*, não um teste novo. Os golden tests são a rede que acusa regressão semântica.

**Comandos canônicos** (rode da raiz do repo, num shell com MSVC no PATH — ex.: "x64 Native Tools Command Prompt" ou após `Enter-VsDevShell`):

```
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Subconjunto golden apenas: `ctest --test-dir build -R Golden --output-on-failure`

---

## Pré-requisito: branch de execução

- [ ] **Step 1: Criar a branch de execução a partir da branch do spec**

Run:
```
git checkout docs/onyx-sdk-spec
git checkout -b feat/onyx-genericization
```
Expected: `Switched to a new branch 'feat/onyx-genericization'`

---

## Task 0: Baseline verde (M0 — rede de segurança)

Estabelece que o estado ATUAL compila e passa, para que qualquer quebra posterior seja atribuível ao refactor.

**Files:** nenhum (só verificação).

- [ ] **Step 1: Configurar o build do zero**

Run:
```
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -B build
```
Expected: termina com `-- Configuring done` / `-- Generating done` / `-- Build files have been written to: .../build`, sem `FATAL_ERROR`.

- [ ] **Step 2: Compilar tudo**

Run:
```
cmake --build build
```
Expected: compila `GoWToolkit`, `gowtoolkit_parser_min` e `gowtoolkit_tests` sem erros.

- [ ] **Step 3: Rodar a suíte completa e confirmar verde**

Run:
```
ctest --test-dir build --output-on-failure
```
Expected: `100% tests passed`. Inclui `unit`, `Golden_GOW2`, `Golden_GOWR`, `Metrics`, `Logger`, `Threading`, `ThemeContrast`.

> Se algum golden falhar AQUI (antes de qualquer mudança), PARE: o problema é ambiental (test assets ausentes, toolchain). Resolva antes de prosseguir — o plano inteiro depende deste baseline verde. Se os golden dependem de ISOs/WADs locais não versionados e eles não existirem nesta máquina, registre isso e use "build limpo + `unit`/`Metrics`/`Logger`/`Threading`/`ThemeContrast` verdes" como rede mínima nas tasks seguintes.

---

## Task 1: Renomear `namespace GOW` → `Onyx` (M1a)

Renomeia o token de namespace `GOW` em todo `src/` e `tests/`. Usa fronteira de palavra (`\bGOW\b`) para NÃO tocar `GOW2`, `GOWR`, `GOWTOOL_OS_*`, `_GOW_REG_CONCAT`, `ProfileGOW2` etc. (todos têm caractere de palavra ou `_` adjacente, então `\b` não casa).

**Files:**
- Modify: todos os `*.h *.hpp *.cpp *.mm *.m` sob `src/` e `tests/` que contêm `\bGOW\b` (174 arquivos no momento da redação).

- [ ] **Step 1: Pré-visualizar o impacto (quantos arquivos casam)**

Run (PowerShell):
```powershell
$re = [regex]'\bGOW\b'
(Get-ChildItem -Path src,tests -Recurse -Include *.h,*.hpp,*.cpp,*.mm,*.m |
  Where-Object { $re.IsMatch([IO.File]::ReadAllText($_.FullName)) }).Count
```
Expected: imprime um número na casa de ~170. (Sanidade: confirma que o regex casa antes de escrever.)

- [ ] **Step 2: Aplicar o replace preservando encoding (sem BOM)**

Run (PowerShell):
```powershell
$enc = New-Object System.Text.UTF8Encoding($false)
$re  = [regex]'\bGOW\b'
Get-ChildItem -Path src,tests -Recurse -Include *.h,*.hpp,*.cpp,*.mm,*.m | ForEach-Object {
  $c = [IO.File]::ReadAllText($_.FullName)
  $n = $re.Replace($c, 'Onyx')
  if ($n -ne $c) { [IO.File]::WriteAllText($_.FullName, $n, $enc) }
}
```
Expected: sem saída (sucesso silencioso).

- [ ] **Step 3: Verificar que não sobrou `GOW::` nem `namespace GOW` solto**

Run (PowerShell):
```powershell
Select-String -Path (Get-ChildItem -Path src,tests -Recurse -Include *.h,*.hpp,*.cpp,*.mm,*.m).FullName -Pattern '\bGOW\b' |
  Select-Object -First 20
```
Expected: nenhuma linha. (Se aparecer algo, é um caso legítimo perdido — inspecione; provavelmente um literal de string. Corrija à mão.)

- [ ] **Step 4: Build**

Run:
```
cmake --build build
```
Expected: compila sem erros. (Erros de "`GOW` is not a namespace" indicam um ponto não renomeado — conserte e repita.)

- [ ] **Step 5: Testes**

Run:
```
ctest --test-dir build --output-on-failure
```
Expected: `100% tests passed` (mesmo conjunto da Task 0).

- [ ] **Step 6: Commit**

```
git add -A
git commit -m "refactor(core): Rename namespace GOW to Onyx"
```

> Nota macOS: `SystemTheme_macos.mm` é renomeado pelo script mas NÃO compila no Windows. O replace é o mesmo regex mecânico; o risco é nulo, mas registre que a verificação real desse arquivo ocorre num build macOS futuro.

---

## Task 2: Renomear os tipos do modelo de dados (M1b)

Renomes de identificador, com fronteira de palavra, em `src/` e `tests/`:

| De | Para |
|---|---|
| `OpenWad` | `AssetContainer` |
| `ParsedEntry` | `AssetEntry` |
| `IGameProfile` | `IAssetProfile` |
| `ParseWad` | `ParseContainer` |

`\bParseWad\b` não casa `ParseWadHeader`/similares (caractere de palavra após). `\bOpenWad\b` não casa `OpenWadDialog` se existir (verificado no Step 1). Nomes app-específicos como `WadAssetName`, `WadBrowser`, `WadNodeBuilder`, `WadStatsView` permanecem (são "Wad" do app, intencional).

**Files:**
- Modify: arquivos sob `src/` e `tests/` contendo qualquer um dos 4 tokens. Inclui notavelmente `src/core/domain/Wad.h` (define `OpenWad`→`AssetContainer`), `src/core/domain/Entry.h` (define `ParsedEntry`→`AssetEntry`), `src/core/interfaces/IGameProfile.h`, `src/core/profiles/gow2/ProfileGOW2.*`, `src/core/profiles/gowr/ProfileGOWR.*`, `src/ui/ViewerRegistry.*`, `src/core/types/ITypeHandler.h`.

> Nota: este passo NÃO renomeia os arquivos `Wad.h`/`Entry.h` nem aposenta `WadTypes.h` — isso é churn de arquivo, reservado para M3. Aqui só os *tipos* mudam de nome; os arquivos ficam onde estão.

- [ ] **Step 1: Pré-visualizar colisões de cada token**

Run (PowerShell):
```powershell
foreach ($t in 'OpenWad','ParsedEntry','IGameProfile','ParseWad') {
  $re = [regex]("\b$t" + "\w+")   # casa o token SEGUIDO de mais letras (falsos positivos)
  $hits = Get-ChildItem -Path src,tests -Recurse -Include *.h,*.hpp,*.cpp,*.mm,*.m |
    ForEach-Object { $re.Matches([IO.File]::ReadAllText($_.FullName)) } |
    ForEach-Object { $_.Value } | Sort-Object -Unique
  "${t}: " + ($hits -join ', ')
}
```
Expected: lista identificadores que COMEÇAM com cada token mas continuam (ex.: `ParsedEntryList` seria um falso positivo a evitar). Se algum aparecer e for um tipo distinto que NÃO deve mudar, anote — o replace com `\b...\b` abaixo já o protege (só troca o token exato). Confirme que não há, por exemplo, um `OpenWadV2` que precisaria de tratamento separado.

- [ ] **Step 2: Aplicar os 4 replaces (token exato, sem BOM)**

Run (PowerShell):
```powershell
$enc = New-Object System.Text.UTF8Encoding($false)
$map = [ordered]@{
  'OpenWad'       = 'AssetContainer'
  'ParsedEntry'   = 'AssetEntry'
  'IGameProfile'  = 'IAssetProfile'
  'ParseWad'      = 'ParseContainer'
}
Get-ChildItem -Path src,tests -Recurse -Include *.h,*.hpp,*.cpp,*.mm,*.m | ForEach-Object {
  $c = [IO.File]::ReadAllText($_.FullName); $n = $c
  foreach ($k in $map.Keys) { $n = [regex]::Replace($n, "\b$k\b", $map[$k]) }
  if ($n -ne $c) { [IO.File]::WriteAllText($_.FullName, $n, $enc) }
}
```
Expected: sem saída.

- [ ] **Step 3: Verificar que os nomes antigos sumiram**

Run (PowerShell):
```powershell
Select-String -Path (Get-ChildItem -Path src,tests -Recurse -Include *.h,*.hpp,*.cpp,*.mm,*.m).FullName `
  -Pattern '\bOpenWad\b','\bParsedEntry\b','\bIGameProfile\b','\bParseWad\b' | Select-Object -First 20
```
Expected: nenhuma linha.

- [ ] **Step 4: Build**

Run:
```
cmake --build build
```
Expected: compila sem erros.

- [ ] **Step 5: Testes**

Run:
```
ctest --test-dir build --output-on-failure
```
Expected: `100% tests passed`.

- [ ] **Step 6: Commit**

```
git add -A
git commit -m "refactor(core): Rename WAD-shaped model types to neutral asset names"
```

---

## Task 3: Atualizar a documentação de arquitetura para os novos nomes

Os comentários de código já foram atualizados pelos replaces (continham os tokens). Falta o `CLAUDE.md` e o spec, que mencionam as interfaces antigas em prosa e devem refletir os novos nomes para não confundir quem ler depois.

**Files:**
- Modify: `CLAUDE.md` (seção "Key Interfaces" e "Data Flow" mencionam `IGameProfile`, `OpenWad`, `ParseWad`).

- [ ] **Step 1: Atualizar as menções em `CLAUDE.md`**

Troque, na prosa do `CLAUDE.md`:
- `IGameProfile` → `IAssetProfile` (mantendo a nota de que `ProfileGOW2`/`ProfileGOWR` o implementam)
- `IGameProfile::ParseWad` → `IAssetProfile::ParseContainer`
- `OpenWad` → `AssetContainer`

Use o Edit tool para cada ocorrência (são poucas, em "Data Flow" e "Key Interfaces"). NÃO mude nomes de arquivos de referência Go (`pack/wad/...`) nem o scope `wad` dos commits — esses são externos/convenção.

- [ ] **Step 2: Verificar build de doc não quebrou nada (sanity)**

Run:
```
git diff --stat CLAUDE.md
```
Expected: mostra `CLAUDE.md` modificado, poucas linhas.

- [ ] **Step 3: Commit**

```
git add CLAUDE.md
git commit -m "docs(claude): Reflect neutral SDK names in architecture notes"
```

---

## Verificação final do Plano 1

- [ ] **Step 1: Build limpo do zero (garante que não há estado de build obsoleto mascarando erro)**

Run:
```
cmake --build build --target clean
cmake --build build
```
Expected: recompila tudo sem erros.

- [ ] **Step 2: Suíte completa**

Run:
```
ctest --test-dir build --output-on-failure
```
Expected: `100% tests passed`.

- [ ] **Step 3: Confirmar ausência total dos nomes antigos no código**

Run (PowerShell):
```powershell
Select-String -Path (Get-ChildItem -Path src,tests -Recurse -Include *.h,*.hpp,*.cpp,*.mm,*.m).FullName `
  -Pattern '\bGOW::','\bnamespace GOW\b','\bOpenWad\b','\bParsedEntry\b','\bIGameProfile\b','\bParseWad\b' |
  Select-Object -First 20
```
Expected: nenhuma linha. Plano 1 concluído.

---

## Roadmap dos planos seguintes (não implementar aqui)

Estes serão escritos como planos próprios, cada um *após* o anterior estar verde, porque seus passos exatos dependem do estado final do anterior.

### Plano 2 — Abrir o sistema de tipos + split engine/app (M1c + M2)
O `TypeId` enum só "abre" naturalmente quando o app sai do engine, então M1c e M2 andam juntos.
- **Engine ganha** (`Types/`): `TypeId` como handle opaco (`struct TypeId { uint32_t value; }`), `MediaKind` (já existe, fica), e um `TypeCatalog` aberto (`Register(TypeInfo)→TypeId`, `Info(TypeId)`, `Media(TypeId)`). `MediaKind` continua o único enum fechado.
- **App ganha**: o `enum class TypeId` atual (renomeado p/ ex. `Gow2TypeId`), o mapa `TypeId→MediaKind` (hoje em `MediaKind.cpp`, 51 refs), os `ITypeHandler` + `TypeRegistry` (dispatch tag/magic) e as macros `REGISTER_*`. O app registra cada tipo no `TypeCatalog` no startup.
- Os **3 `switch` em `TypeVisuals.h`** viram cadeias `if/else` comparando handles.
- Cria `Engine/` + `Apps/GoWToolkit/`, move arquivos pelo boundary da §1 do spec, aposenta `gowtoolkit_parser_min`, faz `engine` linkar como lib. Conserta vazamentos de boundary que surgirem.
- Rede: golden + o novo unit test de `TypeCatalog`.

### Plano 3 — PascalCase + superfície pública + MinimalViewer (M3 + M4)
- Renomeia diretórios para PascalCase; introduz `Include/Onyx/` incremental.
- Cria `MinimalViewer` (2º consumidor que não conhece GoW) — o marco "o SDK existe".

### Plano 4 — Split físico do repo (M5)
- `git filter-repo` extrai `Engine/` para o repo `OnyxSDK`; GoWToolkit consome via FetchContent numa tag; `MinimalViewer` vira `Examples/`. Tag `v0.1.0`.

### Pendências de design herdadas do spec (resolver no plano pertinente)
- Nome definitivo do SDK (`Onyx` é placeholder — checar domínio/crate/npm) → antes do Plano 4.
- Interface de customização do tree browser genérico → Plano 2/3.
- Formato do blob de config opaco do app dentro do `AppConfig` → Plano 2.
