# Piper (TTS) — C++ embedding surface, espeak-ng-data, Android availability, published speed numbers

Primary-source research report. Every claim carries an inline source URL. Where a number is not
published, this report says "not published" rather than estimating.

**Scope of the two canonical repos**

| Repo | Branch | State | Licence |
|---|---|---|---|
| `rhasspy/piper` | `master` | ARCHIVED (README is a redirect stub) | MIT (`LICENSE.md`) |
| `OHF-Voice/piper1-gpl` | `main` | ACTIVE | GPL-3.0 (`COPYING`) |

Evidence for the ARCHIVED/redirect state: `https://raw.githubusercontent.com/rhasspy/piper/master/README.md`
returns exactly 63 bytes: `Development has moved: https://github.com/OHF-Voice/piper1-gpl`.
Licence file names/locations: `https://api.github.com/repos/rhasspy/piper/contents/` (lists `LICENSE.md`)
and `https://api.github.com/repos/OHF-Voice/piper1-gpl/git/trees/main?recursive=1` (lists `COPYING`).
Product description and licence text were **not** read line-by-line in this pass — the SPDX identifiers
asserted in the task brief are reproduced here only as repo file names, not as verified licence text.

**Research limitations encountered (affects completeness, not the findings below)**

- `api.github.com` returned HTTP 403 `API rate limit exceeded` for several calls. Three intended
  calls never succeeded and their content is therefore **not determinable from public sources in this
  pass**: `repo:rhasspy/piper+termux` issue search, `repos/rhasspy/piper-phonemize/releases`
  (published size of the legacy `piper-phonemize` bundles that carried `espeak-ng-data`), and
  `repos/OHF-Voice/piper1-gpl/issues/130/comments`.
- `https://raw.githubusercontent.com/rhasspy/piper/2023.11.14-2/README.md` and
  `https://raw.githubusercontent.com/rhasspy/piper-samples/master/README.md` failed once with a
  transport error; the former was retried successfully, the latter was never obtained.

---

## 1. What inference runtime Piper uses

### 1.1 The premise "src/cpp/CMakeLists.txt" is wrong — that file does not exist

- `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/CMakeLists.txt` → **HTTP 404**.
- `https://api.github.com/repos/rhasspy/piper/contents/src/cpp` lists only: `json.hpp`, `main.cpp`,
  `piper.cpp`, `piper.hpp`, `test.cpp`, `utf8.h`, `utf8/` (dir), `wavfile.hpp`. **No build file of any
  kind is in `src/cpp/`.**
- The legacy build configuration is at the **repo root**:
  `https://raw.githubusercontent.com/rhasspy/piper/master/CMakeLists.txt` (4301 bytes per
  `https://api.github.com/repos/rhasspy/piper/contents/`), driven by
  `https://raw.githubusercontent.com/rhasspy/piper/master/Makefile`.

### 1.2 Legacy runtime: ONNX Runtime — proven by the link line, not by `find_package`

Exact string from `https://raw.githubusercontent.com/rhasspy/piper/master/CMakeLists.txt`:

```
target_link_libraries(piper
  fmt
  spdlog
  espeak-ng
  piper_phonemize
  onnxruntime
  ${PIPER_EXTRA_LIBRARIES}
)
```

Linked targets, verbatim from that same file: **`fmt`, `spdlog`, `espeak-ng`, `piper_phonemize`,
`onnxruntime`** (plus `pthread` on non-MSVC/non-Apple).

- There is **no `find_package(onnxruntime ...)` line anywhere in the legacy root `CMakeLists.txt`**.
  The file instead carries this exact comment immediately above the dependency block:
  `# NOTE: onnxruntime is pulled from piper-phonemize`
- The upstream sources are fetched as ExternalProjects in the same file:
  - `piper-phonemize`: `URL "https://github.com/rhasspy/piper-phonemize/archive/refs/heads/master.zip"`
  - `fmt`: `set(FMT_VERSION "10.0.0")`, from `https://github.com/fmtlib/fmt/archive/refs/tags/${FMT_VERSION}.zip`
  - `spdlog`: `set(SPDLOG_VERSION "1.12.0")`, from `https://github.com/gabime/spdlog/archive/refs/tags/v${SPDLOG_VERSION}.zip`
- Confirmed at the include level: `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.hpp`
  contains `#include <onnxruntime_cxx_api.h>` and `#include <piper-phonemize/phoneme_ids.hpp>`,
  `<piper-phonemize/phonemize.hpp>`, `<piper-phonemize/tashkeel.hpp>`.
- Confirmed at the API level: `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.cpp`
  contains `#include <espeak-ng/speak_lib.h>`, `#include <onnxruntime_cxx_api.h>`,
  `session.env = Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, instanceName.c_str());` and
  `session.onnx.Run(Ort::RunOptions{nullptr}, inputNames.data(), ...)`.

### 1.3 Legacy `libpiper`? — no. The legacy build produces executables only

Exact strings from `https://raw.githubusercontent.com/rhasspy/piper/master/CMakeLists.txt`:

```
add_executable(piper src/cpp/main.cpp src/cpp/piper.cpp)
add_executable(test_piper src/cpp/test.cpp src/cpp/piper.cpp)
```

There is **no `add_library(...)` of any kind** in the legacy root `CMakeLists.txt`. The sole
`install(TARGETS ...)` directive installs the `piper` executable. Therefore the legacy repo publishes
**no shared library named `libpiper`** and no shared library at all.

### 1.4 Current repo: `libpiper` IS a shared library, and there is also a CLI executable

There are **two** CMake projects in `OHF-Voice/piper1-gpl`:

1. repo-root `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CMakeLists.txt` — builds the
   **Python extension** (`add_library(espeakbridge MODULE src/piper/espeakbridge.c)`), invoked by
   scikit-build from `setup.py`.
2. `libpiper/CMakeLists.txt` — builds the **C/C++ shared library**.

Path verification: `https://api.github.com/repos/OHF-Voice/piper1-gpl/git/trees/main?recursive=1`
(non-truncated, `"truncated":false`) confirms the paths `libpiper/CMakeLists.txt`,
`libpiper/include/piper.h`, `libpiper/include/piper_impl.hpp`, `libpiper/src/piper.cpp`,
`libpiper/src/main/CMakeLists.txt`, `libpiper/src/main/main.cpp`, `libpiper/tests/CMakeLists.txt`.
There is **no `src/cpp` directory** in piper1-gpl.

Exact strings from `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt`:

```
add_library(piper SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/piper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/chinese_phonemizer.cpp
)
```

```
target_link_libraries(piper
    ${ESPEAKNG_STATIC_LIB}
    ${UCD_STATIC_LIB}
    onnxruntime::onnxruntime
)
```

So **`libpiper` is a shared library** (CMake target `piper`, `SHARED`, hence `libpiper.so` /
`piper.dll`). `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md` states:
`A shared library for Piper with a C-style API.` and, under `To use \`libpiper\`, you will need to:`:
`Link to the \`libpiper\` library (\`install/\`)`.

Exact ONNX Runtime resolution lines, from `libpiper/CMakeLists.txt`:

```
find_package(onnxruntime QUIET)
```

```
if(NOT DEFINED ONNXRUNTIME_VERSION)
    set(ONNXRUNTIME_VERSION "1.22.0")
endif()
```

with the platform dispatch:

```
set(ONNXRUNTIME_PREFIX "onnxruntime-win-x64-${ONNXRUNTIME_VERSION}")      # Windows x86-64
set(ONNXRUNTIME_PREFIX "onnxruntime-osx-x86_64-${ONNXRUNTIME_VERSION}")  # macOS x86-64
set(ONNXRUNTIME_PREFIX "onnxruntime-osx-arm64-${ONNXRUNTIME_VERSION}")   # macOS Apple Silicon
set(ONNXRUNTIME_PREFIX "onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}")   # Linux x86-64
set(ONNXRUNTIME_PREFIX "onnxruntime-linux-aarch64-${ONNXRUNTIME_VERSION}")  # Linux ARM 64-bit
set(ONNXRUNTIME_PREFIX "onnxruntime-linux-arm32-${ONNXRUNTIME_VERSION}")    # Linux ARM 32-bit
```

and the fallthrough:

```
else()
    message(FATAL_ERROR "Unsupported architecture for onnxruntime")
endif()
```

The Linux arm32 build is special-cased to a third-party mirror:
`ONNXRUNTIME_URL "https://github.com/synesthesiam/prebuilt-apps/releases/download/v1.0/onnxruntime-linux-arm32-${ONNXRUNTIME_VERSION}.tgz"`.
All other platforms default to
`https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${ONNXRUNTIME_PREFIX}.${ONNXRUNTIME_EXT}`.

**Note on the escape hatch:** the whole platform-dispatch block is guarded by
`if(NOT TARGET onnxruntime::onnxruntime AND NOT DEFINED ONNXRUNTIME_DIR)`. If a caller defines
`ONNXRUNTIME_DIR`, the block is skipped and only
`find_library(ONNXRUNTIME_LIB NAMES onnxruntime PATHS ${ONNXRUNTIME_DIR}/lib REQUIRED)` runs.
The code contains **no `ANDROID` branch and no NDK toolchain handling** (see §4).

### 1.5 The CLI executable in the current repo, and the v1.5.0 release note

`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/CMakeLists.txt`:

```
add_executable(piper_exe main.cpp)
...
install(TARGETS piper_exe RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
```

So the CLI executable's CMake target is **`piper_exe`**, and it links the shared library:
`target_link_libraries(piper_exe PRIVATE piper main_utils)`.

Requested verbatim verification — the release body of
`https://api.github.com/repos/OHF-Voice/piper1-gpl/releases/tags/v1.5.0` begins:

```
- Add `libpiper` C++ CLI executable ported from the legacy Piper repository, plus a C++ test suite
```

(Note: the full first bullet begins with `- Add `; the task's paraphrase omitted the leading "Add".)
The same bullet appears in `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md`
under `## 1.5.0`.

The CHANGELOG also documents the origin of the split
(`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md`, `## 1.3.0`):
`Removed C++ code for now to focus on Python development` / `A C API \`libpiper\` written in C++ is planned`.

### 1.6 Are any prebuilt binaries published?

Release assets contain **no shared library and no CLI binary for any platform**:

- `https://api.github.com/repos/rhasspy/piper/releases` — release `2023.11.14-2` assets are exactly:
  `piper_linux_aarch64.tar.gz`, `piper_linux_armv7l.tar.gz`, `piper_linux_x86_64.tar.gz`,
  `piper_macos_aarch64.tar.gz`, `piper_macos_x64.tar.gz`, `piper_windows_amd64.zip`.
  Release `v1.2.0` assets are exactly: `piper_amd64.tar.gz`, `piper_arm64.tar.gz`, `piper_armv7.tar.gz`
  (all Linux).
- `https://api.github.com/repos/OHF-Voice/piper1-gpl/releases` — e.g. `v1.8.0` assets are exactly five
  `piper_tts-1.8.0-cp39-abi3-*.whl` wheels (macosx x86_64, macosx arm64, manylinux aarch64, manylinux
  x86_64, win_amd64) plus `piper_tts-1.8.0.tar.gz`. `v1.5.0` assets follow the same pattern
  (`https://api.github.com/repos/OHF-Voice/piper1-gpl/releases/tags/v1.5.0`).

Therefore: **the official Piper project publishes no prebuilt `.so`/`.dll`/`.aar`/`.apk` for any
platform**; the only distributed binaries are Python wheels and sdists. The wheels do contain a
platform-specific Python extension module (`add_library(espeakbridge MODULE ...)` in the root
`CMakeLists.txt`), which is not a general-purpose `libpiper`.

Build-matrix evidence that no Android/CLI artifact is produced by upstream CI:
`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.github/workflows/build-libpiper.yml`
uses `os: [ubuntu-latest, macos-latest, windows-latest]`;
`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.github/workflows/wheels.yml` uses
`os: [ubuntu-latest, ubuntu-24.04-arm, windows-latest, macos-13, macos-latest]`.

---

## 2. The C++ embedding API

### 2.1 Legacy API — `src/cpp/piper.hpp` (C++ namespace, no stable C ABI)

Source: `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.hpp` (3226 bytes).
Everything is inside `namespace piper { ... }`; there is **no `extern "C"` and no export/visibility
macro**, so this is a C++-ABI surface.

Exact struct/field declarations:

```cpp
typedef int64_t SpeakerId;

struct eSpeakConfig {
  std::string voice = "en-us";
};

struct PiperConfig {
  std::string eSpeakDataPath;
  bool useESpeak = true;

  bool useTashkeel = false;
  std::optional<std::string> tashkeelModelPath;
  std::unique_ptr<tashkeel::State> tashkeelState;
};

enum PhonemeType { eSpeakPhonemes, TextPhonemes };

struct SynthesisConfig {
  float noiseScale = 0.667f;
  float lengthScale = 1.0f;
  float noiseW = 0.8f;
  int sampleRate = 22050;
  int sampleWidth = 2; // 16-bit
  int channels = 1;    // mono
  std::optional<SpeakerId> speakerId;
  float sentenceSilenceSeconds = 0.2f;
  std::optional<std::map<piper::Phoneme, float>> phonemeSilenceSeconds;
};

struct ModelSession {
  Ort::Session onnx;
  Ort::AllocatorWithDefaultOptions allocator;
  Ort::SessionOptions options;
  Ort::Env env;
  ModelSession() : onnx(nullptr){};
};

struct SynthesisResult {
  double inferSeconds;
  double audioSeconds;
  double realTimeFactor;
};

struct Voice {
  json configRoot;
  PhonemizeConfig phonemizeConfig;
  SynthesisConfig synthesisConfig;
  ModelConfig modelConfig;
  ModelSession session;
};
```

Exact function signatures (verbatim, comments included):

```cpp
// Must be called before using textTo* functions
void initialize(PiperConfig &config);

// Clean up
void terminate(PiperConfig &config);

// Load Onnx model and JSON config file
void loadVoice(PiperConfig &config, std::string modelPath,
               std::string modelConfigPath, Voice &voice,
               std::optional<SpeakerId> &speakerId, bool useCuda);

// Phonemize text and synthesize audio
void textToAudio(PiperConfig &config, Voice &voice, std::string text,
                 std::vector<int16_t> &audioBuffer, SynthesisResult &result,
                 const std::function<void()> &audioCallback);

// Phonemize text and synthesize audio to WAV file
void textToWavFile(PiperConfig &config, Voice &voice, std::string text,
                   std::ostream &audioFile, SynthesisResult &result);
```

plus `bool isSingleCodepoint(std::string s);`, `Phoneme getCodepoint(std::string s);`,
`std::string getVersion();`.

- Audio output type is **`std::vector<int16_t>`** (16-bit mono), per `textToAudio` and
  `SynthesisConfig::sampleWidth = 2`.
- Minimal usage example (calls `loadVoice` **before** `initialize`, then `textToWavFile`):
  `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/test.cpp`.

### 2.2 Current API — `libpiper/include/piper.h` (flat C API, export macro)

Source: `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h` (8451
bytes). It is guarded by `#ifdef __cplusplus extern "C" { #endif`, C-compatible (`char32_t` is
typedef'd to `uint32_t` for C), and defines `EXPORT_SYMBOL` as `__declspec(dllexport)`/`dllimport`
under Windows, empty elsewhere. `libpiper/CMakeLists.txt` adds
`target_compile_definitions(piper PRIVATE BUILDING_LIBPIPER)` to arm the dllexport path.

Error codes, verbatim:

```c
#define PIPER_OK (0)
#define PIPER_DONE (1)
#define PIPER_ERR_GENERIC (-1)
```

Opaque handle and the three public structs, verbatim field lists:

```c
typedef struct piper_synthesizer piper_synthesizer;
```

```c
typedef struct piper_audio_chunk {
  const float *samples;
  size_t num_samples;
  int sample_rate;
  bool is_last;
  const char32_t *phonemes;
  size_t num_phonemes;
  const int *phoneme_ids;
  size_t num_phoneme_ids;
  const int *alignments;
  size_t num_alignments;
} piper_audio_chunk;
```

```c
typedef struct piper_synthesize_options {
  int speaker_id;
  float length_scale;
  float noise_scale;
  float noise_w_scale;
} piper_synthesize_options;

typedef struct piper_create_options {
  size_t struct_size;
  const char *model_path;
  const char *config_path;
  const char *espeak_data_path;
  const char *g2pw_model_dir;
  const char *data_dir;
} piper_create_options;
```

Note: **audio output is `const float *samples`** (32-bit float), a change from the legacy `int16_t`
buffer. The struct is explicitly ABI-versioned: "This struct is versioned by struct_size. Set
struct_size = sizeof(piper_create_options) before passing to piper_create_with_options. Future fields
will be appended at the end and gated by struct_size."

Every public function, verbatim signature (each preceded by `EXPORT_SYMBOL`):

```c
piper_synthesizer *piper_create_with_options(const piper_create_options *options);
piper_synthesizer *piper_create(const char *model_path, const char *config_path,
                                const char *espeak_data_path);
void piper_free(piper_synthesizer *synth);
piper_synthesize_options piper_default_synthesize_options(piper_synthesizer *synth);
int piper_synthesize_start(piper_synthesizer *synth, const char *text,
                           const piper_synthesize_options *options);
int piper_synthesize_next(piper_synthesizer *synth, piper_audio_chunk *chunk);
char const *piper_version(void);
```

plus the static inline initializer:

```c
static inline void piper_init_create_options(piper_create_options *opts) { ... }
```

Streaming contract, quoted from the header: `piper_synthesize_start must be called before this
function. Each call to piper_synthesize_next will fill the audio chunk, invalidating the memory of the
previous chunk. The final audio chunk will have is_last = true. A return value of PIPER_DONE indicates
that synthesis is complete.`

`piper_create_options` is documented in-header as: `Path to espeak-ng data directory, or NULL if not
needed (text phonemes).` and `Optional root data directory that may contain espeak-ng-data/ and g2pw/.`

Internal (not-installed-by-default) header:
`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper_impl.hpp` — it
defines `struct piper_synthesizer`, `enum class PhonemeType { Invalid = 0, Text, Espeak, Pinyin };`,
and the constants `DEFAULT_LENGTH_SCALE = 1.0F`, `DEFAULT_NOISE_SCALE = 0.667F`,
`DEFAULT_NOISE_W_SCALE = 0.8F`, `DEFAULT_HOP_LENGTH = 256`, `ID_PAD = 0`, `ID_BOS = 1`, `ID_EOS = 2`.

**Latency-relevant limitation of the new C API:** `piper_audio_chunk` has **no timing fields** — there
is no `realTimeFactor`/`inferSeconds` equivalent. The legacy `SynthesisResult` exposed
`realTimeFactor`; the current public C API does not, so a caller must time chunks itself.

### 2.3 How phonemization is invoked from C++

**Legacy** — via `piper-phonemize` (linked as target `piper_phonemize`), which itself wraps espeak-ng.
Exact strings from `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.cpp`:

```cpp
void initialize(PiperConfig &config) {
  if (config.useESpeak) {
    // Set up espeak-ng for calling espeak_TextToPhonemesWithTerminator
    // See: https://github.com/rhasspy/espeak-ng
    spdlog::debug("Initializing eSpeak");
    int result = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS,
                                   /*buflength*/ 0,
                                   /*path*/ config.eSpeakDataPath.c_str(),
                                   /*options*/ 0);
    if (result < 0) {
      throw std::runtime_error("Failed to initialize eSpeak-ng");
    }
```

```cpp
void terminate(PiperConfig &config) {
  if (config.useESpeak) {
    espeak_Terminate();
```

```cpp
  if (voice.phonemizeConfig.phonemeType == eSpeakPhonemes) {
    // Use espeak-ng for phonemization
    eSpeakPhonemeConfig eSpeakConfig;
    eSpeakConfig.voice = voice.phonemizeConfig.eSpeak.voice;
    phonemize_eSpeak(text, eSpeakConfig, phonemes);
  } else {
    // Use UTF-8 codepoints as "phonemes"
    CodepointsPhonemeConfig codepointsConfig;
    phonemize_codepoints(text, codepointsConfig, phonemes);
  }
```

`piper.hpp` gets `phonemize_eSpeak` / `phoneme_ids` from
`#include <piper-phonemize/phonemize.hpp>` (and `phoneme_ids.hpp`, `tashkeel.hpp`). So the legacy path
is: **`espeak_Initialize` called directly, but text→phoneme conversion delegated to the
`piper-phonemize` library** (`piper_phonemize`).

**Current** — espeak-ng is called **directly**; `piper-phonemize` is gone. Exact strings from
`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/piper.cpp`:

```cpp
#include <espeak-ng/speak_lib.h>
```

```cpp
  if (phoneme_type == PhonemeType::Espeak &&
      espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, final_espeak_data_path,
                        0) < 0) {
    return nullptr;
  }
```

```cpp
  if (synth->phoneme_type == PhonemeType::Espeak &&
      espeak_SetVoiceByName(synth->espeak_voice.c_str()) != EE_OK) {
    return PIPER_ERR_GENERIC;
  }
```

```cpp
      const char *phonemes = espeak_TextToPhonemesWithTerminator(
          &text_ptr, espeakCHARS_AUTO, espeakPHONEMES_IPA, &terminator);
```

```cpp
  if (synth->phoneme_type == PhonemeType::Espeak) {
    espeak_Terminate();
  }
```

Dispatched by `phoneme_type` from the voice config JSON:
`PhonemeType::Espeak` (espeak-ng, above), `PhonemeType::Text` (Unicode NFD lowercase via `uni_algo`,
no espeak at all), `PhonemeType::Pinyin` (a `piper::ChinesePhonemizer` + g2pw dictionaries, no espeak).
Rationale for taking upstream espeak-ng, quoted from
`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/BUILDING.md`: `We build upstream
[espeak-ng][] since they added the \`espeak_TextToPhonemesWithTerminator\` that Piper depends on.`

The current public README confirms the removal of the middle layer
(`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/README.md`):
`A fast and local neural text-to-speech engine that embeds [espeak-ng][] for phonemization.`
and CHANGELOG `## 1.3.0`: `Embed espeak-ng directly instead of using separate \`piper-phonemize\` library`.

Third-party corroboration that sherpa-onnx does *not* use this phonemizer:
`https://api.github.com/repos/rhasspy/piper/issues/251` — `Note that it does not depend on
https://github.com/rhasspy/piper-phonemize`.

---

## 3. Does the C++ API need `espeak-ng-data` bundled?

### 3.1 Legacy: yes, mandatory for espeak voices, located next to the executable

- API surface: `PiperConfig::eSpeakDataPath` (`std::string eSpeakDataPath;`) in
  `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.hpp`.
- Runtime default resolution, exact strings from
  `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/main.cpp`:
  - comment on the config field: `// Path to espeak-ng data directory (default is next to piper executable)`
  - CLI flag: `--espeak_data           DIR   path to espeak-ng data directory`
  - default computation, reusing the executable's own path
    (`filesystem::canonical("/proc/self/exe")` on Linux, `_NSGetExecutablePath` on Apple,
    `GetModuleFileNameW` on MSVC):
    ```cpp
      piperConfig.eSpeakDataPath =
          std::filesystem::absolute(
              exePath.parent_path().append("espeak-ng-data"))
              .string();

      spdlog::debug("espeak-ng-data directory is expected at {}",
                    piperConfig.eSpeakDataPath);
    ```
  - it is only required when the voice uses espeak phonemes:
    `if (voice.phonemizeConfig.phonemeType == piper::eSpeakPhonemes) { ... } else { // Not using eSpeak \n piperConfig.useESpeak = false; }`
- Packaging: `https://raw.githubusercontent.com/rhasspy/piper/master/CMakeLists.txt` installs it to
  the **install prefix root**:
  ```
  install(
    DIRECTORY ${PIPER_PHONEMIZE_DIR}/share/espeak-ng-data
    DESTINATION ${CMAKE_INSTALL_PREFIX}
  )
  ```
  i.e. `<prefix>/espeak-ng-data`. The test fixture path is the same data dir:
  `add_test(NAME test_piper COMMAND test_piper "${CMAKE_SOURCE_DIR}/etc/test_voice.onnx" "${PIPER_PHONEMIZE_DIR}/share/espeak-ng-data" ...)`.
- Independent confirmation that the data dir is a hard build-time dependency of the legacy pipeline:
  `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/test.cpp` exits with
  `std::cerr << "Need espeak-ng-data path" << std::endl;` if argv[2] is missing.

### 3.2 Current repo: yes, passed explicitly by the embedding app

- `libpiper/README.md` (`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md`)
  lists it as one of the four things an integrator must supply, verbatim:
  ```
  To use `libpiper`, you will need to:

  * Include `piper.h` (`install/include/`)
  * Link to the `libpiper` library (`install/`)
  * Link to the `libonnxruntime` library (`install/lib/`)
  * Provide `piper_create` with the path to espeak-ng's data (`install/espeak-ng-data/`)
  ```
- **Documentation/CMake discrepancy (reported as observed):** the README says `install/espeak-ng-data/`,
  but `libpiper/CMakeLists.txt` installs to the *data root*:
  ```
  set(ESPEAKNG_DATA_SRC ${CMAKE_BINARY_DIR}/espeak_ng-install/share/espeak-ng-data)

  install(
      DIRECTORY ${ESPEAKNG_DATA_SRC}
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}
  )
  ```
  With `CMAKE_INSTALL_DATAROOTDIR` defaulting to `share`, the actual installed path is
  `<prefix>/share/espeak-ng-data`, not `<prefix>/espeak-ng-data`.
- Runtime default when embedding the bundled CLI: `<directory of the piper executable>/espeak-ng-data`,
  from `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/main.cpp`:
  ```
    // Get the path to the piper executable so we can locate espeak-ng-data,
    // etc. next to it.
  ...
      runConfig.eSpeakDataPath =
          std::filesystem::absolute(
              exePath.parent_path().append("espeak-ng-data"))
              .string();
    }
    if (!runConfig.eSpeakDataPath.has_value()) {
      throw std::runtime_error("eSpeak data path not set");
    }
  ```
  with CLI flags documented in `libpiper/src/main/utils/main_utils.cpp`:
  `--espeak_data           DIR   path to espeak-ng data directory` and
  `--data_dir            DIR   base data dir (looks for espeak-ng-data and g2pw subdirs)`.
- Library-side fallback when only `data_dir` is set, from `libpiper/src/piper.cpp`:
  ```cpp
    if (!final_espeak_data_path && data_dir_opt) {
      std::string cand = std::string(data_dir_opt) + "/espeak-ng-data";
      resolved_espeak_data = cand;
      final_espeak_data_path = resolved_espeak_data.c_str();
    }
  ```
- Same for the Python module build — the root `CMakeLists.txt` copies the generated data into the
  package source tree at build time:
  ```
  set(DATA_DST ${CMAKE_CURRENT_SOURCE_DIR}/src/piper/espeak-ng-data)
  add_custom_target(copy_espeak_ng_data ALL
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${DATA_SRC} ${DATA_DST}
      DEPENDS espeak_ng_external
      COMMENT "Copying espeak-ng-data after espeak-ng external project builds"
  )
  ```

### 3.3 `PIPER_ESPEAKNG_DATA_DIRECTORY` and `ESPEAK_DATA_PATH`

- `PIPER_ESPEAKNG_DATA_DIRECTORY`: **not present in any source file inspected in this research.**
  The files examined were `rhasspy/piper` `src/cpp/piper.hpp`, `src/cpp/piper.cpp`, `src/cpp/main.cpp`,
  `src/cpp/test.cpp`, root `CMakeLists.txt`, `Makefile`, master `README.md`, and the tagged READMEs;
  and `OHF-Voice/piper1-gpl` `libpiper/CMakeLists.txt`, `libpiper/src/piper.cpp`,
  `libpiper/src/main/main.cpp`, `libpiper/src/main/utils/main_utils.cpp`,
  `libpiper/include/piper.h`, `libpiper/include/piper_impl.hpp`, `libpiper/README.md`, root
  `CMakeLists.txt`, root `README.md`, `docs/BUILDING.md`, `docs/VOICES.md`, `CHANGELOG.md`,
  `.gitignore`, `.github/workflows/build-libpiper.yml`, `.github/workflows/wheels.yml`,
  and the full `git/trees/main?recursive=1` listing. No identifier by that name appears. A whole-repo
  text grep across all branches/history was **not** performed, so an absolute claim is
  **not determinable from public sources** on this pass; within the inspected surface it does not exist.
- `ESPEAK_DATA_PATH`: not present in the official Piper sources inspected. It appears in a **third-party**
  Termux guide as an environment variable the third-party CLI passes to `piper_create`:
  `https://raw.githubusercontent.com/gyroing/piper-tts-for-termux/main/README.md`
  (`ESPEAK_DATA_PATH | Path to espeak-ng-data folder optional`, and
  `piper_synthesizer *synth = piper_create(onnx_path.c_str(), json_path.c_str(), espeak_path);`
  where `espeak_path = std::getenv("ESPEAK_DATA_PATH")`).
- The official install-time data directory referenced in the piper1-gpl wheels workflow is the same
  generated `espeak-ng-data` tree (`copy_espeak_ng_data` above).

### 3.4 Size of `espeak-ng-data`

- **Not published by the Piper project.** No byte size, file count or archive size for
  `espeak-ng-data` appears in `rhasspy/piper` or `OHF-Voice/piper1-gpl` READMEs, docs, CHANGELOG, or
  CMake files inspected (URL list in §3.3). The value is **not published**.
- Size is **not derivable from the published Piper binaries either**: the piper1-gpl wheels bundle the
  data *together with* ONNX Runtime in one archive — e.g. `piper_tts-1.8.0-cp39-abi3-macosx_10_9_x86_64.whl`
  = 34,111,822 bytes and `...manylinux_2_17_aarch64...whl` = 34,131,751 bytes
  (`https://api.github.com/repos/OHF-Voice/piper1-gpl/releases`) — so no per-component figure can be
  extracted from them.
- **piper1-gpl's source tree contains no `espeak-ng-data` directory.** The full recursive tree listing
  at `https://api.github.com/repos/OHF-Voice/piper1-gpl/git/trees/main?recursive=1` returns
  `"truncated":false` and contains **no path matching `espeak-ng-data`** — the number of files in
  `espeak-ng-data` within the checked-in source tree is **0**. It is generated/copied at build time
  (§3.2) and is explicitly ignored:
  `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.gitignore` contains the line
  `espeak-ng-data/`.
- Reference size from a downstream packager of the same data (not a Piper-published figure):
  Debian's `espeak-ng-data` package version `1.52.0+dfsg-5` —
  `https://packages.debian.org/stable/sound/espeak-ng-data` reports **Package Size 8,450.7 kB** and
  **Installed Size 24,034.0 kB**, identical for `amd64` and `arm64`. The file inventory is at
  `https://packages.debian.org/trixie/amd64/espeak-ng-data/filelist`. Caveat: Debian's set is built
  from espeak-ng 1.52.0 with Debian's own options, whereas Piper builds espeak-ng from a pinned git
  commit with a reduced feature set — `libpiper/CMakeLists.txt` passes
  `-DBUILD_SHARED_LIBS:BOOL=OFF -DUSE_ASYNC:BOOL=OFF -DUSE_MBROLA:BOOL=OFF -DUSE_LIBSONIC:BOOL=OFF
  -DUSE_LIBPCAUDIO:BOOL=OFF -DUSE_KLATT:BOOL=OFF -DUSE_SPEECHPLAYER:BOOL=OFF -DEXTRA_cmn:BOOL=ON
  -DEXTRA_ru:BOOL=ON` — so the Debian figures are an indicative reference, **not** the size of Piper's
  own data directory.
- The legacy `piper-phonemize` bundles that shipped `espeak-ng-data` for the legacy build
  (`https://github.com/rhasspy/piper-phonemize`) could not be measured: the releases API call
  `https://api.github.com/repos/rhasspy/piper-phonemize/releases` returned HTTP 403 (rate limit).
  Published bundle sizes for that repo are therefore **not determinable from public sources** in this pass.

### 3.5 Which espeak-ng commit each project pins (relevant to data content)

Two different pins exist in the same active repo:

- `libpiper/CMakeLists.txt`: `GIT_TAG 212928b394a96e8fd2096616bfd54e17845c48f6  # 2025-Mar-22`
- root `CMakeLists.txt` (Python module): `GIT_TAG 724808c  # 2026-Apr-06`

---

## 4. Is there a prebuilt Android arm64 library or built-in Android support?

### 4.1 Official Android support: NONE

Evidence checked, in order:

1. **Legacy release assets contain no Android artifact.** `https://api.github.com/repos/rhasspy/piper/releases`
   — release `2023.11.14-2`: `piper_linux_aarch64.tar.gz`, `piper_linux_armv7l.tar.gz`,
   `piper_linux_x86_64.tar.gz`, `piper_macos_aarch64.tar.gz`, `piper_macos_x64.tar.gz`,
   `piper_windows_amd64.zip`. Release `v1.2.0`: `piper_amd64.tar.gz`, `piper_arm64.tar.gz`,
   `piper_armv7.tar.gz`. No `.so`, no `.aar`, no `.apk`, no Android target.
2. **Current release assets contain no Android artifact.** `https://api.github.com/repos/OHF-Voice/piper1-gpl/releases`
   — every release (v1.8.0, v1.7.0, v1.4.2, v1.4.1, v1.4.0, v1.3.0) ships only Python wheels for
   macOS/Linux/Windows plus a `.tar.gz` sdist.
3. **No Android branch in the build system.** `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt`
   handles only `WIN32`, `APPLE`, and (Linux) `x86_64` / `aarch64` / `armv7l`, else
   `message(FATAL_ERROR "Unsupported architecture for onnxruntime")`. There is no `ANDROID` variable
   check, no `ANDROID_ABI`/`ANDROID_PLATFORM` handling, and no `-DCMAKE_TOOLCHAIN_FILE=.../android.toolchain.cmake`
   anywhere in the file.
4. **No Android CI.** `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.github/workflows/build-libpiper.yml`
   → `os: [ubuntu-latest, macos-latest, windows-latest]`;
   `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.github/workflows/wheels.yml` →
   `os: [ubuntu-latest, ubuntu-24.04-arm, windows-latest, macos-13, macos-latest]`. No Android runner,
   no NDK setup step.
5. **Issue trackers show requests, not implementations.**
   - `https://api.github.com/search/issues?q=repo:rhasspy/piper+android&per_page=50` → `"total_count":16`.
     Notable items: `https://api.github.com/repos/rhasspy/piper/issues/103` — `[Request] Please make it
     run on Android` (state `open`, 8 comments); `https://api.github.com/repos/rhasspy/piper/issues/180`
     — `Piper on termux android` (state `open`, 18 comments, body shows a build failure
     `ld.lld: error: unable to find library -lc++_shared` on `Android ... aarch64`);
     `https://api.github.com/repos/rhasspy/piper/issues/370` — `The ONNX multi-speaker model cannot
     work properly in an Android app` (state `closed`).
   - `https://api.github.com/search/issues?q=repo:OHF-Voice/piper1-gpl+android&per_page=50` → `"total_count":6`.
     Notable items: `https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/130` — `How to build and
     integrate into an Android Studio project?` (state `open`, `comments: 0` as of the API response);
     `https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/7` — `New build - first impressions on
     the install process on Android` (state `open`, 20 comments; body records Termux build failures
     including `ld.lld: error: unable to find library -lgcc` and a `phondata`/`phsource` compile error);
     `https://api.github.com/repos/OHF-Voice/piper1-gpl/pull/21` — `feat: Termux build improvements and
     unified CMakeLists.txt`, state `open`, `"merged_at":null` (i.e. **unmerged**);
     `https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/78` — `Implementing piper for mobile phone`
     (state `open`).
   - **Conclusion stated explicitly: no official Android build exists** for either repo, and no
     official Android/NDK build support is implemented.
6. **`repo:rhasspy/piper+termux` search: not obtained.** `https://api.github.com/search/issues?q=repo:rhasspy/piper+termux&per_page=50`
   returned HTTP 403 (rate limit) and was not retried successfully. Termux-related items surfaced only
   indirectly, via the `android` search (`issues/180`, `issues/525`). A dedicated termux-keyword count
   is **not determinable from public sources** in this pass.

### 4.2 Termux packaging

- **`piper` is not packaged in `termux-packages`.** Both plausible package paths 404:
  `https://raw.githubusercontent.com/termux/termux-packages/master/packages/piper/build.sh` → HTTP 404,
  `https://raw.githubusercontent.com/termux/termux-packages/master/packages/piper-tts/build.sh` → HTTP 404.
- The URL pattern is verified correct by a working control file:
  `https://raw.githubusercontent.com/termux/termux-packages/master/packages/espeak/build.sh` → HTTP 200,
  containing `TERMUX_PKG_HOMEPAGE=https://github.com/espeak-ng/espeak-ng`, `TERMUX_PKG_VERSION="1.52.0"`,
  `TERMUX_PKG_LICENSE="GPL-2.0"`, `# Use eSpeak NG as the original eSpeak project is dead.`
  So Termux packages **espeak-ng** (under the package name `espeak`) but not Piper.
- A search of the whole `termux-packages` package index was not performed (only these exact paths were
  probed), so a differently-named package cannot be fully excluded — but no `piper` package directory
  exists at the canonical path.

### 4.3 THIRD-PARTY Android builds (clearly distinguished from official)

1. **sherpa-onnx / Next-gen Kaldi (`k2-fsa`)** — a **separate engine** that re-implements Piper-model
   inference; it does **not** use Piper's code. Evidence:
   - `https://k2-fsa.github.io/sherpa/onnx/tts/apk.html` documents Android TTS APKs with the naming rule
     `sherpa-onnx-{version}-{arch}-{lang}-tts-{model}.apk` and states verbatim:
     `Note: Models from [piper](https://github.com/rhasspy/piper) have their names prefixed with **vits-piper-**.`
   - It publishes **arm64-v8a Mandarin Piper APKs**, which I verified in the page's link list:
     `.../tts-new/1.13.7/sherpa-onnx-1.13.7-arm64-v8a-zho-tts-vits-piper-zh_CN-huayan-medium.apk`,
     `...-zho-tts-vits-piper-zh_CN-chaowen-medium.apk`, `...-zho-tts-vits-piper-zh_CN-xiao_ya-medium.apk`,
     all under `https://huggingface.co/csukuangfj2/sherpa-onnx-apk/resolve/main/`.
   - The APK source is at
     `https://github.com/k2-fsa/sherpa-onnx/tree/master/android/SherpaOnnxTts` (linked from the same page).
   - Independence from Piper's phonemizer: `https://api.github.com/repos/rhasspy/piper/issues/251`
     (`FYI: Run models from piper with the Next-gen Kaldi subproject sherpa-onnx`) states
     `Note that it does not depend on https://github.com/rhasspy/piper-phonemize`.
   - `https://api.github.com/repos/rhasspy/piper/issues/257` (`FYI: Download links about Android APKs
     for piper models`, opened by a `CONTRIBUTOR`) announces these and at the time listed support for
     `English / French / Spanish / German`, pointing to `https://k2-fsa.github.io/sherpa/onnx/tts/apk.html`.
2. **`nihui/ncnn-android-piper`** — third-party Android sample using **Tencent ncnn**, not ONNX Runtime.
   `https://raw.githubusercontent.com/nihui/ncnn-android-piper/master/README.md` states verbatim:
   `This is a sample ncnn android project, it depends on ncnn library`, and
   `This project uses a custom dictionary to implement phonemizer` plus
   `If you need an espeak-ng phonemizer implementation(GPL), refer to https://github.com/nihui/ncnn-android-piper/issues/2`.
   It ships an APK (`## android apk file download` / `https://github.com/nihui/ncnn-android-piper/releases/latest`)
   and documents converting piper checkpoints to ncnn via `pnnx`.
3. **`gyroing/piper-tts-for-termux`** — third-party Termux build guide for the **current**
   `piper1-gpl` `libpiper`, not an official artifact.
   `https://raw.githubusercontent.com/gyroing/piper-tts-for-termux/main/README.md` states verbatim:
   `🛠️ How to Build libpiper.so and a Native piper CLI Binary in Termux (Android)`, clones
   `https://github.com/OHF-Voice/piper1-gpl.git` and builds `libpiper`, then instructs to fetch
   Microsoft's Android ONNX Runtime AAR and patch the SONAME:
   `unzip onnxruntime-android-1.22.0.aar` → `jni/arm64-v8a/libonnxruntime.so`, and
   `patchelf --replace-needed libonnxruntime.so.1 libonnxruntime.so install/lib/libpiper.so`.
   It also distributes a `piper-tts-cli` `.deb` for Termux. This confirms a **third party has produced
   `libpiper.so` for Android arm64 by hand**, not that upstream supports it.
4. **`jvoice-project/piper-jni`** — the Java/JNI binding that the **official** piper1-gpl README lists
   under `Bindings to use Piper in programming languages other than Python and C/C++:`
   (`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/README.md`, entry
   `* Java: [Piper JNI](https://github.com/jvoice-project/piper-jni)`).
   Its own README explicitly does **not** list Android:
   `https://raw.githubusercontent.com/jvoice-project/piper-jni/main/README.md` — `Java >= 17 is supported.`
   with platform support `Windows x86_64`, `Linux x86_64/arm64 (built with Ubuntu Focal Fossa, GLIBC version 2.31)`,
   `macOS x86_64/arm64 (built for macOS 14 Sonoma and newer)`. The JAR is distributed via Maven Central
   (`io.github.jvoice-project:piper-jni`).
5. **`VoxSherpa TTS`** — a third-party offline Android TTS app using Piper models via sherpa-onnx.
   Referenced in `https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/224` (a PR proposing to add it
   to the README's project list; that PR has `"merged_at":null`, i.e. closed without merge, so it is
   **not** in the README as fetched). The PR body describes it as
   `an offline Android TTS application that utilizes the amazing Piper engine via Sherpa-ONNX`.

### 4.4 Does the official project publish a `.so` for *any* platform?

**No prebuilt shared library is published.** The `libpiper` target is declared `SHARED` in source
(`add_library(piper SHARED ...)`) and `install(TARGETS piper DESTINATION ${CMAKE_INSTALL_LIBDIR})` is
declared, but no release asset in either repo is a `.so`/`.dll`/`.dylib`; the release asset lists in
§1.6 and §4.1 contain only tarballs/zips containing executables (legacy) and Python wheels (current).
The CI does upload a `libpiper-${{ matrix.os }}` **build artifact** of `libpiper/install/`
(`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.github/workflows/build-libpiper.yml`),
but that is a workflow artifact for `ubuntu-latest`/`macos-latest`/`windows-latest`, is not a release
asset, and includes no Android variant.

### 4.5 Microsoft's ONNX Runtime Android AAR (relevant context, not a Piper artifact)

- Exists and is published by Microsoft on Maven Central:
  `https://repo1.maven.org/maven2/com/microsoft/onnxruntime/onnxruntime-android/1.22.0/` lists
  `onnxruntime-android-1.22.0.aar` at **28,515,295 bytes**, dated `2025-05-09 23:10`, alongside its
  `.asc`/`.sha256` signatures and `onnxruntime-android-1.22.0.pom`. Version 1.22.0 matches the default
  `ONNXRUNTIME_VERSION` pinned by `libpiper/CMakeLists.txt`.

---

## 5. Published RTF / latency numbers

### 5.1 The premise "the legacy README has a performance/speed section" is not supported

`https://raw.githubusercontent.com/rhasspy/piper/master/README.md` is a **63-byte redirect stub** whose
entire content is `Development has moved: https://github.com/OHF-Voice/piper1-gpl`. It has **no**
performance section. The last tagged legacy READMEs were checked instead:
`https://raw.githubusercontent.com/rhasspy/piper/v1.0.0/README.md`,
`https://raw.githubusercontent.com/rhasspy/piper/v1.2.0/README.md`,
`https://raw.githubusercontent.com/rhasspy/piper/2023.11.14-2/README.md`.

### 5.2 Every published speed statement, quoted exactly

The **only** speed claims published by the Piper project are qualitative. Verbatim:

1. Legacy READMEs (identical opening sentence in v1.0.0, v1.2.0 and 2023.11.14-2):
   > `A fast, local neural text to speech system that sounds great and is optimized for the Raspberry Pi 4.`

   The only hardware named anywhere is **"Raspberry Pi 4"**. No RTF, no seconds, no CPU model.
   Sources: `https://raw.githubusercontent.com/rhasspy/piper/v1.0.0/README.md`,
   `https://raw.githubusercontent.com/rhasspy/piper/v1.2.0/README.md`,
   `https://raw.githubusercontent.com/rhasspy/piper/2023.11.14-2/README.md`.
2. Current README (`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/README.md`):
   > `A fast and local neural text-to-speech engine that embeds [espeak-ng][] for phonemization.`

   No number, no hardware named.
3. Legacy README v1.0.0 also states onnxruntime versioning:
   > `Last tested with [onnxruntime](https://github.com/microsoft/onnxruntime) 1.14.1.`

   (v1.0.0 only; not present in v1.2.0 or 2023.11.14-2.)

### 5.3 RTF for medium / low / x_low voices: not published

- **No real-time-factor figure for any voice quality is published** in either repo's README, docs,
  CHANGELOG, or CMake files that were inspected. The value is **not published**.
- `https://rhasspy.github.io/piper-samples/` was fetched and contains **no RTF column**. Its table
  header is `Language Voice Quality Speaker`, and its only published numbers are model sizes:
  > `x_low - 16Khz audio, 5-7M params`
  > `low - 16Khz audio, 15-20M params`
  > `medium - 22.05Khz audio, 15-20M params`
  > `high - 22.05Khz audio, 28-32M params`
  plus `Multi-speaker models can quickly switch between different speakers, but the quality of an
  individual speaker may be less than a single speaker model.`
- Per-quality RTF for a named CPU model: **not published**.

### 5.4 RTF exists only as a runtime-measured value, and only in the legacy API

- Legacy, computed in `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.cpp`:
  ```cpp
  result.audioSeconds = (double)audioCount / (double)synthesisConfig.sampleRate;
  result.realTimeFactor = 0.0;
  if (result.audioSeconds > 0) {
    result.realTimeFactor = result.inferSeconds / result.audioSeconds;
  }
  ```
  and at the end of `textToAudio`: `if (result.audioSeconds > 0) { result.realTimeFactor = result.inferSeconds / result.audioSeconds; }`
- Legacy CLI log format, exact string from
  `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/main.cpp`:
  ```cpp
    spdlog::info("Real-time factor: {} (infer={} sec, audio={} sec)",
                 result.realTimeFactor, result.inferSeconds,
                 result.audioSeconds);
  ```
- **The current C API has no RTF field at all.** `piper_audio_chunk` in
  `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h` exposes only
  `samples`, `num_samples`, `sample_rate`, `is_last`, `phonemes`, `phoneme_ids`, `alignments` — no
  timing members. `piper_synthesize_next` returns `PIPER_DONE`/`PIPER_OK`/error only. An embedder must
  measure latency itself.

### 5.5 Published ONNX Runtime / quantization details for Piper voices

- Voice training/export target, verbatim from `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/VOICES.md`
  (and identically in the legacy READMEs):
  > `Voices are trained with [VITS](https://github.com/jaywalnut310/vits/) and exported to the [onnxruntime](https://onnxruntime.ai/).`
- **No published quantization scheme (int8/fp16/dynamic-range) for Piper voice `.onnx` files was found.**
  The word "quantized" appears in the CHANGELOG only for the *Chinese g2pW text-processing model*, not
  for Piper voices — `https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md`, `## 1.4.0`:
  > `- Add Chinese phonemizer based on [g2pW](https://github.com/GitYCC/g2pW/)`
  > `    - Using a quantized version of the original model with \`quantize_dynamic\``
  So ONNX Runtime precision/quantization details for Piper **voices** are **not published**.
- Runtime session options actually used (latency-relevant, primary source). `libpiper/src/piper.cpp`
  (`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/piper.cpp`):
  ```cpp
  synth->session_options.DisableCpuMemArena();
  synth->session_options.DisableMemPattern();
  synth->session_options.DisableProfiling();
  synth->session_options.SetIntraOpNumThreads(1);
  synth->session_options.SetInterOpNumThreads(1);
  synth->session_options.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_ENABLE_BASIC);
  synth->session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
  ```
  Legacy equivalent, with the maintainers' own comments (verbatim, including the relative claims, which
  are **not quantified** in any published source):
  ```cpp
  // Slows down performance by ~2x
  // session.options.SetIntraOpNumThreads(1);

  // Roughly doubles load time for no visible inference benefit
  // session.options.SetGraphOptimizationLevel(
  //     GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

  session.options.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_DISABLE_ALL);

  // Slows down performance very slightly
  // session.options.SetExecutionMode(ExecutionMode::ORT_PARALLEL);
  ```
- Default ONNX Runtime version pinned by the current C++ build: `1.22.0`
  (`https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt`).

### 5.6 Published model file sizes for the Mandarin (zh_CN) voices (primary: HuggingFace API via mirror)

Sizes are exact byte counts from the repository API:

- `zh/zh_CN/huayan/x_low/zh_CN-huayan-x_low.onnx` = **20,628,813 bytes** —
  `https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/x_low`
- `zh/zh_CN/huayan/medium/zh_CN-huayan-medium.onnx` = **63,201,294 bytes** —
  `https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/medium`
- There is **no `low` quality for `huayan`**: `https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan`
  lists only `medium` and `x_low`; and
  `https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/low` returns
  `{"error":"zh/zh_CN/huayan/low does not exist on \"main\""}`.
- The `zh_CN` language directory has three speakers:
  `https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN` lists `chaowen`, `huayan`,
  `xiao_ya`. (Sizes for `chaowen`/`xiao_ya` were not fetched → **not published in this report**.)
- `huayan/medium` MODEL_CARD, verbatim (`https://hf-mirror.com/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/medium/MODEL_CARD`):
  > `* Language: zh_CN (Chinese, China)`
  > `* Speakers: 1`
  > `* Quality: medium`
  > `* Samplerate: 22,050Hz`
  > `* Training: Finetuned from U.S. English lessac voice (medium quality).`
  > `* License: Unknown` (for the training dataset `https://github.com/PlayVoice/HuaYan_TTS`)
- Latency/RTF numbers for any zh_CN voice: **not published**.

---

## Complete list of URLs used

### GitHub REST API
- https://api.github.com/repos/rhasspy/piper/contents/
- https://api.github.com/repos/rhasspy/piper/contents/src/cpp
- https://api.github.com/repos/rhasspy/piper/releases
- https://api.github.com/repos/OHF-Voice/piper1-gpl/git/trees/main?recursive=1
- https://api.github.com/repos/OHF-Voice/piper1-gpl/releases
- https://api.github.com/repos/OHF-Voice/piper1-gpl/releases/tags/v1.5.0
- https://api.github.com/search/issues?q=repo:rhasspy/piper+android&per_page=50
- https://api.github.com/search/issues?q=repo:OHF-Voice/piper1-gpl+android&per_page=50
- https://api.github.com/repos/rhasspy/piper/issues/103
- https://api.github.com/repos/rhasspy/piper/issues/180
- https://api.github.com/repos/rhasspy/piper/issues/251
- https://api.github.com/repos/rhasspy/piper/issues/257
- https://api.github.com/repos/rhasspy/piper/issues/370
- https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/7
- https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/78
- https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/130
- https://api.github.com/repos/OHF-Voice/piper1-gpl/pull/21
- https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/224

### Raw source files
- https://raw.githubusercontent.com/rhasspy/piper/master/README.md
- https://raw.githubusercontent.com/rhasspy/piper/master/CMakeLists.txt
- https://raw.githubusercontent.com/rhasspy/piper/master/Makefile
- https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.hpp
- https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.cpp
- https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/main.cpp
- https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/test.cpp
- https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/CMakeLists.txt *(404 — does not exist)*
- https://raw.githubusercontent.com/rhasspy/piper/v1.0.0/README.md
- https://raw.githubusercontent.com/rhasspy/piper/v1.2.0/README.md
- https://raw.githubusercontent.com/rhasspy/piper/2023.11.14-2/README.md
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/README.md
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CMakeLists.txt
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.gitignore
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/BUILDING.md
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/VOICES.md
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper_impl.hpp
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/piper.cpp
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/CMakeLists.txt
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/main.cpp
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/utils/main_utils.cpp
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.github/workflows/build-libpiper.yml
- https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.github/workflows/wheels.yml
- https://raw.githubusercontent.com/termux/termux-packages/master/packages/piper/build.sh *(404)*
- https://raw.githubusercontent.com/termux/termux-packages/master/packages/piper-tts/build.sh *(404)*
- https://raw.githubusercontent.com/termux/termux-packages/master/packages/espeak/build.sh *(200 — control)*
- https://raw.githubusercontent.com/termux/termux-packages/master/packages/espeak-ng/build.sh *(404)*
- https://raw.githubusercontent.com/gyroing/piper-tts-for-termux/main/README.md
- https://raw.githubusercontent.com/nihui/ncnn-android-piper/master/README.md
- https://raw.githubusercontent.com/jvoice-project/piper-jni/main/README.md

### Project / documentation sites
- https://rhasspy.github.io/piper-samples/
- https://k2-fsa.github.io/sherpa/onnx/tts/apk.html
- https://repo1.maven.org/maven2/com/microsoft/onnxruntime/onnxruntime-android/1.22.0/
- https://packages.debian.org/stable/sound/espeak-ng-data
- https://packages.debian.org/trixie/amd64/espeak-ng-data/filelist

### Hugging Face (via hf-mirror.com — direct huggingface.co fetches fail in this environment)
- https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN
- https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan
- https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/x_low
- https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/medium
- https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/low *(404 — does not exist)*
- https://hf-mirror.com/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/medium/MODEL_CARD
- https://huggingface.co/csukuangfj2/sherpa-onnx-apk/resolve/main/tts-new/1.13.7/ (APK host prefix, cited from the sherpa page)

### URLs that failed or were rate-limited (content not obtained)
- https://api.github.com/search/issues?q=repo:rhasspy/piper+termux&per_page=50 *(HTTP 403 — rate limit)*
- https://api.github.com/repos/rhasspy/piper-phonemize/releases *(HTTP 403 — rate limit)*
- https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/48/comments *(HTTP 403 — rate limit)*
- https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/130/comments *(HTTP 403 — rate limit)*
- https://raw.githubusercontent.com/rhasspy/piper-samples/master/README.md *(transport error)*
