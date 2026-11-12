# Offline / on-device Mandarin TTS for a Qt 6.11.1 Android app (arm64-v8a, API 28+) — factual comparison

**Target context:** Qt 6.11.1, C++17, CMake, Android 9+ / API 28, ABI `arm64-v8a`.
App currently contains **zero** Java/JNI and no gradle files, already vendors
whisper.cpp 1.9.1 + ggml 0.16.0, and links prebuilt OpenSSL for Android.

**Method.** Facts only. Every number carries the URL of the source that owns it.
Where the owning project does not publish a number, the entry reads
**not published** — nothing here is inferred from adjacent projects.
Two environment limits are recorded because they bound what is knowable:
(a) direct `huggingface.co` fetches failed from this machine (transport error),
so Hugging Face cards and byte-exact repo trees were read through the
`hf-mirror.com` mirror of identical paths and are cited with canonical
`huggingface.co` URLs; (b) `developer.android.com`, `source.android.com`,
`support.google.com`, `android.googlesource.com` and `play.google.com` are
unreachable, so Android API levels were established by **diffing AOSP source
tags** via `cdn.jsdelivr.net/gh/aosp-mirror/...` instead of reading
"Added in API level N" annotations.

---

## 1. sherpa-onnx (k2-fsa)

### Runtime dependency
- Inference runtime: **ONNX Runtime**. sherpa-onnx is the C++ wrapper; it does not
  vendor ONNX Runtime, it downloads it. `build-android-arm64-v8a.sh` defaults to
  `SHERPA_ONNX_ONNXRT_VERSION=1.28.2` and fetches
  `https://github.com/csukuangfj/onnxruntime-libs/releases/download/v1.28.2/onnxruntime-android-1.28.2.zip`
  — https://github.com/k2-fsa/sherpa-onnx/blob/master/build-android-arm64-v8a.sh
- Published size of that prebuilt Android ONNX Runtime archive (all ABIs in one zip):
  `onnxruntime-android-1.28.2.zip` = **33,765,715 bytes (~32.2 MiB)**; per-ABI static
  variant `onnxruntime-android-arm64-v8a-static_lib-1.28.2.zip` = 229,052,542 bytes
  — https://api.github.com/repos/csukuangfj/onnxruntime-libs/releases/tags/v1.28.2
- Published per-ABI `.so` sizes on arm64-v8a from the official Android build guide:
  `libonnxruntime.so` = 15 MB, `libsherpa-onnx-jni.so` = 3.7 MB; the measured APK
  runtime component was 7.2 MB total, of which `libonnxruntime.so` was 5.8 MB
  — https://k2-fsa.github.io/sherpa/onnx/android/build-sherpa-onnx.html
  (that page predates the build script above and names ONNX Runtime 1.17.1)
- Android prebuilt artifacts: **yes**, several shapes, all at
  https://github.com/k2-fsa/sherpa-onnx/releases/tag/v1.13.7 (latest release
  **v1.13.7**, published 2026-09-01):
  `sherpa-onnx-1.13.7.aar` = **49,113,869 B** (46.8 MB);
  `sherpa-onnx-static-link-onnxruntime-1.13.7.aar` = **37,809,521 B** (36.1 MB);
  `sherpa-onnx-1.13.7-rknn.aar` = **26,121,272 B** (24.9 MB);
  `sherpa-onnx-v1.13.7-android.tar.bz2` = **45,287,000 B**;
  `sherpa-onnx-v1.13.7-android-static-link-onnxruntime.tar.bz2` = **34,346,901 B**;
  `sherpa-onnx-v1.13.7-android-aarch64-termux-shared.tar.bz2` = 16,171,946 B;
  `sherpa-onnx-jvm-1.13.7.jar` = 187,297 B
  — https://github.com/k2-fsa/sherpa-onnx/releases/expanded_assets/v1.13.7
- The CI workflow that produces the Android tarball exports
  `SHERPA_ONNX_ENABLE_C_API=ON` before running the per-ABI build scripts and tars the
  resulting `jniLibs/` tree
  — https://github.com/k2-fsa/sherpa-onnx/blob/master/.github/workflows/android.yaml

### Is there a documented pure-C++ API usable from CMake? — yes
- Build with `-DSHERPA_ONNX_ENABLE_C_API=ON -DBUILD_SHARED_LIBS=ON`; that produces
  `libsherpa-onnx-c-api`, `libsherpa-onnx-cxx-api`, a `sherpa-onnx.pc` pkg-config file,
  and headers `sherpa-onnx/c-api/c-api.h` and `sherpa-onnx/c-api/cxx-api.h`. The static
  build additionally ships `libespeak-ng.a` and `libpiper_phonemize.a`
  — https://k2-fsa.github.io/sherpa/onnx/c-api/index.html
- The Android build script's own generated README states verbatim:
  "`libsherpa-onnx-c-api.so` and `libsherpa-onnx-cxx-api.so` are for users who don't
  use JNI. In that case, `libsherpa-onnx-jni.so` is not needed. In any case,
  `libonnxruntime.so` is always needed."
  — https://github.com/k2-fsa/sherpa-onnx/blob/master/build-android-arm64-v8a.sh
- Documented C++ usage (namespace `sherpa_onnx::cxx`, `OfflineTts::Create`,
  `tts.Generate(text, gen_cfg, ProgressCallback)`,
  `WriteWave(filename, {audio.samples, audio.sample_rate})`), compiled with
  `g++ -std=c++17 … -lsherpa-onnx-cxx-api -lsherpa-onnx-c-api -lonnxruntime`
  — https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-chaowen-medium.html
- Android platform floor knob: `SHERPA_ONNX_ANDROID_PLATFORM` defaults to `android-21`,
  with an in-script comment that `-DANDROID_PLATFORM=android-27` is needed for NNAPI
  — same build-script URL. (API 28 is above the default floor.)
- Upstream project description claims offline operation explicitly: "…using next-gen
  Kaldi with onnxruntime **without Internet connection**. Support embedded systems,
  Android, iOS, …" — https://api.github.com/repos/k2-fsa/sherpa-onnx

### Chinese model(s), published sizes and quantisation
| Model | Speakers | Published ONNX file size | Sample rate | Published archive size |
|---|---|---|---|---|
| `vits-piper-zh_CN-chaowen-medium` | 1 | not published in sherpa docs (upstream file 63,221,984 B) | 22050 Hz | not published |
| `vits-piper-zh_CN-xiao_ya-medium` | 1 | not published in sherpa docs (upstream file 63,221,984 B) | 22050 Hz | not published |
| `matcha-icefall-zh-baker` | 1 (female) | `model-steps-3.onnx` = 72 MB + `vocos-22khz-univ.onnx` = 51 MB | 22050 Hz | not published |
| `vits-icefall-zh-aishell3` | 174 | 30 MB | 8000 Hz | **31,559,701 B** |
| `vits-zh-aishell3` | 174 | 116 MB | 8000 Hz | 146,922,607 B |
| `vits-zh-hf-fanchen-C` | 187 | 116 MB | 16000 Hz | 119,326,431 B |
| `vits-zh-hf-theresa` | 804 | 117 MB | 22050 Hz | 120,596,617 B |
| `vits-zh-hf-eula` | 804 | 117 MB | 22050 Hz | 120,562,119 B |
| `vits-zh-ll` | 5 | 115 MB | 16000 Hz | not published |
| `vits-melo-tts-zh_en` (zh+en) | 1 | `model.onnx` = 163 MB, `lexicon.txt` = 6.5 MB | 44100 Hz | not published |
| `matcha-icefall-zh-en` | 1 | not published | not published | not published |
| `kokoro-multi-lang-v1_0` (zh+en) | 53 | `model.onnx` = 310 MB, `voices.bin` = 26 MB, `lexicon-zh.txt` = 2.3 MB | 24000 Hz | not published |
| `kokoro-multi-lang-v1_1` (zh+en) | 103 | not published (RTF table lists 311 MB) | 24000 Hz | not published |
| `kokoro-int8-multi-lang-v1_1` | 103 | **int8 quantised variant** | 24000 Hz | not published |

Sources: RTF/size table https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html ;
VITS table + per-model listings https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html ;
Matcha listing https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html ;
Kokoro listing https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html ;
Kokoro v1_1 speaker map https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese-English/kokoro-multi-lang-v1_1.html ;
archive byte counts https://api.github.com/repos/k2-fsa/sherpa-onnx/releases/tags/tts-models .

`kokoro-multi-lang-v1_0` Chinese speakers are sids 45–52: `zf_xiaobei`, `zf_xiaoni`,
`zf_xiaoxiao`, `zf_xiaoyi`, `zm_yunjian`, `zm_yunxi`, `zm_yunxia`, `zm_yunyang`
— https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html .
`kokoro-multi-lang-v1_1` maps **55 Chinese female (`zf_*`, sid 3–57) + 45 Chinese male
(`zm_*`, sid 58–102)** plus 3 English speakers = 103, "Sample rate … fixed to 24000 Hz"
— https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese-English/kokoro-multi-lang-v1_1.html .

Quantisation: the only Chinese/mixed model with a **published quantised artefact** in
these pages is `kokoro-int8-multi-lang-v1_1.tar.bz2` ("`int8` quantization"). The VITS
page's own `*.onnx` listings show int8 only for `vits-vctk`
(`vits-vctk.int8.onnx` 37 MB vs 116 MB fp32), **not** for the Chinese VITS models
— https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html .
Separately, third-party fp16/int8 re-exports of two Mandarin Piper voices do exist:
`csukuangfj2/vits-piper-zh_CN-xiao_ya-medium-fp16` = 31,922,716 B and `…-int8` =
18,652,795 B; `…/vits-piper-zh_CN-chaowen-medium-int8` = 18,652,796 B
— https://huggingface.co/api/models/csukuangfj2/vits-piper-zh_CN-xiao_ya-medium-fp16/tree/main
and https://huggingface.co/api/models/csukuangfj2/vits-piper-zh_CN-chaowen-medium-int8/tree/main .

### Chinese phonemisation / text front-end
- **Lexicon/pinyin dictionary + longest-match word segmentation**, not espeak-ng, for
  the native Chinese models. They are configured with
  `config.model.vits.lexicon = "<model>/lexicon.txt"` + `tokens.txt`
  (e.g. https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-chaowen-medium.html),
  and Matcha Chinese with `--matcha-lexicon=…/lexicon.txt`.
  The front-end change is PR #2507 "Support specifying pronunciations of phrases in
  Chinese TTS", whose description reads: "Introduces longest-match, dictionary-based
  tokenization for more accurate word segmentation. Reduces out-of-vocabulary
  artifacts by smarter multi-token matching before fallback. Improves sentence
  handling by flushing on punctuation for cleaner segmentation boundaries."
  — https://github.com/k2-fsa/sherpa-onnx/pull/2507
- **Rule FSTs** `phone.fst` / `date.fst` / `number.fst` ship inside the Chinese bundles
  and are passed via `--tts-rule-fsts=phone.fst,date.fst,number.fst`; the docs'
  examples deliberately exercise 多音字-sensitive and numeric content (银行/行, 行政,
  长江/长白山, 110, 18920240511, 123456块钱)
  — https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html ,
  https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-chaowen-medium.html
- OOV handling is documented as "add them to `lexicon.txt`"
  — https://k2-fsa.github.io/sherpa/onnx/tts/faq.html
- jieba was used historically (a debug log shows `After jieba: 下面_是_一个_测试_悬界_芯片…`
  in https://k2-fsa.github.io/sherpa/onnx/homophone-replacer/index.html) but
  **PR #2664 "Remove cppjieba" (merged 2025-10-10)** removed it: "TTS and homophone
  replacement now use a character-based lexicon"
  — https://github.com/k2-fsa/sherpa-onnx/pull/2664
- `espeak-ng-data` is required only for models whose commands pass `*-data-dir`
  (Piper English, matcha-ljspeech, kokoro, ZipVoice) — the docs pass `--vits-lexicon`
  and no data-dir for `vits-melo-tts-zh_en` and `matcha-icefall-zh-baker`
  — https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html ,
  https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html
  The shared `espeak-ng-data.tar.bz2` is 7,252,012 B
  — https://api.github.com/repos/k2-fsa/sherpa-onnx/releases/tags/tts-models

### Speed (all published RTF values are Raspberry Pi 4 Model B Rev 1.5)
Source for the whole table: https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html

| Model | 1 thread | 2 | 3 | 4 | Architecture |
|---|---|---|---|---|---|
| `vits-icefall-zh-aishell3` (30 MB) | 0.365 | 0.220 | 0.171 | **0.156** | VITS, non-autoregressive |
| `matcha-icefall-zh-baker` (73 MB + 51 MB vocoder) | 0.892 | 0.536 | 0.432 | **0.391** | Matcha-TTS flow matching, non-autoregressive |
| `vits-zh-hf-fanchen-C` (116 MB) | 4.306 | 2.451 | 1.846 | 1.600 | VITS |
| `vits-zh-hf-fanchen-wnj` (116 MB) | 4.276 | 2.505 | 1.827 | 1.608 | VITS |
| `sherpa-onnx-vits-zh-ll` (116 MB) | 4.275 | 2.494 | 1.840 | 1.593 | VITS |
| `vits-zh-hf-theresa` (117 MB) | 6.032 | 3.448 | 2.566 | 2.210 | VITS |
| `vits-zh-hf-eula` (117 MB) | 6.011 | 3.473 | 2.537 | 2.231 | VITS |
| `vits-melo-tts-zh_en` (163 MB) | 6.727 | 3.877 | 2.914 | 2.518 | VITS |
| `kokoro-multi-lang-v1_1` (311 MB) | 7.635 | 4.470 | 3.430 | 3.191 | Kokoro (StyleTTS2-family) |
| `kokoro-en-v0_19` (English, 330 MB) | 6.629 | 3.870 | 2.999 | 2.774 | Kokoro |
| `vits-piper-en_US-lessac-medium` (English, 61 MB) | 0.774 | 0.482 | 0.390 | 0.357 | VITS |
| `matcha-icefall-en_US-ljspeech` (English) | 0.941 | 0.561 | 0.451 | 0.411 | Matcha |

**No published RTF or latency figure for arm64 Android hardware was found for any
sherpa-onnx model** — the only figures the project publishes are the Raspberry Pi 4
numbers above.

### Licence
- **Code/engine: Apache-2.0** — https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/master/LICENSE
  and GitHub API `"license":{"spdx_id":"Apache-2.0"}`
  — https://api.github.com/repos/k2-fsa/sherpa-onnx
- **Model weights differ per model and are not uniformly permissive:**
  - `matcha-icefall-zh-baker` — the docs carry a Caution: "**The dataset is for
    `non-commercial` use only.**" (dataset from https://en.data-baker.com/datasets/freeDatasets/)
    — https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html
  - `vits-melo-tts-zh_en` — converted from `myshell-ai/MeloTTS-Chinese`; the bundle ships
    its own `LICENSE` file (1.0K) which the docs list but do not reproduce
    — https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html
  - The Matcha page also states: "Models are from icefall. **We don't support models from
    https://github.com/shivammehta25/Matcha-TTS.**"
  - The repo-level Apache-2.0 does not cover weights.

### Integration shape
- Pure C/C++ library; usable as prebuilt `.so` + header or built from source
  (`-DSHERPA_ONNX_ENABLE_C_API=ON`). **No JNI required** if you use
  `libsherpa-onnx-c-api.so`.
- Artifacts for `jniLibs/arm64-v8a/`: `libonnxruntime.so` (15 MB published) +
  `libsherpa-onnx-c-api.so` (size **not published**). The exact file list inside
  `sherpa-onnx-v1.13.7-android.tar.bz2` was not verified — see "Not determinable".
- espeak-ng and piper_phonemize are compiled in (libraries only, executables off:
  `-DBUILD_PIPER_PHONEMIZE_EXE=OFF -DBUILD_ESPEAK_NG_EXE=OFF`)
  — https://github.com/k2-fsa/sherpa-onnx/blob/master/build-android-arm64-v8a.sh
- **No Python step is needed to use the published models.** Python export scripts exist
  in `scripts/` for some models (melo-tts, kokoro, matcha) but the shipped models are
  pre-converted — https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html
- **Alternative integration with no native linking at all:** sherpa-onnx also publishes
  a **system TTS-engine APK** per model, so a Qt app could reach it through
  `android.speech.tts` instead of linking it — e.g.
  `sherpa-onnx-1.13.7-arm64-v8a-zho-tts-engine-vits-piper-zh_CN-chaowen-medium.apk`;
  source https://github.com/k2-fsa/sherpa-onnx/tree/master/android/SherpaOnnxTtsEngine ,
  download links on the per-model pages, e.g.
  https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-chaowen-medium.html

### PCM output
Yes. `SherpaOnnxGeneratedAudio` = `const float *samples` ("Generated mono samples in
the range [-1, 1]"), `int32_t n`, `int32_t sample_rate` ("Output sample rate")
— https://k2-fsa.github.io/sherpa/onnx/c-api/html/structSherpaOnnxGeneratedAudio.html .
Incremental generation callbacks exist: `SherpaOnnxGeneratedAudioCallback(samples, n)`,
`SherpaOnnxGeneratedAudioProgressCallback(samples, n, p)` and `…WithArg` variants taking
a `void *`; config also exposes `max_num_sentences` and `silence_scale`
— https://k2-fsa.github.io/sherpa/onnx/c-api/html/c-api_8h.html ,
https://k2-fsa.github.io/sherpa/onnx/c-api/html/structSherpaOnnxOfflineTtsConfig.html .
Sample rate per model is in the table above (8000 / 16000 / 22050 / 24000 / 44100 Hz).

---

## 2. Piper (rhasspy/piper → OHF-Voice/piper1-gpl)

### Maintenance, canonical repo, versions
- `rhasspy/piper` is **archived** (`"archived":true`), MIT, last push 2025-08-26; its
  `master` README is a stub: "Development has moved: https://github.com/OHF-Voice/piper1-gpl"
  — https://raw.githubusercontent.com/rhasspy/piper/master/README.md ,
  https://api.github.com/repos/rhasspy/piper
- Current canonical repo `OHF-Voice/piper1-gpl`: not archived, **GPL-3.0**; latest
  release **v1.8.0 (2026-09-04)**
  — https://github.com/OHF-Voice/piper1-gpl/releases/tag/v1.8.0
- The README still carries an open "Looking for Maintainers" notice
  — https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/README.md

### Chinese voices and exact published sizes
The `rhasspy/piper-voices` repository contains **three `zh_CN` voice families**
(`chaowen`, `huayan`, `xiao_ya`) and **four downloadable models**; there is no
`zh_TW`, and no `low` or `high` tier for any Chinese voice
— https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN

| Voice (path) | Published `.onnx` size (bytes) | Tier | Sample rate | Speakers | Front-end |
|---|---|---|---|---|---|
| `zh_CN/huayan/x_low/zh_CN-huayan-x_low.onnx` | **20,628,813** | x_low | 16000 Hz | 1 | espeak `cmn` |
| `zh_CN/huayan/medium/zh_CN-huayan-medium.onnx` | **63,201,294** | medium | 22050 Hz | 1 | espeak `cmn` |
| `zh_CN/chaowen/medium/zh_CN-chaowen-medium.onnx` | **63,221,984** | medium | 22050 Hz | 1 | pinyin (g2pW) |
| `zh_CN/xiao_ya/medium/zh_CN-xiao_ya-medium.onnx` | **63,221,984** | medium | 22050 Hz | 1 | pinyin (g2pW) |

Sizes from the repository tree API:
https://hf-mirror.com/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/x_low ,
`…/huayan/medium`, `…/chaowen/medium`, `…/xiao_ya/medium`.
Each voice also ships a `*.onnx.json` config (2,927–4,822 B).

**Quantisation: none published for Chinese by the Piper project.** Those directories
contain only `MODEL_CARD`, `.onnx`, `.onnx.json`, `samples/` (plus `ALIASES` for
`huayan/x_low`). The stored numeric precision of the released `.onnx` weights is
**not published**. Third-party fp16/int8 re-exports exist only for `xiao_ya` and
`chaowen` (see §1).

### Chinese phonemisation — two different pipelines
**(a) `huayan` → eSpeak-NG `cmn`.** `zh_CN-huayan-medium.onnx.json`:
```json
"espeak": { "voice": "cmn" },
"phoneme_type": "espeak",
"dataset": "huayan"
```
Its `phoneme_id_map` is the espeak-ng IPA inventory (includes `ɕ ʂ ʐ ɤ ɥ` at ids
55/95/106/69/70). `x_low` also has `"espeak": {"voice": "cmn"}` (predates the explicit
`phoneme_type` key; `piper_version` 0.2.0 vs 1.0.0)
— https://hf-mirror.com/rhasspy/piper-voices/raw/main/zh/zh_CN/huayan/medium/zh_CN-huayan-medium.onnx.json ,
https://hf-mirror.com/rhasspy/piper-voices/raw/main/zh/zh_CN/huayan/x_low/zh_CN-huayan-x_low.onnx.json

**(b) `xiao_ya` and `chaowen` → pinyin + g2pW.** Their configs have
`"phoneme_type": "pinyin"` and a 256-entry **pinyin initials/finals + tone-digit**
inventory (`Ø`, `b p m f…`, `zh ch sh`, `y w`, `a o e ai ei ao ou an en ang eng ong`,
`v ve van vn er ue`, digits `1`–`5`, punctuation classes); `"espeak": {"voice": "zh"}`
is vestigial
— https://hf-mirror.com/rhasspy/piper-voices/raw/main/zh/zh_CN/xiao_ya/medium/zh_CN-xiao_ya-medium.onnx.json .
The `xiao_ya` MODEL_CARD states the voice "Only works on the **Python** version of Piper
1.4+ due to a dependency on [g2pW](https://github.com/GitYCC/g2pW/)".

**Offline implication (fact, not inference).** The pinyin path fetches at runtime: the
g2pW model is downloaded from
`https://huggingface.co/datasets/rhasspy/piper-checkpoints/resolve/main/zh/zh_CN/_resources/g2pw.tar.gz`
(see `G2PW_URL` in `src/piper/phonemize_chinese.py`), and an open PR documents that the
same path also makes "a separate HTTP request to Hugging Face for the `bert-base-chinese`
tokenizer" — https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/src/piper/phonemize_chinese.py ,
https://github.com/OHF-Voice/piper1-gpl/pull/245 . Only `huayan` avoids that
for phonemisation.

**The C++ library's Chinese phonemizer is documented as degraded.** `libpiper`'s
`chinese_phonemizer.cpp` is headed "Phase 1 (monophonic …)" with the comment
"**Relaxed: use first reading for poly to avoid empty -> silence**"; `g2pw.onnx` and
`POLYPHONIC_CHARS.txt` are documented as "Phase 2". Merged PR #271 is titled
"Add Chinese pinyin support – **Phase 1 (monophonic dict fallback, honest scoping)**"
and defers context to #272, which **has not shipped**
— https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/chinese_phonemizer.cpp ,
https://github.com/OHF-Voice/piper1-gpl/pull/271 ,
https://github.com/OHF-Voice/piper1-gpl/pull/272

### Speed
- **Piper publishes no RTF or latency figure for any voice.** The legacy README says
  only "optimized for the Raspberry Pi 4" (the only hardware it ever names); the current
  README says only "A fast and local neural text-to-speech engine"
  — https://raw.githubusercontent.com/rhasspy/piper/v1.2.0/README.md ,
  https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/README.md
- `realTimeFactor` existed only in the **legacy** C++ result struct
  (`SynthesisResult{inferSeconds, audioSeconds, realTimeFactor}`); the current chunk
  struct has **no timing fields**
  — https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.hpp ,
  https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h
- The only published RTF table with named hardware that includes Piper voices is
  third-party (sherpa-onnx, Raspberry Pi 4 Model B Rev 1.5): `vits-piper-en_US-lessac-medium`
  0.774 / 0.482 / 0.390 / 0.357 at 1/2/3/4 threads (61 MB, English)
  — https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html
- **RTF for any zh_CN Piper voice: not published. RTF on Android arm64: not published.**

### Licence split
- **Code:** legacy `rhasspy/piper` = **MIT**; current `OHF-Voice/piper1-gpl` =
  **GPL-3.0** (`COPYING` is the full GPLv3 text; the repo name encodes it). The
  relicence is recorded in the changelog under 1.3.0 "Change license to GPLv3"
  — https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/COPYING ,
  https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md
- **Weights:** the voices repo declares `license: mit` at repository level
  (https://hf-mirror.com/rhasspy/piper-voices/raw/main/README.md), but the project's own
  docs say: "The `MODEL_CARD` file for each voice contains important licensing
  information. **Piper is intended for personal use and text to speech research only**;
  … Some voices may have restrictive licenses, however, so please review them carefully!"
  — https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/VOICES.md
- Per-voice MODEL_CARDs:
  - `xiao_ya` — dataset `openspeech/BZNSYP`, "License: **Non-commercial use** (see
    https://www.data-baker.com/data/index/TNtts/)"
  - `huayan` — dataset `PlayVoice/HuaYan_TTS`, "License: **Unknown**"; "Finetuned from
    U.S. English lessac voice (medium quality)"
  - `chaowen` — dataset `OHF-Voice/voice-datasets`, "License: **CC0**"; "Finetuned from
    Xiao Ya voice (medium quality)"
  — https://hf-mirror.com/rhasspy/piper-voices/raw/main/zh/zh_CN/xiao_ya/medium/MODEL_CARD ,
  https://hf-mirror.com/rhasspy/piper-voices/raw/main/zh/zh_CN/huayan/medium/MODEL_CARD ,
  https://hf-mirror.com/rhasspy/piper-voices/raw/main/zh/zh_CN/chaowen/medium/MODEL_CARD
  `chaowen` is a finetune **of** `xiao_ya`, so the non-commercial marker is materially
  relevant and is not resolved by the repo-level MIT tag.

### Integration shape
- **Pure C/C++ API exists:** `libpiper`, a shared library (`add_library(piper SHARED …)`)
  with a flat `extern "C"` API in `piper.h`:
  `piper_create(model_path, config_path, espeak_data_path)`,
  `piper_default_synthesize_options`, `piper_synthesize_start`,
  `piper_synthesize_next(synth, &chunk)` looped until `PIPER_DONE`; the current API also
  has an ABI-versioned `piper_create_options` including a `g2pw_model_dir` field
  — https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md ,
  https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h
- Build: `cmake -B build -DCMAKE_BUILD_TYPE=Release …`, which "will automatically
  download/build espeak-ng as well as download shared libraries for the onnxruntime".
  Runtime needs the espeak-ng data directory: "Provide `piper_create` with the path to
  espeak-ng's data (`install/espeak-ng-data/`)" plus `libpiper` + `libonnxruntime`
  — https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md
- Default ONNX Runtime version **1.22.0**; espeak-ng is built statically from a pinned
  commit; Mandarin data is opt-in at espeak-ng build time (`-DEXTRA_cmn:BOOL=ON`)
  — https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt
- **No JNI needed** (the third-party `jvoice-project/piper-jni` binding is optional).
- **No official Android build and no prebuilt shared library for any platform:**
  release assets are Python wheels/sdist only; `libpiper/CMakeLists.txt` dispatches only
  WIN32/APPLE/Linux and otherwise `message(FATAL_ERROR "Unsupported architecture for
  onnxruntime")` — there is no ANDROID branch and no NDK toolchain file
  — https://github.com/OHF-Voice/piper1-gpl/releases ,
  https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt
  Only third-party Android routes exist, all non-official (sherpa-onnx APKs — a
  *different engine*; a Termux hand-build; `nihui/ncnn-android-piper` on ncnn).
  The official README's Java binding `jvoice-project/piper-jni` lists no Android support.
- `espeak-ng-data` size: **Piper publishes no figure**. Measured from a real bundled tree:
  the compiled `cmn_dict` is 1,566,335 B (`phondata` 550,424; `yue_dict` 563,571;
  `en_dict` 166,944) — https://huggingface.co/api/models/csukuangfj/vits-piper-zh_CN-huayan-medium/tree/main/espeak-ng-data

### PCM output
Yes: **mono float32**, streamed in chunks; the documented example pipes it to
`aplay -r 22050 -c 1 -f FLOAT_LE -t raw` for the zh_CN voices, i.e. **22050 Hz**
(legacy returned `int16`; current returns `const float *samples`)
— https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md .

---

## 3. Android system TextToSpeech (`android.speech.tts.TextToSpeech`)

### Does it work fully offline?
- The framework gives **no blanket offline guarantee**. The class doc promises only
  "Synthesizes speech from text for immediate playback or to create a sound file", and
  an engine must be installed and initialised
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java
- Offline is an **engine-declared feature**: `Engine.KEY_FEATURE_EMBEDDED_SYNTHESIS` =
  `"embeddedTts"` — "If set and supported by the engine as per
  `TextToSpeech#getFeatures(Locale)`, the engine must synthesize text on-device
  (without making network requests)". `@Deprecated` since API 21.
  Its counterpart `KEY_FEATURE_NETWORK_SYNTHESIS` = `"networkTts"` — "the engine must
  use network based synthesis"; the API-21 replacement is documented as: call
  `getVoices()`, find a suitable network voice (`Voice#isNetworkConnectionRequired()`)
  and pass it to `setVoice(Voice)`
  — same `TextToSpeech.java` URL
- `Voice.isNetworkConnectionRequired()` — "Does the Voice require a network connection
  to work", backed by an engine-reported `mRequiresNetworkConnection` field
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/Voice.java
- AOSP's own Settings decides network-need with exactly that pair:
  `return features.contains(KEY_FEATURE_NETWORK_SYNTHESIS) && !features.contains(KEY_FEATURE_EMBEDDED_SYNTHESIS);`
  and shows the `tts_engine_network_required` alert
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-9.0.0_r1/src/com/android/settings/tts/TextToSpeechSettings.java
- **Net fact:** the framework can drive a fully offline engine, but only if the installed
  engine exposes a voice with `isNetworkConnectionRequired() == false`. No AOSP source
  read asserts that any specific shipping engine is fully offline.

### Is voice data required to be pre-downloaded?
- `Engine.ACTION_INSTALL_TTS_DATA` = `"android.speech.tts.engine.INSTALL_TTS_DATA"`:
  "Triggers the platform TextToSpeech engine to start the activity that installs the
  resource files on the device that are **required for TTS to be operational**"; the doc
  warns the user can decline, so apps "should check installation status with
  `ACTION_CHECK_TTS_DATA`"
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java
- `ACTION_CHECK_TTS_DATA` returns `CHECK_VOICE_DATA_PASS=1` / `CHECK_VOICE_DATA_FAIL=0`
  plus `EXTRA_AVAILABLE_VOICES` and `EXTRA_UNAVAILABLE_VOICES` ("all the unavailable
  voices (**ones that user can install**)"), and there is an
  `ACTION_TTS_DATA_INSTALLED` broadcast — same URL.
- Missing-data signals: `LANG_MISSING_DATA = -1` ("the language data is missing"),
  `ERROR_NOT_INSTALLED_YET = -9` ("failure caused by an unfinished download of the voice
  data"), and `KEY_FEATURE_NOT_INSTALLED` = `"notInstalled"` — "the voice may need to
  download additional data to be fully functional. **The download will be triggered by
  calling `setVoice(Voice)` or `setLanguage(Locale)`.**" — same URL.
- AOSP Settings 4.4 had a user-facing install-data preference (`KEY_INSTALL_DATA =
  "tts_install_data"`, enabled only when unavailable voices exist) that launched
  `ACTION_INSTALL_TTS_DATA`
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-4.4_r1/src/com/android/settings/tts/TtsEngineSettingsFragment.java .
  In Android 9 (API 28) that preference is gone; per-engine download is delegated to
  whatever UI the engine app ships, reached via `TtsEngines.getSettingsIntent(engine)`
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-9.0.0_r1/src/com/android/settings/tts/TextToSpeechSettings.java
- **No engine installed:** `TtsEngines.getHighestRankedEngineName()` returns `null` "if
  no TTS engines were present in the system image"; `getEngines()` "can be empty, but
  never null"; `initTts()` falls back requested→default→highest-ranked and, if all fail,
  dispatches `ERROR` with the in-source comment "This might fail for various reasons like
  if the user disables all their TTS engines"
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TtsEngines.java ,
  https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java
  AOSP Settings hides the TTS page entirely when `getEngines().isEmpty()`.
- **Whether Google's engine (`com.google.android.tts`) is present on a device without
  Play Services, and the exact wording of Google's "Install voice data" support article,
  are NOT determinable from public sources** — `support.google.com`, `play.google.com`
  and `developer.android.com` were unreachable, and the literal string "Install voice
  data" does not appear in the retrieved AOSP Settings sources. The package name
  `com.google.android.tts` could not be confirmed from any retrieved Google primary
  source.

### Which engines implement TextToSpeech?
- **Registration contract (authoritative):** any app exposing a Service for
  `TextToSpeech.Engine.INTENT_ACTION_TTS_SERVICE` (`"android.intent.action.TTS_SERVICE"`);
  discovery is `pm.queryIntentServices(new Intent(INTENT_ACTION_TTS_SERVICE), MATCH_DEFAULT_ONLY)`;
  "the 'engine name' is the same as the package name"; "the current API allows only one
  engine per package name"; optional meta-data `TextToSpeech.Engine.SERVICE_META_DATA`
  (`"android.speech.tts"`) pointing at XML whose root tag must be `tts-engine`; ranking
  puts system-image engines first, then by declared `priority`
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TtsEngines.java
- Legacy `DEFAULT_ENGINE = "com.svox.pico"` is `@hide @Deprecated`
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java
- **eSpeak NG is a verified standard engine:** its manifest registers exactly the
  contract — `<service android:name=".TtsService" android:directBootAware="true"
  android:exported="true">` with `<action android:name="android.intent.action.TTS_SERVICE"/>`,
  `<meta-data android:name="android.speech.tts" android:resource="@xml/tts_engine"/>`,
  plus `.DownloadVoiceData` (INSTALL_TTS_DATA), `.CheckVoiceData` (CHECK_TTS_DATA),
  `.GetSampleText`, `.TtsSettingsActivity`; versionName 1.53.0
  — https://cdn.jsdelivr.net/gh/espeak-ng/espeak-ng@master/android/AndroidManifest.xml
- **Chinese engines other than Google's:** vendor SDKs for **iFlytek (讯飞)** and
  **Baidu** are documented, but their own docs describe **proprietary SDKs** only
  (`SpeechUtility.createUtility`, `msc.jar`/`libmsc.so`, `SpeechSynthesizer`,
  `createSynthesizer`/`startSpeaking`/`synthesizeToUri`, offline AIkit AAR) and **none of
  the retrieved vendor pages mention `android.intent.action.TTS_SERVICE`,
  `android.speech.tts` meta-data, `CHECK_TTS_DATA` or `INSTALL_TTS_DATA`**
  — https://www.xfyun.cn/doc/tts/online_tts/Android-SDK.html ,
  https://www.xfyun.cn/doc/mscapi/Android/androidsynthesizer.html ,
  https://www.xfyun.cn/doc/tts/AIkit_offline_tts/Android-SDK%28Lightweight%29.html ,
  https://ai.baidu.com/ai-doc/SPEECH/cltwwjwqm
  Whether any of them registers as a system TTS engine is **not documented**.
  **Samsung (`com.samsung.SMT*`)**: `developer.samsung.com`, Galaxy Store and Knox
  docs were unreachable — **not determinable from public sources**.
- AOSP's reference engine (`PicoTts`) is not present at the expected mirror path
  (`https://github.com/aosp-mirror/platform_packages_apps_PicoTts` → 404).

### How a Qt C++ app reaches it
- **Qt ships a wrapper** (see §4), and that is the supported path. But Qt's Android
  backend **ships a Java class compiled by Gradle** — so it is not a "zero Java" path
  for the app build:
  - C++ declares `Q_DECLARE_JNI_CLASS(QtTextToSpeech, "org/qtproject/qt/android/speech/QtTextToSpeech")`
    and holds a `QJniObject`: https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.h
  - Java helper: https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech/QtTextToSpeech.java
  - Built into jar `Qt6AndroidTextToSpeech` by `qt_internal_add_jar(... OUTPUT_DIR "${QT_BUILD_DIR}/jar")`
    + `install_jar(...)`: https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/CMakeLists.txt ,
    with `plugins { alias(libs.plugins.android.library) }`, namespace
    `org.qtproject.qt.android.speech`, `minSdk` default **28**, `compileSdk` default 36:
    https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/build.gradle
  - The plugin `add_dependencies(... Qt${QtSpeech_VERSION_MAJOR}AndroidTextToSpeech)`:
    https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/CMakeLists.txt
  - `JNI_OnLoad` registers 7 native methods (`notifyError`, `notifyReady`,
    `notifySpeaking`, `notifyRangeStart`, `notifyBeginSynthesis`, `notifyAudioAvailable`,
    `notifyEndSynthesis`):
    https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.cpp
- **Can `QJniObject::callMethod` on `"android/speech/tts/TextToSpeech"` work with no Java
  source at all?** Calling *framework* classes from C++ with no app Java code is
  documented, with fully-qualified framework examples
  (`QJniObject::callStaticObjectMethod("java/lang/Thread","currentThread",…)`,
  `Q_DECLARE_JNI_CLASS(SettingsSecure, "android/provider/Settings$Secure")`)
  — https://doc.qt.io/qt-6/qjniobject.html . **But** `TextToSpeech`'s constructor
  requires a `TextToSpeech.OnInitListener` and progress arrives via the abstract
  `UtteranceProgressListener`
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java ,
  https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/UtteranceProgressListener.java
  and Java→C++ callbacks require `QJniEnvironment::registerNativeMethods()`, hence a Java
  `native` declaration. Qt's own documented answer for that case is **generated Java glue
  in a Gradle project (Qt Jenny)**: "For an application that uses both method invocations
  and interface callbacks, two generation projects are needed"
  — https://doc.qt.io/qt-6/qtjenny.html , https://doc.qt.io/qt-6/qtjenny-installation.html
  No Qt example, forum thread or bug report was found that drives
  `android.speech.tts.TextToSpeech` from `QJniObject` with **no** Java class.
- For context, Qt for Android already uses Gradle (documented as Gradle 9.3.1 + AGP 9.0.0,
  JDK 21, NDK r27c) — https://doc.qt.io/qt-6/android.html

### API 30+ manifest requirement
"Apps targeting Android 11 that use text-to-speech should declare
`TextToSpeech.Engine#INTENT_ACTION_TTS_SERVICE` in the `queries` elements":
```xml
<queries><intent><action android:name="android.intent.action.TTS_SERVICE"/></intent></queries>
```
— https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java

### Sample rate and raw PCM
- **`TextToSpeech.getSampleRate()` does not exist.** The complete public method list in
  AOSP master `TextToSpeech.java` (`addSpeech`, `speak`, `playEarcon`,
  `playSilentUtterance`, `getFeatures`, `isSpeaking`, `stop`, `setSpeechRate`,
  `setPitch`, `setAudioAttributes`, `getCurrentEngine`, `getDefaultLanguage`,
  `setLanguage`, `getLanguage`, `setVoice`, `getVoice`, `getDefaultVoice`,
  `isLanguageAvailable`, `synthesizeToFile`, `setOnUtteranceProgressListener`,
  `getEngines`, …) contains **no sample-rate getter** — same URL.
- The rate is chosen by the **engine**:
  `SynthesisCallback.start(int sampleRateInHz, @SupportedAudioFormat int audioFormat,
  @IntRange(from=1,to=2) int channelCount)` — "Sample rate in HZ of the generated audio …
  Must be one of `ENCODING_PCM_8BIT` or `ENCODING_PCM_16BIT`. Can also be
  `ENCODING_PCM_FLOAT` when targetting Android N and above. … channelCount … Must be `1`
  or `2`."
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/SynthesisCallback.java
- The **client** learns the format via
  `UtteranceProgressListener.onBeginSynthesis(String utteranceId, int sampleRateInHz,
  int audioFormat, int channelCount)` — "It provides information about the format of the
  byte array for subsequent `onAudioAvailable` calls", and receives PCM chunks via
  `UtteranceProgressListener.onAudioAvailable(String utteranceId, byte[] audio)` —
  "This is called when a chunk of audio is ready for consumption … if `onDone` or
  `onError` is called all chunks have been received"
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/UtteranceProgressListener.java
- **API levels (established by tag-diff, because doc annotations were unreachable):**
  `onBeginSynthesis` + `onAudioAvailable` are **absent at `android-6.0.1_r1` (API 23)**
  and **present at `android-7.0.0_r1` (API 24)** ⇒ **added in API 24**, not API 26.
  `onRangeStart` is absent at `android-7.0.0_r1` and present at `android-7.1.1_r1`
  ⇒ **added in API 25**. Corroborated by Qt gating capabilities on
  `QOperatingSystemVersion::AndroidNougat`
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@android-6.0.1_r1/core/java/android/speech/tts/UtteranceProgressListener.java ,
  https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@android-7.0.0_r1/core/java/android/speech/tts/UtteranceProgressListener.java ,
  https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@android-7.1.1_r1/core/java/android/speech/tts/UtteranceProgressListener.java
- `synthesizeToFile(CharSequence, Bundle, File, String)` is "asynchronous, i.e. the method
  just adds the request to the queue"; the container format is engine-produced, not
  framework-specified — same `TextToSpeech.java` URL.
- **Latency: not published as measurements.** The only latency figures in the framework
  are engine-declared labels on `Voice.getLatency()` — `LATENCY_VERY_LOW` "< 20ms",
  `LATENCY_LOW` "~20ms", `LATENCY_NORMAL` "~50ms", `LATENCY_HIGH` "Network based expected
  synthesizer latency (~200ms)", `LATENCY_VERY_HIGH` "> 200ms"
  — https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/Voice.java
- **The concrete sample rate used by any real shipping engine (e.g. Google's Mandarin
  voices) is not published anywhere in AOSP** — it is only discoverable at runtime via
  `onBeginSynthesis`.

---

## 4. Qt TextToSpeech module (Qt 6)

### Does it exist in Qt 6.11.x? — yes
- The manual is served as "Qt TextToSpeech | Qt 6.11.2"
  — https://doc.qt.io/qt-6/qttexttospeech-index.html
- Module name **QtTextToSpeech** (QML import `QtTextToSpeech`), classes `QTextToSpeech`
  and `QVoice`. CMake: `find_package(Qt6 REQUIRED COMPONENTS TextToSpeech)` +
  `target_link_libraries(mytarget PRIVATE Qt6::TextToSpeech)`; qmake `QT += texttospeech`
  — same URL.
- The `qtspeech` repo has branches `6.11`, `6.11.0`, `6.11.1`, `6.11.2`, `6.11.3`, `6.12`
  — https://code.qt.io/cgit/qt/qtspeech.git/tree/src/plugins/tts/android
- **Introduced in Qt 5.8 as a Technology Preview named "Qt Speech"**: "Technology Preview
  Modules — Qt Speech - A module to make text to speech and speech recognition easy. …
  **It has backends for several speech synthesizers on macOS, Android, Windows and Linux**
  currently." — https://wiki.qt.io/New_Features_in_Qt_5.8 (Qt 5.8 released 23 Jan 2017,
  https://wiki.qt.io/Qt_5.8_Release). Still "Qt Speech" in 5.9 archives
  — https://doc.qt.io/archives/qt-5.9/qtspeech-index.html

### Does it have an Android backend? — yes, and it is the only one there
- "The `android` engine is **the only engine available on the Android platform**. It uses
  the `TextToSpeech` package, which in turn supports multiple engine backends."
  "**Note:** The `android` engine does not have the `PauseResume` capability."
  Engine parameter `androidEngine` (QString) — "There is no API in Qt to get the list of
  installed engines."
  — https://doc.qt.io/qt-6/qttexttospeech-engines.html
- Full documented backend list: **winrt** (Windows.Media.SpeechSynthesis, plays PCM via
  `QAudioSink` from Qt Multimedia), **sapi** (SAPI 5.3, "reduced quality compared to the
  winrt engine"), **darwin** (AVFoundation; iOS + macOS), **android**, **flite**
  (needs Flite ≥ 2.2, renders PCM via `QAudioSink`), **speech-dispatcher** (`speechd`,
  needs libspeechd ≥ 0.9; lacks `WordByWordProgress` and `Synthesize`). A "mock" engine is
  also returned by `availableEngines()` and "should not be deployed to target systems"
  — same URL.
- QtTextToSpeech is listed under **Qt Add-Ons**, not Essentials, and the docs state that
  add-ons "may only be available on some development platform" and are downloaded via the
  installer option — https://doc.qt.io/qt-6/qtmodules.html
  **Whether the module and its `Qt6AndroidTextToSpeech` jar are present by default in the
  Qt Online Installer's Android package, or must be explicitly selected, is not
  determinable from the Qt pages read** — Qt for Android documents only the platform
  configuration table and no per-module list
  (https://doc.qt.io/qt-6/android.html ; https://doc.qt.io/qt-6/supported-platforms.html).

### What it wraps / JNI and Java files needed
Qt does **not** bring its own Mandarin synthesiser on Android. It wraps
`android.speech.tts.TextToSpeech`
(https://developer.android.com/reference/android/speech/tts/TextToSpeech) through
**JNI plus a Java helper class** — see §3 "How a Qt C++ app reaches it" for the exact
files, the `Qt6AndroidTextToSpeech` jar, minSdk 28, the `add_dependencies` linkage and
the 7 registered native methods.
Capability declaration, verbatim:
`{"Keys": ["android"], "Provider": "android", "Version": 100, "Priority": 100,
"Capabilities": ["Speak", "WordByWordProgress", "Synthesize"]}` — **no `PauseResume`**
— https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/android_plugin.json
What the Java class actually calls: `new TextToSpeech(context, listener[, engine])`,
`setOnUtteranceProgressListener`, `speak(text, QUEUE_FLUSH, params, UTTERANCE_ID)` with
`KEY_PARAM_VOLUME`, and for PCM
**`synthesizeToFile(text, params, new File("/dev/null"), SYNTHESIZE_ID)`** with bytes
captured in `onAudioAvailable` → `notifyAudioAvailable`; format mapped in
`onBeginSynthesis` (8BIT→UInt8, 16BIT→Int16, FLOAT→Float); rates/pitch defaults read from
`Settings.Secure.TTS_DEFAULT_RATE`/`TTS_DEFAULT_PITCH` and divided by 100
— https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech/QtTextToSpeech.java
Documented behavioural limitation: "**Note:** On Android, resuming paused speech will
restart from the beginning. This is a limitation of the underlying text-to-speech engine."
— https://doc.qt.io/qt-6/qtexttospeech.html

### Licence
Three Qt primary sources state it slightly differently:
- Module page: commercial, or **LGPL version 3, or GPL version 2**
  — https://doc.qt.io/qt-6/qttexttospeech-index.html
- Source headers: `SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR
  GPL-2.0-only OR GPL-3.0-only`
  — https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.h ,
  https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech/QtTextToSpeech.java
  (build files are `BSD-3-Clause`: https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/CMakeLists.txt)
- Qt 6.11 licensing page: commercial or LGPLv3 (or GPLv3), and **Qt TextToSpeech is not
  in the GPLv3-only module list**
  — https://doc.qt.io/qt-6/licensing.html

### Raw PCM output
- **Yes.** `QTextToSpeech::synthesize()` — "[since 6.6] … **Synthesizes the text into raw
  audio data.** … the functor will be called as `functor(QAudioFormat format, QByteArray
  bytes)` … While synthesizing, the functor might be called multiple times, possibly with
  changing values for `format`." Gated on `Capability::Synthesize` ("The engine can
  synthesize PCM audio data from text", enum introduced in Qt 6.6). `State::Synthesizing`
  = 4 — "Text is being synthesized into PCM data."
  — https://doc.qt.io/qt-6/qtexttospeech.html
- **`setAudioOutput()` does not exist** in the documented Qt 6.11.2 API — it is absent from
  the Public Functions and Public Slots lists, and the only audio-device parameter in the
  module is the engine-level `audioDevice` for the **winrt** and **flite** engines, not
  Android — https://doc.qt.io/qt-6/qtexttospeech.html ,
  https://doc.qt.io/qt-6/qttexttospeech-engines.html
- On Android the PCM path is real: the plugin declares `Synthesize` and the Java glue
  implements it via `synthesizeToFile(..., new File("/dev/null"), ...)` +
  `onAudioAvailable` (URLs above).
- `QTextToSpeechEngine` / `QTextToSpeechPlugin` were removed from the public API in Qt 6 —
  "not part of the documented and supported API"
  — https://doc.qt.io/qt-6/qttexttospeech-changes-qt6.html

---

## 5. MeloTTS (myshell-ai/MeloTTS) — Chinese

- **Runtime dependency:** PyTorch + HuggingFace `transformers`. `requirements.txt` pins
  `torch`, `torchaudio`, `transformers==4.27.4`, `pypinyin==0.50.0`, `cn2an==0.5.22`,
  `jieba==0.42.1` — https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/requirements.txt
- **Architecture: non-autoregressive** (VITS family); README "based on TTS, VITS, VITS2
  and Bert-VITS2", config confirms a VITS synthesizer (`n_layers_trans_flow: 3`,
  `upsample_rates [8,8,2,2,2]`)
  — https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/README.md ,
  https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/melo/configs/config.json
- **Chinese model name + size:** `myshell-ai/MeloTTS-Chinese`, `checkpoint.pth` =
  **207,770,124 B (~198 MiB)**; **no fp16/int8 published**
  — https://huggingface.co/api/models/myshell-ai/MeloTTS-Chinese/tree/main
- **Chinese front-end:** `cn2an` (number normalisation) + `pypinyin` (`Style.INITIALS`,
  `Style.FINALS_TONE3`, `neutral_tone_with_five=True`) + `jieba.posseg` (segmentation with
  POS tags) + a custom `ToneSandhi` module + the `opencpop-strict.txt` pinyin→phoneme map,
  **and a BERT**: `melo/text/chinese_bert.py` hard-codes
  `model_id='hfl/chinese-roberta-wwm-ext-large'` via `AutoModelForMaskedLM.from_pretrained`
  — https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/melo/text/chinese.py ,
  https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/melo/text/chinese_bert.py
- That BERT is **1,306,484,351 B** (`pytorch_model.bin`, ≈1.22 GiB), licence
  **apache-2.0** (a separate licence from MeloTTS)
  — https://huggingface.co/api/models/hfl/chinese-roberta-wwm-ext-large/tree/main ,
  https://huggingface.co/hfl/chinese-roberta-wwm-ext-large
- **Android/C++ path: only via sherpa-onnx.** Official exporter at
  `scripts/melo-tts/`; **it zeroes BERT out** — in `ModelWrapper.forward`,
  `bert = torch.zeros(x.shape[0], 1024, x.shape[1], dtype=torch.float32)`, while metadata
  still declares `"bert_dim": 1024`
  — https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/master/scripts/melo-tts/export-onnx.py ,
  https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html
  That is why the shipped ONNX is 163 MB rather than 207 MB + 1.22 GiB.
- **Python export step needed: yes** (exporter imports torch/onnx/melo at export time only;
  the shipped sherpa-onnx model needs none).
- **Speed:** published RTF on Raspberry Pi 4 Model B Rev 1.5 = **6.727 / 3.877 / 2.914 /
  2.518** at 1/2/3/4 threads (i.e. ~2.5× slower than real time even at 4 threads).
  Upstream claims only "Fast enough for `CPU real-time inference`" with no hardware named.
  Android arm64 RTF: **not published**
  — https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html ,
  https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/README.md
- **Licence:** code **and** weights **MIT** — "This library is under MIT License, which
  means it is free for both commercial and non-commercial use."
  — https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/README.md ;
  HF repo metadata `license: mit` — https://huggingface.co/myshell-ai/MeloTTS-Chinese
  (the required BERT is separately apache-2.0).
- **Integration shape:** no ONNX script in the MeloTTS repo, no Android/CMake/Gradle; the
  C++ route is sherpa-onnx, so it inherits sherpa-onnx's infrastructure — but with a
  Python export step and a 163 MB model.
- **PCM output:** 44100 Hz, mono; published `soxi` shows "Sample Rate: 44100, 16-bit
  Signed Integer PCM" and the config sets `sampling_rate: 44100`
  — https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html ,
  https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/melo/configs/config.json

---

## 6. Kokoro (hexgrad/Kokoro-82M and Kokoro-82M-v1.1-zh)

- **Licence: weights Apache-2.0** for both `hexgrad/Kokoro-82M` and
  `hexgrad/Kokoro-82M-v1.1-zh`; inference lib `hexgrad/kokoro` Apache-2.0;
  `thewh1teagle/kokoro-onnx` MIT ("kokoro-onnx: MIT / kokoro model: Apache 2.0")
  — https://huggingface.co/hexgrad/Kokoro-82M ,
  https://huggingface.co/hexgrad/Kokoro-82M-v1.1-zh ,
  https://api.github.com/repos/hexgrad/kokoro ,
  https://raw.githubusercontent.com/thewh1teagle/kokoro-onnx/main/README.md
- **Architecture: non-autoregressive.** StyleTTS 2 + ISTFTNet; the card says
  "Decoder only: no diffusion, no encoder release"; **82 million parameters**
  — https://huggingface.co/hexgrad/Kokoro-82M-v1.1-zh ,
  https://arxiv.org/abs/2306.07691 , https://arxiv.org/abs/2203.02395
- **Chinese:** `hexgrad/Kokoro-82M-v1.1-zh` sets `license: apache-2.0` via YAML and is
  "the result of a short training run that added **100 Chinese speakers** from a
  professional dataset" (LongMaoData), sample rate **24000 Hz**, "2 & 103" langs and
  voices, ">100 hours"
  — https://huggingface.co/hexgrad/Kokoro-82M-v1.1-zh
- **Upstream 8-voice Mandarin set** in `VOICES.md`, all **Overall Grade "D"**, Target
  Quality "C": `zf_xiaobei`, `zf_xiaoni`, `zf_xiaoxiao`, `zf_xiaoyi`, `zm_yunjian`,
  `zm_yunxi`, `zm_yunxia`, `zm_yunyang`. `VOICES.md` also states verbatim: "**Support for
  non-English languages may be absent or thin due to weak G2P and/or lack of training
  data.**" and "**Weakness** on short utterances, especially less than 10-20 tokens."
  — https://huggingface.co/hexgrad/Kokoro-82M/blob/main/VOICES.md
- **Chinese phonemiser upstream = `misaki[zh]` = jieba + pypinyin (+ cn2an), NOT
  espeak-ng:** `pyproject.toml` extras `zh = ["jieba", "ordered-set", "pypinyin",
  "cn2an", "pypinyin-dict"]`; misaki README: "Jieba now cuts and tags, and
  pinyin-to-ipa is no longer used", with an unresolved TODO "**Homographs**: Escalate hard
  words like `axes bass bow lead tear wind` using BERT contextual word embeddings"
  — https://raw.githubusercontent.com/hexgrad/misaki/main/pyproject.toml ,
  https://raw.githubusercontent.com/hexgrad/misaki/main/README.md
  espeak-ng is used only for "English OOD fallback and some non-English languages"
  — https://raw.githubusercontent.com/hexgrad/kokoro/main/README.md
- **Published sizes.** Upstream HF repo: `kokoro-v1_0.pth` = **327,212,226 B**;
  `voices/` = 54 `.pt` files, each ≈**523,420–523,440 B** (e.g. `zf_xiaobei.pt`
  523,435 B). **No ONNX in the upstream repo.**
  — https://huggingface.co/api/models/hexgrad/Kokoro-82M/tree/main ,
  https://huggingface.co/api/models/hexgrad/Kokoro-82M/tree/main/voices
  Third-party ONNX builds with quantisation **and published quality deltas**
  (`thewh1teagle/kokoro-onnx`, release `model-files-v1.1`):
  `kokoro-v1.0.onnx` 325,505,369 B; `kokoro-v1.0.fp16.onnx` **163,527,961 B**
  ("spectral correlation 0.999 against fp32"); `kokoro-v1.0.int8.onnx` **114,119,327 B**
  ("0.916"); `kokoro-v1.1-zh.onnx` 325,506,167 B; `kokoro-v1.1-zh.fp16.onnx`
  **163,528,759 B** ("0.994"); `kokoro-v1.1-zh.int8.onnx` **114,120,125 B** ("0.874");
  `voices-v1.0.bin` 28,214,398 B; `voices-v1.1-zh.bin` 53,815,880 B. That project requires
  **ONNX Runtime ≥ 1.20.1** and claims "~300MB (quantized: ~80MB)" and "near real-time on
  macOS M1" with no RTF number
  — https://github.com/thewh1teagle/kokoro-onnx/releases/tag/model-files-v1.1 ,
  https://raw.githubusercontent.com/thewh1teagle/kokoro-onnx/main/README.md
- **Speed via sherpa-onnx (Raspberry Pi 4 Model B Rev 1.5):**
  `kokoro-multi-lang-v1_1` (311 MB) **7.635 / 4.470 / 3.430 / 3.191**; `kokoro-en-v0_19`
  (English, 330 MB) 6.629 / 3.870 / 2.999 / 2.774. Android arm64: **not published**
  — https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html
- **Android artifacts:** sherpa-onnx publishes a per-ABI TTS-Engine APK, e.g.
  `sherpa-onnx-1.13.7-arm64-v8a-eng-tts-engine-kokoro-multi-lang-v1_1.apk`; the C API and
  C++ API documented on the same page work from Qt with no Java
  — https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese-English/kokoro-multi-lang-v1_1.html
  Third-party Kotlin ports exist (`puff-dayo/Kokoro-82M-Android` GPL-3.0 and archived;
  `siva-sub/NekoSpeak` MIT; `Mobile-Artificial-Intelligence/maise` MIT)
  — https://api.github.com/repos/puff-dayo/Kokoro-82M-Android ,
  https://api.github.com/repos/siva-sub/NekoSpeak
- **PCM output:** 24000 Hz raw float PCM (`sf.write(..., 24000)`); sherpa-onnx writes mono
  16-bit PCM WAV.

---

## 7. ChatTTS (2noise/ChatTTS)

- **Licence — code AGPL-3.0, weights CC BY-NC 4.0, academic-only.** README verbatim:
  "The code is published under `AGPLv3+` license." / "The model is published under
  `CC BY-NC 4.0` license. It is intended for educational and research use, and should not
  be used for any commercial or illegal purposes." / "The released model is for academic
  purposes only." GitHub API `license.spdx_id = AGPL-3.0`; HF card YAML
  `license: cc-by-nc-4.0`
  — https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md ,
  https://api.github.com/repos/2noise/ChatTTS , https://huggingface.co/2Noise/ChatTTS
- **Architecture: autoregressive** (GPT + DVAE + Vocos). README FAQ 2: "This is a problem
  that typically occurs with autoregressive models (for bark and valle)". **No official
  paper published.**
- **Model files + exact sizes** (https://huggingface.co/api/models/2Noise/ChatTTS/tree/main/asset):
  `GPT.pt` **900,746,442 B**; `Embed.safetensors` 145,598,536; `Decoder.pt` 103,718,156;
  `DVAE_full.pt` 60,402,442; `Vocos.pt` 54,363,119; … **sum of `.pt` ≈ 1.25 GB**;
  **no fp16/int8 published**.
- **Chinese support claim:** README lists `[x] English`, `[x] Chinese`; "trained with
  Chinese and English audio data of 100,000+ hours"; the open release is 40,000-hour
  pre-trained without SFT.
- **Speed:** the only published figure is GPU-only: "For a 30-second audio clip, at least
  4GB of GPU memory is required. For the 4090 GPU, it can generate audio corresponding to
  approximately 7 semantic tokens per second. The Real-Time Factor (RTF) is around 0.3."
  **CPU/ARM RTF: not published.**
- **Android feasibility: none found.** Roadmap item "- [ ] ChatTTS.cpp (new repo in
  `2noise` org is welcomed)" is **unchecked**, and
  `https://api.github.com/repos/2noise/ChatTTS.cpp` → `{"message":"Not Found"}`.
- **Chinese front-end is not self-contained:** needs `pynini`, `WeTextProcessing` and
  `nemo_text_processing` for `normalizer_zh_tn()`
  — https://github.com/2noise/ChatTTS/pull/178 ,
  https://github.com/2noise/ChatTTS/issues/906
- **PCM output:** raw float array; saved at **24000 Hz**.

---

## 8. PaddleSpeech (PaddlePaddle/PaddleSpeech)

- **Runtime dependency: PaddlePaddle required.** README "PaddleSpeech depends on
  paddlepaddle". Three engines exist in code: Paddle dynamic graph, Paddle Inference
  (`from paddle import inference`), ONNX Runtime (`import onnxruntime as ort`)
  — https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/exps/syn_utils.py
- **ONNX export is official/first-party** (`examples/csmsc/tts3/local/paddle2onnx.sh`,
  `--use_onnx true`, changelog "2022.06.22: All TTS models support ONNX format"),
  **but the ONNX path still imports paddlepaddle in Python**
  — https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/examples/csmsc/tts3/local/paddle2onnx.sh ,
  https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/cli/tts/infer.py
- **Architecture:** fastspeech2 is **non-autoregressive** ("feed-forward structure, which
  can generate a target mel spectrogram sequence in parallel"); tacotron2/TransformerTTS
  are autoregressive
  — https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/tts/models_introduction.md
- **Chinese models — only one size per row is published, headed "Size (static)";
  checkpoint / ONNX / Paddle-Lite archive sizes are not published** (the registry gives
  md5 only) — https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/released_model.md :
  `fastspeech2_csmsc` **157 MB**; `fastspeech2_cnndecoder_csmsc` 84 MB;
  `fastspeech2_aishell3` **147 MB**; `fastspeech2_canton` 146 MB;
  `fastspeech2_male` zh/ljspeech/vctk/zh_en-mix 146/145/145/145 MB;
  `speedyspeech_csmsc` **13 MB**; `tacotron2_csmsc` 103 MB;
  vocoders `pwgan_csmsc` **4.8 MB**, `mb_melgan_csmsc` **7.6 MB**, `hifigan_csmsc` 46 MB,
  `wavernn_csmsc` 18 MB. ONNX zips exist for most (e.g.
  `fastspeech2_csmsc_onnx_0.2.0.zip`, `hifigan_csmsc_onnx_0.2.0.zip`).
- **Chinese front-end:** `jieba.posseg`, `g2pM`, `pypinyin` (+ large-pinyin dict),
  `G2PWOnnxConverter`, `ToneSandhi`, `TextNormalizer`; default `g2p_model="g2pW"` with
  `assert g2p_model in ('pypinyin','g2pM','g2pW')`. g2pW runs an **ONNX BERT** for
  polyphone disambiguation (`onnxruntime.InferenceSession(<dir>/g2pW.onnx)`, "modified from
  https://github.com/GitYCC/g2pW")
  — https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/frontend/zh_frontend.py ,
  https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/frontend/g2pw/onnx_api.py
  Docs verbatim: "We use g2pM and pypinyin as the default g2p tools. They can solve the
  problem of polyphones to a certain extent… However, g2pM and pypinyin do not perform
  well in tone sandhi, we use rules to solve this problem"
  — https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/tts/zh_text_frontend.md
  `G2PWModel_1.1.zip` size for the ONNX g2pW model: **not published**.
- **Published speed with named hardware:** the wiki TTS-Benchmark uses
  `8x Tesla V100-SXM2-32GB, 24 core Intel Xeon Gold 6148`, frontend excluded, Paddle 2.2:
  fastspeech2 am-only 0.01358 GPU / 0.07090 CPU; +pwgan 0.03191 / **2.371**;
  +mb_melgan 0.01831 / **0.2733**; +hifigan 0.0218 / **1.7128**. Streaming
  (fastspeech2_cnndecoder_onnx + hifigan_onnx, ORT 1.10.0, cpu_thread=4) RTF
  **0.488 / 0.623 / 0.580** on three named Intel CPUs (Xeon E5-2680 v4, i5-8250U, i5-8257U)
  — https://raw.githubusercontent.com/wiki/PaddlePaddle/PaddleSpeech/TTS-Benchmark.md .
  The PPTTS doc claims "RTF < 1 on low-voltage" with **hardware not specified**
  — https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/tts/PPTTS.md .
  **Android arm64 RTF: not published.**
- **Android support exists but needs Paddle Lite + Gradle + Java, and ships no text
  front-end:** `demos/TTSAndroid` (Android Studio + NDK, "该过程会自动下载 Paddle Lite
  预测库和模型", Paddle Lite Java API, `PaddlePredictor.jar`,
  `jniLibs/arm64-v8a/libpaddle_lite_jni.so`, models `.nb`) and states verbatim:
  "**本 Demo 不包含文本前端模块 … 如需文本前端模块请自行处理**"
  — https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/demos/TTSAndroid/README.md
  `demos/TTSArmLinux` uses the Paddle Lite **C++** API with libs pinned to a Paddle-Lite
  commit and states "目前只支持中文合成，出现任何英文都会导致程序崩溃".
- **C++ inference without Paddle: not documented.** Every in-repo C++ path needs a Paddle
  runtime; ONNX inference is documented **only in Python** — there is no in-repo C++
  onnxruntime demo, CMake target or doc page.
- **Licence:** code **Apache-2.0** (https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/LICENSE).
  **Weights: no separate model licence is published** — `released_model.md` has no licence
  column, the registry carries url/md5 only, and no `NOTICE` was found. Commercial terms
  and dataset licences: **not published**.
- **PCM output:** 24 kHz sample-rate WAV by default ("Output 24k sample rate wav format
  audio"; CLI `--fs` default 24000); the streaming server emits **raw int16 PCM**
  — https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/server/engine/tts/online/python/tts_engine.py

---

## 9. ekho (hgneng/ekho)

- **Licence: code GPL-2.0-or-later** — `COPYING` verbatim "either version 2 of the
  License, or any later version", Copyright "Cameron Wong … 2008-2022"
  — https://raw.githubusercontent.com/hgneng/ekho/master/COPYING
- **Voice data is licenced separately, and Mandarin has none.** `COPYING`: "For copyright
  information of voice data of Ekho, please refer their copyright description files that
  begin with `COPYING-`." Per-voice files exist for Cantonese, Hakka, Korean, Ngangien and
  Toisanese — **there is no `COPYING-*` for `ekho-data/pinyin/` (Mandarin)**, which holds
  only a `README`. **Mandarin voice-data licence: not published.**
- **Synthesis = concatenative unit concatenation, explicitly documented:**
  `ekho-data/pinyin/README` states "为每一个拼音录制一个音频文件，然后拼接起来合成句子"
  (one recorded audio file per pinyin syllable, concatenated)
  — https://raw.githubusercontent.com/hgneng/ekho/master/ekho-data/pinyin/README
- **Mandarin + Cantonese supported:** voice aliases `Mandarin`/`zh`/`cmn` → `pinyin`;
  `Cantonese`/`yue` → `jyutping`. In-repo Mandarin data: `pinyin.voice` **1,803,172 B**,
  `pinyin.index` **16,395 B**, `zh.dict` 703,508 B, `zh_list` 36,496 B,
  `zh_listx` 1,256,746 B. **Published downloadable voice-data archives**
  (SourceForge "Ekho Voice Data 0.2", 16 items / 767.5 MB):
  `pinyin-yali-44100-wav-v10.tar.xz` **51.8 MB**; `pinyin-huang-44100-wav-v3.tar.xz`
  **86.3 MB**; `pinyin-huang-16000-wav.tar.xz` **34.2 MB**;
  `pinyin-yali-16000-1x.tar.bz2` **16.6 MB**; `pinyin-yali-44100.tar.bz2` 43.0 MB
  — https://sourceforge.net/projects/e-guidedog/files/Ekho-Voice-Data/0.2/
- **Not dependency-free.** `configure.ac` makes **libcurl**, **libsndfile**,
  **espeak-ng** and (by default) **pulseaudio** mandatory hard failures; `Makefile.am`
  proves unconditional linkage `-lcurl -lsndfile -lespeak-ng`
  — https://raw.githubusercontent.com/hgneng/ekho/master/configure.ac ,
  https://raw.githubusercontent.com/hgneng/ekho/master/Makefile.am
- **Android / arm64: no Android build files in master** — no `Android.mk`, no
  `AndroidManifest.xml`, no `jni/`, no `CMakeLists.txt`; autotools only; **no
  arm64/aarch64 cross-compile option**; Android appears only as
  `#ifdef DEBUG_ANDROID LOGD(...)` in `src/audio.cpp`. eGuideDog does list a
  `ekho-cantonese-9.0.apk (22M)` and says the voice data is installed separately because
  "语音数据文件较大，没有包含在APK包中"
  — https://api.github.com/repos/hgneng/ekho/git/trees/master?recursive=1 ,
  https://www.eguidedog.net/ekho.php
- **`hgneng/ekho-android` does not exist**: `https://api.github.com/repos/hgneng/ekho-android`
  → HTTP 404. The successor is **`hgneng/ekho-android-cantonese`** (GPL-2.0), which **does**
  have a working Android build: `jni/Android.mk` builds `libsndfile` statically from
  vendored sources, then the ekho sources out of tree, with
  `LOCAL_MODULE := libttsekho`; `build.gradle` declares `minSdkVersion 21`,
  `compileSdk 35`, and `abiFilters 'arm64-v8a', 'armeabi-v7a', 'x86', 'x86_64'`
  — https://api.github.com/repos/hgneng/ekho-android-cantonese ,
  https://raw.githubusercontent.com/hgneng/ekho-android-cantonese/main/jni/Android.mk ,
  https://raw.githubusercontent.com/hgneng/ekho-android-cantonese/main/build.gradle
- **Sample rate:** **no fixed output rate for Chinese.** `EkhoImpl::initStream()` sets
  16000 Hz only for the English path; otherwise it uses the rate from the voice data and
  errors with "Sample rate not detected: " if 0. Published downloadable Mandarin data is
  16000 Hz and 44100 Hz. A single canonical rate for the in-repo `pinyin.voice`:
  **not published**
  — https://raw.githubusercontent.com/hgneng/ekho/master/src/libekho_impl.cpp
- **PCM output:** WAV export is `SF_FORMAT_WAV | SF_FORMAT_PCM_16`; rate as above.

---

## 10. eSpeak-NG used directly (espeak-ng/espeak-ng)

- **Licence: GPL-3.0-or-later** — `COPYING` is the verbatim GPLv3 text; README "released
  under the [GPL version 3](COPYING) or later license." **No linking exception and no
  library-specific licence statement is published** — the FSF boilerplate about
  proprietary programs/LGPL inside `COPYING` is licence text, not an eSpeak-NG project
  statement — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/COPYING ,
  https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md
- **Officially buildable for Android** from the main repo: `docs/building.md` has an
  `## Android` section (Android Studio, NDK, Gradle 8.13+, JDK 17,
  `cd android && ./gradlew assembleRelease`); `android/jni/CMakeLists.txt` builds libsonic
  from source, adds the espeak-ng subdirectory, creates an `espeak-data` target, and builds
  `libttsespeak.so`; `android/build.gradle` declares `minSdk 21`, `compileSdk 36`,
  `ndkVersion "29.0.14206865"`, and a native-args set including `-DUSE_ASYNC:BOOL=OFF`
  — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/building.md ,
  https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/jni/CMakeLists.txt ,
  https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/build.gradle
- **Library target:** `add_library(espeak-ng …)` in `src/libespeak-ng/CMakeLists.txt`;
  the produced file is `libespeak-ng`. **`BUILD_SHARED_LIBS` defaults OFF (static)**;
  sources compile with `-fPIC -fvisibility=hidden`
  — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/libespeak-ng/CMakeLists.txt
- **Cross-compilation gotcha, documented:** "Because the eSpeak NG build process uses the
  built program to compile the language and voice data, you need to build it natively
  first… `-DNativeBuild_DIR=build/src`", and the top-level CMake emits "Not building
  intonations as the build is a cross compile."
  — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/building.md
- **No prebuilt Android library is published.** Release assets are APK/MSI/tarball only —
  `1.52.0` ships `espeak-1.52.0-signed.apk` = 10,446,659 B and `espeak-ng.msi`; `1.51`
  ships `espeak-ng-1.51.tar.gz` and MSIs. **No `.so` and no `.aar`**
  — https://api.github.com/repos/espeak-ng/espeak-ng/releases
- **`espeak-ng-data` size is not published by the project** (README claims only "Compact
  size. The program and its data, including many languages, totals about few Mbytes"). An
  independent reference: Debian `espeak-ng-data` 1.51+dfsg-10+deb12u2 = 4,156.4 kB package
  / 11,782.0 kB installed, same for amd64 and arm64 (note Piper uses a reduced-feature
  build) — https://packages.debian.org/bookworm/espeak-ng-data .
  Cross-reference: sherpa-onnx's shared `espeak-ng-data.tar.bz2` = 7,252,012 B.
- **Chinese front-end:** language file at **`espeak-ng-data/lang/sit/cmn`** with
  `language cmn` / `language zh-cmn` / `language zh`, `phonemes cmn`, `dictionary cmn`,
  `dict_min 100000`
  — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/lang/sit/cmn
  - `dictsource/cmn_rules` (3,997 B) holds pinyin→phoneme rules and the tone map
    `1→55, 2→35, 3→214, 4→51, 5→11`, with an in-file `// TODO: àn is not handled`
    — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_rules
  - `dictsource/cmn_list` (36,237 B) is a built-in **Han→pinyin** table: header "Most
    frequent pronunciations of the 3799 most common characters (from Unihan … kHanyuPinlu
    field with some corrections)", one character per line → pinyin + tone digit; therefore
    **single-reading by construction**
    — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_list
  - **An extended pinyin dictionary exists: `dictsource/extra/cmn_listx`** — "21611 single
    characters plus **36500 compound exceptions** (includes 320 added 'yi' and 10721 added
    'bu' exceptions, and 9700 extra 2-syllable words for 3rd-tone sandhi blocking)".
    Extended dicts are **ON by default** (`EXTRA_cmn`)
    — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/extra/cmn_listx ,
    https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/cmake/data.cmake
  - `espeak-ng-data/cmn_dict` is **not checked in** — it is generated at build time
    (`cmake --build build --target data`, or `espeak_CompileDictionary`). As shipped in a
    real bundle it is **1,566,335 B**
    — https://huggingface.co/api/models/csukuangfj/vits-piper-zh_CN-huayan-medium/tree/main/espeak-ng-data
  - **No word segmentation** (no jieba/pypinyin): maintainer PR #2455 states each hanzi
    "must be a separate word so that words can match their multi-word `*_list` entries"
    — https://github.com/espeak-ng/espeak-ng/pull/2455
  - A build flag **`--without-extdict-cmn`** exists
    — https://github.com/espeak-ng/espeak-ng/issues/1665
- **Raw PCM API (exact signatures)** — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/speak_lib.h :
  - `ESPEAK_API int espeak_Initialize(espeak_AUDIO_OUTPUT output, int buflength, const char *path, int options);`
    — "**Returns: sample rate in Hz**, or -1 (EE_INTERNAL_ERROR)"; `buflength` is "The length
    in mS of sound buffers passed to the SynthCallback function."
  - `typedef int (t_espeak_callback)(short*, int, espeak_EVENT*);` and
    `ESPEAK_API void espeak_SetSynthCallback(t_espeak_callback* SynthCallback);` —
    "wav: … **NULL indicates that the synthesis has been completed.**"; "numsamples: … may
    sometimes be zero (which does NOT indicate end of synthesis)"; "Callback returns:
    0=continue synthesis, 1=abort synthesis."
  - **There is NO `espeak_ng_SetSynthCallback`.** The only callback setters in both public
    headers are `espeak_SetSynthCallback`, `espeak_SetUriCallback`,
    `espeak_SetPhonemeTrace`, plus new-GI `espeak_ng_SetOutputHooks(...)`
    — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/espeak_ng.h
  - `ESPEAK_API espeak_ERROR espeak_Synth(const void *text, size_t size, unsigned int position,
    espeak_POSITION_TYPE position_type, unsigned int end_position, unsigned int flags,
    unsigned int* unique_identifier, void* user_data);` — "**The command is asynchronous:
    it is internally buffered and returns as soon as possible.**"; plus `espeak_Cancel`,
    `espeak_Synchronize`, `espeak_Terminate`.
  - In-tree reference JNI bridge pattern:
    `espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 300, c_path, 0)` →
    `espeak_SetSynthCallback(SynthCallback)` → `espeak_Synth(..., espeakCHARS_UTF8, ...)` →
    `espeak_Synchronize()`; the callback copies `numSamples * 2` bytes of **S16LE mono**
    — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/jni/jni/eSpeakService.c
- **Sample rate:** `src/libespeak-ng/speech.c` has `static int voice_samplerate = 22050;`
  and `int srate = 22050; // default sample rate 22050 Hz` then `LoadPhData(&srate, ...)`
  and `WavegenInit(srate, 0)` — **phoneme data can override**, the rate is per-voice and can
  change at runtime via `espeakEVENT_SAMPLERATE`
  — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/libespeak-ng/speech.c
- **Synthesis method and the project's own quality statement, verbatim:** "eSpeak NG uses a
  'formant synthesis' method. This allows many languages to be provided in a small size.
  **The speech is clear, and can be used at high speeds, but is not as natural or smooth as
  larger synthesizers which are based on human speech recordings.** It also supports Klatt
  formant synthesis, and the ability to use MBROLA as backend speech synthesizer."
  Diphone synthesis is only via the optional MBROLA backend. **No Mandarin-specific quality
  statement is published** — `docs/languages.md` lists only
  `| sit | cmn | Sino-Tibetan | Chinese | Mandarin |`
  — https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md ,
  https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages.md

---

## 11. Other credible offline Chinese TTS usable from C++ on Android

**CosyVoice (FunAudioLLM/CosyVoice)** — open source, **autoregressive LLM**, PyTorch.
"based on large language models (LLM)"; runtimes are vLLM / Docker+NVIDIA / TensorRT-LLM.
Code **Apache-2.0**; README Disclaimer "The content provided above is for academic
purposes only and is intended to demonstrate technical capabilities"; weights licence text
**not published**. Param counts published (0.5B / 0.5B / 300M) but **on-disk file sizes are
not published**. Latency "as low as 150ms" with **hardware not stated**. **No ONNX export,
no C++ runtime, no Android path documented**, and sherpa-onnx has no CosyVoice page
— https://raw.githubusercontent.com/FunAudioLLM/CosyVoice/main/README.md ,
https://k2-fsa.github.io/sherpa/onnx/tts/index.html

**Matcha-TTS / Vocos** — Matcha-TTS code **MIT**, paper: "The method is probabilistic,
**non-autoregressive**, and learns to speak from scratch without external alignments";
upstream Chinese support **not published**; weights licence **not published**. Vocos code
**MIT**, paper claims "an **order of magnitude increase in speed** compared to prevailing
time-domain neural vocoding approaches"; weights licence **not published**. Android route =
sherpa-onnx's `matcha-icefall-zh-baker` only
— https://raw.githubusercontent.com/shivammehta25/Matcha-TTS/main/LICENSE ,
https://arxiv.org/abs/2309.03199 , https://raw.githubusercontent.com/charactr-platform/vocos/main/LICENSE ,
https://arxiv.org/abs/2306.00814

**piper-plus (ayutaz/piper-plus)** — **MIT**, espeak-ng-free, includes Chinese (`zh=2`),
ships a C API shared library `libpiper_plus.so`. Uses its **own phone set, incompatible with
upstream piper voices**. Hardware-named benchmark (Intel Xeon E5-2650 v4 @2.20GHz, 48 cores,
Linux x86_64, ORT 1.24, **English sentence only**): piper-plus MB-iSTFT RTF **0.078** /
38 MB vs upstream Piper `en_US-lessac-medium` **0.066** / 60 MB vs sherpa-onnx
`vits-piper-en_US-amy-low` **0.075** / 60 MB. **No Chinese RTF published, and no Android
`.so`/AAR release asset documented** (release list is windows-x64, macos-arm64, linux-x64,
linux-arm64)
— https://raw.githubusercontent.com/ayutaz/piper-plus/dev/README.md

**F5-TTS (SWivid/F5-TTS)** — open source **MIT**; 0.3B params per CosyVoice's comparison
table, test-zh CER 1.52 %; AR/non-AR status not stated on the retrieved pages; **no
ONNX/Android port published**.

**IndexTTS / IndexTTS2** — open weights but **not standard OSS**: "bilibili Model Use
License Agreement" requiring a separate written licence if you or affiliates had
**>100M MAU** in the prior calendar month or **>RMB 1B** annual revenue in the prior year;
§4.2 prohibits high-risk deployments (medical, autonomous driving, military, critical
infrastructure, large-scale biometric surveillance, automated decision-making) without
independent compliance assessment; §6 PRC law + Shanghai Arbitration Commission. Issue #228
asks for licence clarification
— https://raw.githubusercontent.com/index-tts/index-tts/main/LICENSE ,
https://github.com/index-tts/index-tts/issues/228

**Muyan-TTS** — **English only, excluded**: "Note: Muyan-TTS only supports English input
since the training data is heavily skewed toward English." Autoregressive LLM (Llama-3.2-3B
+ SoVITS decoder); single A100 40GB, r = 0.33 s compute per second of audio; no
ONNX/C++/Android path; licence **not published**
— https://raw.githubusercontent.com/MYZY-AI/Muyan-TTS/main/README.md

**"Chinese-TTS-ONNX"** — no credible primary-source project of that exact name was found →
**not verified**.

**iFlytek 讯飞离线语音合成 (AIkit) — commercial closed SDK, closed source, Java/AAR + Gradle.**
Capability "普通品质aisound合成能力，支持中英发音人". Authorisation terms, verbatim:
"授权方式支持【设备授权】和【应用授权】2种。… **在线激活**: 在首次使用时，需要将设备联网，
SDK初始化时获取授权license激活。设备激活后，即可在无网环境下使用…" — i.e.
**first use requires network connectivity**, and clearing app storage invalidates the
licence until back online. Integration is Java-only: `implementation files('libs/AIKit.aar')`
in `build.gradle`. Permissions include `INTERNET`, `READ_PHONE_STATE`,
`READ_PRIVILEGED_PHONE_STATE`, `READ_PHONE_NUMBERS`, `MANAGE_EXTERNAL_STORAGE`,
`MOUNT_UNMOUNT_FILESYSTEMS`. Android 5.0–13. **Output: frames; `encoding` ∈ {lame, speex,
opus, speex-wb} (default `speex-wb`), `sample_rate` ∈ {16000, 8000} (default 16000),
`channels` ∈ {1,2}, `bit_depth` ∈ {16,8}.** Voices include `xiaoyan`, `xiaofeng`,
`xiaomeng`, `xiaoqiang`, `xiaolin`, `xiaorong` (Sichuan), `xiaoqian` (Northeast), `nannan`
(child), `xiaomei` (Cantonese), English `catherine`/`john`. **Pricing/royalty amounts: not
published**
— https://www.xfyun.cn/doc/tts/AIKit_offline_tts/Android-SDK%28Lightweight%29.html

---

## Comparison table

"JNI needed" answers the question *for the engine's own C++ API*, not for Qt's wrapper.
"Speed" reproduces only published figures; the Raspberry Pi 4 Model B Rev. 1.5 column is
the only hardware-named Chinese TTS measurement in the whole survey.

| Engine | Runtime | Chinese model + published size | Front-end method | Speed (published; RPi 4 unless noted) | Licence (code / weights) | JNI needed | Integration effort |
|---|---|---|---|---|---|---|---|
| **sherpa-onnx** | ONNX Runtime (separate dep; 1.28.2 default; 33,765,715 B multi-ABI zip; 15 MB arm64 `libonnxruntime.so`) | `vits-icefall-zh-aishell3` 30 MB @8k; `matcha-icefall-zh-baker` 72 MB + 51 MB vocoder @22.05k; `vits-zh-hf-fanchen-C` 116 MB @16k; `theresa`/`eula` 117 MB @22.05k; `vits-melo-tts-zh_en` 163 MB @44.1k; `kokoro-multi-lang-v1_1` 311 MB @24k (int8 variant published); `vits-piper-zh_CN-*` (22.05k) | **pinyin lexicon (lexicon.txt) + longest-match segmentation** + phone/date/number FSTs; espeak-ng only for Piper/Kokoro-derived models | RTF **0.156** (aishell3, 4 thr) / **0.391** (matcha-baker, 4 thr) / 1.60–2.52 (other VITS) / 3.19 (kokoro v1_1); **no Android figure** | Apache-2.0 / **per-model**: matcha-zh-baker non-commercial dataset; others vary | No (C API; JNI `.so` exists but is not needed) | Prebuilt Android tarball + AAR exist; C/C++ API, CMake, docs, example code per model; espeak-ng compiled in; **no Python step for shipped models** |
| **Piper** | ONNX Runtime (default 1.22.0) + statically built espeak-ng | `zh_CN-huayan-x_low` **20,628,813 B** @16k; `huayan-medium` **63,201,294 B**; `chaowen-medium` **63,221,984 B**; `xiao_ya-medium` **63,221,984 B** (all 22.05k except x_low). No official fp16/int8 | **Two paths**: `huayan` → espeak-ng `cmn`; `xiao_ya`/`chaowen` → pinyin + g2pW (g2pW fetched at runtime; C++ lib uses first-reading only, Phase 2 unshipped) | **not published for any zh_CN voice**; no Android figure | **GPL-3.0** (current) / MIT (archived legacy); weights: repo-level MIT but per-voice `xiao_ya` **non-commercial**, `huayan` **unknown**, `chaowen` CC0; docs say "personal use and TTS research only" | No (libpiper C ABI) | CMake lib + C header; **no official Android build** (no ANDROID branch in CMake, no NDK/CI, no `.so`/`.aar` assets); requires bundled espeak-ng-data; third-party Android builds only |
| **Android system TTS** | Vendor engine (framework API) | n/a — Mandarin availability depends entirely on the installed engine | Engine-specific; **not inspectable from the app** | **not published** (engine-declared `Voice.getLatency()` labels only: "<20ms" … ">200ms") | Framework: Apache-2.0 (AOSP); engine licences vary | **Java required in practice** — `TextToSpeech` ctor needs `OnInitListener`, progress needs `UtteranceProgressListener`; Qt's own backend ships a Gradle-built Java class (minSdk 28) | Manifest `<queries>` for `INTENT_ACTION_TTS_SERVICE` on API 30+; **offline only if the installed engine declares `embeddedTts` and exposes a voice with `isNetworkConnectionRequired() == false`**; voice data may need user download (`INSTALL_TTS_DATA`) |
| **Qt TextToSpeech** | Wraps the platform engine (Android: `android.speech.tts`) | n/a — brings **no** Mandarin synthesiser on Android | Delegated to the Android engine | **not published** | Qt commercial / LGPL-3.0 / GPL-2.0 per module page; headers say LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only | **Java ships inside Qt** — plugin `add_dependencies` on a Gradle-built `Qt6AndroidTextToSpeech` jar (minSdk 28) | `Qt6::TextToSpeech`, add-on module; raw PCM via `synthesize()` (since Qt 6.6, needs `Capability::Synthesize`); **no `setAudioOutput()`**; no `PauseResume`; documented zip for flite/winrt only |
| **MeloTTS** | PyTorch + transformers (+ BERT) | `myshell-ai/MeloTTS-Chinese` `checkpoint.pth` **207,770,124 B** + BERT `hfl/chinese-roberta-wwm-ext-large` **1,306,484,351 B**; sherpa-onnx port `model.onnx` **163 MB** (BERT zeroed out) | cn2an + pypinyin + jieba.posseg + custom ToneSandhi + opencpop-strict map + **BERT** | RTF **6.727 / 3.877 / 2.914 / 2.518** (1/2/3/4 thr) via sherpa-onnx; upstream claims "CPU real-time" with no hardware; no Android figure | MIT / MIT (BERT separately apache-2.0) | No direct C++ path; only via sherpa-onnx | Needs a **Python export step**; exporter discards the pronunciation-relevant context encoder |
| **Kokoro** | ONNX Runtime (third-party onnxruntime builds need ≥1.20.1); or PyTorch upstream | v1.1-zh: 82 M params, 103 voices (**100 Chinese**), 24000 Hz. Upstream `kokoro-v1_0.pth` 327,212,226 B; third-party ONNX `kokoro-v1.1-zh.fp16.onnx` **163,528,759 B** (0.994 spectral corr.), `…int8.onnx` **114,120,125 B** (0.874), `voices-v1.1-zh.bin` 53,815,880 B; sherpa-onnx v1_0 `model.onnx` 310 MB + `voices.bin` 26 MB, v1_1 311 MB + int8 variant | Upstream `misaki[zh]` = **jieba + pypinyin + cn2an** (not espeak-ng); sherpa-onnx uses `lexicon-zh.txt` | RTF **7.635 / 4.470 / 3.430 / 3.191** via sherpa-onnx (311 MB); "near real-time on macOS M1" claimed by kokoro-onnx with no number; no Android figure | Apache-2.0 weights / Apache-2.0 (kokoro-onnx MIT) | No (via sherpa-onnx C API / kokoro-onnx) | Upstream 8 Mandarin voices graded **"D"**; VOICES.md warns non-English support "may be absent or thin due to weak G2P"; sherpa-onnx ships per-ABI TTS-engine APKs |
| **ChatTTS** | PyTorch | `GPT.pt` **900,746,442 B**; sum of `.pt` ≈ 1.25 GB; no fp16/int8 published | pynini + WeTextProcessing/nemo normaliser for Chinese; a 16,000-entry `homophones_map.json` post-hoc pronunciation patch | RTF **~0.3 on RTX 4090** (30-s clip, ≥4 GB VRAM); CPU/ARM **not published** | **AGPL-3.0 code / CC BY-NC 4.0 weights, academic-only** | n/a | **No C++/ONNX/Android port** (roadmap item unchecked; `ChatTTS.cpp` 404) |
| **PaddleSpeech** | PaddlePaddle (or ONNX Runtime, but the ONNX Python path still imports paddle) | `fastspeech2_csmsc` **157 MB** (static); `fastspeech2_aishell3` **147 MB**; `fastspeech2_cnndecoder_csmsc` (streaming) 84 MB; `speedyspeech_csmsc` **13 MB**; vocoders 4.8–46 MB (MB-MelGAN 7.6 MB); ONNX zips published; ckpt/ONNX sizes **not published** | jieba.posseg + g2pM + pypinyin + **g2pW ONNX BERT** polyphone disambiguation + ToneSandhi rules | streaming ONNX (4 CPU threads) RTF **0.488 / 0.623 / 0.580** on three named Intel CPUs; +MB-MelGAN full pipeline RTF 0.2733 on Xeon Gold 6148; **no Android figure** | Apache-2.0 code / **weights licence not published** | Android demo uses Paddle Lite **Java** API + Gradle + `.nb` models; **no text front-end shipped** ("本 Demo 不包含文本前端模块") | C++ inference without a Paddle runtime is **not documented**; the only C++ Android demo crashes on any English input |
| **ekho** | Custom C++ (autotools), mandatory libcurl + libsndfile + espeak-ng + pulseaudio | Mandarin `pinyin.voice` 1,803,172 B + `pinyin.index` 16,395 B; downloadable voice data `pinyin-yali-44100-wav-v10` **51.8 MB**, `pinyin-huang-44100-wav-v3` **86.3 MB**, `pinyin-huang-16000-wav` 34.2 MB, `pinyin-yali-16000-1x` 16.6 MB | Concatenation of one recorded syllable per pinyin (documented in `ekho-data/pinyin/README`); `zh_list`/`zh_listx` dictionaries | **not published** | **GPL-2.0-or-later** code / **Mandarin voice-data licence not published** (no `COPYING-*` for `ekho-data/pinyin/`) | No (C++), but Android needs the third-party `ekho-android-cantonese` build | Master has **no Android build files and no arm64 cross-compile**; `hgneng/ekho-android` 404s; sample rate not fixed for Chinese |
| **eSpeak-NG directly** | Custom C (formant synthesis) | `espeak-ng-data` size **not published** (Debian reference: 4,156.4 kB package / 11,782.0 kB installed); compiled `cmn_dict` 1,566,335 B | `cmn_rules` (pinyin→phoneme, tone map 55/35/214/51/11) + `cmn_list` (3,799 single-reading characters) + `extra/cmn_listx` (21,611 chars + 36,500 compound exceptions, ON by default); **no word segmentation** | **not published** | **GPL-3.0-or-later**, no linking exception published; data under the same licence | No (C library `libespeak-ng`, `BUILD_SHARED_LIBS` defaults OFF) | Official Android build exists (Gradle + NDK, minSdk 21) but **no prebuilt `.so`/`.aar` published**; cross-compiling requires a native `NativeBuild_DIR` first; **output is robotic formant synthesis by the project's own statement** |
| **CosyVoice** | PyTorch (+ vLLM/TensorRT runtimes) | 0.5B/0.5B/300M params; **on-disk sizes not published** | LLM-based | "as low as 150ms" latency, **hardware not stated** | Apache-2.0 code / weights licence text **not published**; "academic purposes only" disclaimer | n/a | **No ONNX/C++/Android path**; autoregressive |
| **piper-plus** | ONNX Runtime | Chinese included (`zh=2`); own phone set, **incompatible with upstream piper voices**; sizes **not published** | Own G2P; espeak-ng-free | RTF **0.078** (MB-iSTFT, Xeon E5-2650 v4, **English sentence only**) | **MIT** | No (ships `libpiper_plus.so`) | Release assets are desktop-only; **no Android `.so`/AAR documented** |
| **iFlytek AIkit offline** | Closed proprietary SDK | Voice list published; **sizes not published** | Proprietary | **not published** | **Closed source, commercial**; pricing **not published** | **Java required** (AAR + Gradle) | **Network activation required on first use**; permissions include phone state and `MANAGE_EXTERNAL_STORAGE`; output is an encoded frame stream (speex-wb 16 kHz default), not raw PCM |

---

## Known quality problems reported for Chinese (concrete, cited)

### sherpa-onnx / its Chinese model zoo
| Report | Concrete complaint | Status / maintainer response |
|---|---|---|
| [#2904](https://github.com/k2-fsa/sherpa-onnx/issues/2904) | Probabilistic synthesis errors on aarch64 Kunpeng 920: 剩余燃油零千克, **发动机总扭矩为百分之零** (矩 dropped), 旋翼转速百分之零, 风速零公里每小时, 计时已开始, 虚拟助手已启动; also 方案 → 方方. Occurs at `num_threads` 1, 4 and 16; also reproduced on the model author's demo site (1/5–1/10 rate). | Maintainer `csukuangfj` (2025-12-19), verbatim: "这个属于**模型的问题**, sherpa-onnx 层面无法解决" — declared a **model** defect, not an engine defect. |
| [#2902](https://github.com/k2-fsa/sherpa-onnx/issues/2902) | Original report of the same failure class. | open |
| [#2706](https://github.com/k2-fsa/sherpa-onnx/issues/2706) | Output worse than the model author's demo specifically around particles 着/的; user attached a sherpa-onnx WAV and a reference MP3. | closed 2025-12-04 |
| [#2755](https://github.com/k2-fsa/sherpa-onnx/issues/2755) | "matcha_tts_zh_en_20251010，是中英混合模型，但是生成音频的时候，英文发音是**完全听不清**的" | closed after PR [#2763](https://github.com/k2-fsa/sherpa-onnx/pull/2763) |
| [#2730](https://github.com/k2-fsa/sherpa-onnx/issues/2730) | "无法播报英文内容会自动过滤英文" | closed 2025-11-24 |
| [#1552](https://github.com/k2-fsa/sherpa-onnx/issues/1552) | "当中文中遇到长段的英文时会明显出现性能下降"; "英文发音并不好，很多单词不会发音，看起来和 cum 字典有关" | open |
| [#2960](https://github.com/k2-fsa/sherpa-onnx/issues/2960) | MeloTTS ONNX (zh_en and en) cannot pronounce words containing "v", on Linux **and Android**; reporter suspects the export step since MeloTTS' own demo is fine. | open |
| [#2726](https://github.com/k2-fsa/sherpa-onnx/issues/2726) | kokoro-multi-lang-v1_1 "会把中文句号读成 dot" | open |
| [#2004](https://github.com/k2-fsa/sherpa-onnx/issues/2004) | "Incorrect phoneme handling (Kokoro-TTS)" | open |
| [#2325](https://github.com/k2-fsa/sherpa-onnx/issues/2325) | `sherpa-onnx-offline-tts-play-alsa` truncated playback of a long Chinese paragraph while the saved WAV was complete — a streaming-playback defect. | open |
| [#468](https://github.com/k2-fsa/sherpa-onnx/issues/468) | Chinese output broken on Windows unless UTF-8 is enabled; cited by the TTS FAQ ("如果英文模型正常，中文模型不正常") | referenced by docs |
| [#2961](https://github.com/k2-fsa/sherpa-onnx/issues/2961) | The pinyin phrase-replacement feature drops characters: "广州市场" became "广州市"; the reporter found that removing multi-character entries from `lexicon.txt` fixed it. | open (ASR homophone-replacer, but the same lexicon/segmentation machinery) |

**No issue dedicated to Mandarin tone (声调) errors was found in the sherpa-onnx tracker.**

### eSpeak-NG Mandarin
| Report | Concrete complaint | Status |
|---|---|---|
| [#606](https://github.com/espeak-ng/espeak-ng/issues/606) | "Chinese dictionary multiple match" — 地面 matched a multi-word rule `(地 面) di4mian4`, then 面 was re-translated anyway (`ti53m'iE51n_\| m'iE51n_\|`); root cause given as `dictionary.c:LookupDict2` setting `dictionary_skipwords` to 1 instead of 2. Also reported that `zh_listx` "is not loaded and has no effect" (2019 state). | **OPEN** |
| [#815](https://github.com/espeak-ng/espeak-ng/issues/815) | "Wrong tone sandhi" — 好旅馆 yields `X'Au35` where `X'Au214` is expected. | **OPEN** |
| [#257](https://github.com/espeak-ng/espeak-ng/issues/257) | "Mistake in pronounced of 9 in Mandarin Chinese" — Arabic numeral 9 spoken as "xiǔ" instead of "jiǔ"; labels `bug` + `languages/pronunciation`. | **OPEN** since 2017 |
| [#1851](https://github.com/espeak-ng/espeak-ng/issues/1851) | Han characters read as descriptive names ("it says 'Chinese Letter'"). | **OPEN** |
| [#1275](https://github.com/espeak-ng/espeak-ng/issues/1275) | Tone numerals mangled in IPA output. | **OPEN** |
| [#1805](https://github.com/espeak-ng/espeak-ng/issues/1805) | "Mandarin Pinyin issue" — the **only** eSpeak-NG issue matching `多音字`/`polyphone`. | **OPEN** |
| [#1028](https://github.com/espeak-ng/espeak-ng/issues/1028) | "The pronunciation of Mandarin Chinese using ESpeak NG in NVDA is not normal" — tones read aloud as English numbers ("今One 天One 的Five…"). | closed |
| [#1163](https://github.com/espeak-ng/espeak-ng/issues/1163) | Requests defaulting the voice role to "Chinese (Mandarin, latin as Pinyin)"; body states "#1028 makes ESpeak **completely unusable for Chinese users**." | closed |
| [#1044](https://github.com/espeak-ng/espeak-ng/issues/1044) | `Full dictionary is not installed for 'zh'` and `Error processing file 'zh_rules': No such file or directory` while `cmn_rules`/`cmn_list` exist — a concrete `zh`-vs-`cmn` data-naming failure mode. | closed |
| [#685](https://github.com/espeak-ng/espeak-ng/issues/685) | "some characters are reported two times" (汐止區 → 汐止止區). | closed `completed` |
| [#1669](https://github.com/espeak-ng/espeak-ng/issues/1669) | Decimal points not supported in `cmn_list` ("WHERE IS THE 'dian3'?"). | closed `completed` |
| [#2151](https://github.com/espeak-ng/espeak-ng/issues/2151) | "Does it support mixed Chinese and English streaming mode?" | **OPEN** |
| [PR #738](https://github.com/espeak-ng/espeak-ng/pull/738) | "Improves on some Chinese pronunciation" — the dictionary-quality fix attempt. | **closed unmerged** |

**No eSpeak-NG maintainer statement defending Mandarin quality was found**, and
#257/#606/#815/#1275/#1805/#1851/#2151 remain open while #738 stays closed unmerged.
The project's own README documents quality limitations only generically ("not as natural
or smooth as larger synthesizers … based on human speech recordings"), and
`docs/languages.md` carries no Mandarin caveat.

### rhasspy/piper and OHF-Voice/piper1-gpl
| Report | Concrete complaint | Status |
|---|---|---|
| [rhasspy/piper #305](https://github.com/rhasspy/piper/issues/305) | On `zh_CN-huayan-medium`: "**tone 1 sometimes sounds like tone 2, and tone 4 sometimes sounds like tone 1**. And the total sentence sounds strange." Also documents an espeak-ng IPA bug (213→2, 51→5) with a proposed `dictionary.c` patch, and asks "please retrain the model after fixed the espeak-ng". | **OPEN** |
| [rhasspy/piper #278](https://github.com/rhasspy/piper/issues/278) | "English is very natural, but **Chinese has an English accent and seems unnatural**. The segmentation of sentence pauses feels a bit mechanical."; mixed Chinese/English unsupported. | **OPEN** |
| [rhasspy/piper #243](https://github.com/rhasspy/piper/issues/243) | `一点儿` mispronounced by `zh_CN-huayan-medium`; a native reference recording is attached. | **OPEN** |
| [rhasspy/piper #164](https://github.com/rhasspy/piper/issues/164) | "Better Chinese phonemization" — "**the Chinese dict in espeak-ng is far too small and rigid**". | **closed `not_planned`** (2024-09-14) |
| [rhasspy/piper #652](https://github.com/rhasspy/piper/issues/652) | "the pronunciation tone of the Piper project's Chinese model is incorrect". | closed `not_planned` |
| [rhasspy/piper #835](https://github.com/rhasspy/piper/issues/835) | "Error pause for Chinese". | **OPEN** (`reopened`) |
| [rhasspy/piper #613](https://github.com/rhasspy/piper/issues/613) | Finetuning `en_US-lessac` on Chinese data gives "effect not good". | **OPEN** |
| [rhasspy/piper #740](https://github.com/rhasspy/piper/issues/740) | Mixed zh/en streaming. | **OPEN** |
| [rhasspy/piper #505](https://github.com/rhasspy/piper/issues/505) | "Is the Chinese language code currently using cmn or zh_cN?" | open, unanswered |
| [piper1-gpl #128](https://github.com/OHF-Voice/piper1-gpl/issues/128) | "**all Chinese punctuation is dropped, so Chinese paragraphs just become a single run-on sentence**" in `espeakbridge`; a fix and a test are supplied. | **OPEN** |
| [piper1-gpl #29](https://github.com/OHF-Voice/piper1-gpl/issues/29) | "how to solve the error pause for Chinese". | **OPEN** |
| [piper1-gpl #134](https://github.com/OHF-Voice/piper1-gpl/issues/134) | Community retrain of huayan medium: "The current model still needs improvement in sentence-level transitions." | **OPEN** |
| [piper1-gpl PR #289](https://github.com/OHF-Voice/piper1-gpl/pull/289) (merged) | **Documented silent-failure precedent in Piper's own C++ Chinese path:** "Phase 1 strict mono returned **empty for any poly char (虹/称/绛/简/重/行/长)**, causing long sentences like 彩虹，又称天弓… to be **empty -> PIPER_ERR_GENERIC -> silent** in piper-app iOS/macOS." The chosen fix: "use first reading for poly chars instead of failing … **95% coverage**, tradeoff: 重庆 may pick first sense until Phase 2, acceptable vs silence." | merged; Phase 2 (contextual disambiguation, [PR #272](https://github.com/OHF-Voice/piper1-gpl/pull/272)) **not shipped** |

**Counter-evidence for balance (facts):** the Piper project invested in a dedicated g2pW
Chinese front-end (1.4.0) and ported it to ONNX-only in 1.6.1 to drop PyTorch; two Mandarin
voices were built on that newer path; and a contributor reports that a native speaker found
g2pW ≈ pypinyin "the same, including intonation"
— https://github.com/OHF-Voice/piper1-gpl/pull/205 ,
https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md .
(That comparison concerns two pinyin front-ends, not the eSpeak-NG `cmn` path.)
**No peer-reviewed MOS or independent benchmark for Piper's or eSpeak-NG's Mandarin voices
exists — not determinable from public sources.** All evidence above is issue-tracker and
model-card material.

### MeloTTS
- The sherpa-onnx ONNX port cannot pronounce the letter "v" at all, on Linux and Android
  ([#2960](https://github.com/k2-fsa/sherpa-onnx/issues/2960)).
- English words not in the converted `lexicon.txt` are unpronounceable: "if you input
  English words, only those that are present in the `lexicon.txt` can be pronounced"
  — https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html
- Upstream: [#206](https://github.com/myshell-ai/MeloTTS/issues/206) "How is pronunciation
  decided?" (homograph *wind* pronounced wrong; "right for the default voices, but wrong
  when I trained a new English voice"); [#241](https://github.com/myshell-ai/MeloTTS/issues/241)
  "Wrong pronunciation in english(all voices)" (`plugin` → "ploogin"). A search for
  `多音字` in that repo returns only #193/#66/#98 — **no dedicated Mandarin polyphone bug
  report found**.

### Kokoro
- [#238](https://github.com/hexgrad/kokoro/issues/238) (open), verbatim: "KokoroTTS does
  not perform well, and even fails to recognize sentences that mix Chinese and English… I
  suspect it might be due to the system converting every Chinese character into phonetic
  symbols based on pinyin… This issue is even more pronounced in Kokoro-zh-v1.1, as it
  doesn't attempt to pronounce the English words at all."
- Upstream `VOICES.md` grades all eight Mandarin voices **"D"** overall and warns
  "Support for non-English languages may be absent or thin due to **weak G2P** and/or lack
  of training data"
  — https://huggingface.co/hexgrad/Kokoro-82M/blob/main/VOICES.md
- `misaki`'s Chinese G2P leaves homographs unresolved: TODO "**Homographs**: Escalate hard
  words like `axes bass bow lead tear wind` using BERT contextual word embeddings and
  logistic regression"
  — https://raw.githubusercontent.com/hexgrad/misaki/main/README.md

### PaddleSpeech — 多音字 issues (verified via the search API)
- [#3737](https://github.com/PaddlePaddle/PaddleSpeech/issues/3737) "TTS多音字问题"
  (closed 2024-04-12) — 行 read as `xing` not `hang` in
  "是否有我行人员或其他人员向你收取手续费？"
- [#3297](https://github.com/PaddlePaddle/PaddleSpeech/issues/3297) "TTS多音字问题"
  (closed 2025-06-27, 7 comments) — 银行/行人 OK but "欢迎来到我行办理业务" wrong.
- [#3664](https://github.com/PaddlePaddle/PaddleSpeech/issues/3664) "fastspeech2_aishell3
  效果很差" (multiple voices despite `spk_id`, unclear characters).
- [#4171](https://github.com/PaddlePaddle/PaddleSpeech/issues/4171) — **OPEN**, created
  2026-06-15: "TTS CPU segfault on paddlepaddle 2.6.2".

### ChatTTS — 多音字 workaround, documented
- Merged [PR #350](https://github.com/2noise/ChatTTS/pull/350) "[Fix] Replace mispronounced
  words in TTS using hack method" adds `ChatTTS/homophones_map.json` with **16,000 entries**;
  the stated method is "1. Identify correctly pronounced characters by ChatTTS. 2. Replace
  the mispronounced characters with correctly pronounced ones." Worked example:
  `关关雎鸠，在河之洲。窈窕淑女，君子好逑。` → `关关居鸠，在河之洲。咬挑淑女，君子好求。`
  Stated limitations: "Some characters and words might not be covered" and "Some characters
  do not have correctly pronounced homophones, e.g. 'sǒu 叟'".
  This is a first-party acknowledgement that the model mispronounces 多音字 and that the
  fix is a text-level dictionary substitution.
- [#659](https://github.com/2noise/ChatTTS/issues/659) — garbled zh/en text then
  `Segmentation fault (core dumped)`.

### Which candidate handles 多音字 / tones best, per published evidence
Facts only, with the caveat that **no comparable, independent Mandarin accuracy benchmark
exists for any candidate**:
- **PaddleSpeech** publishes an explicit architecture for it — a g2pW ONNX BERT plus a
  `Polyphonic`/`ToneSandhi` front-end — and its docs are candid that "g2pM and pypinyin do
  not perform well in tone sandhi, we use rules to solve this problem"; it also has the
  largest number of filed 多音字 bug reports (#3737, #3297).
- **sherpa-onnx** depends on a **lexicon + longest-match segmentation** design
  (PR #2507) with `phone.fst`/`date.fst`/`number.fst` rules and a documented policy of
  adding needed words to `lexicon.txt`; it has a dedicated pinyin-phrase replacement
  facility. No mandarin-tone bug report was found in its tracker, but its Matcha Chinese
  model has a maintainer-confirmed model-level error report (#2904).
- **Piper** publishes the clearest *negative* statement about its own shipping capability:
  its C++ Chinese path uses the **first reading for polyphonic characters** by design
  (95 % coverage claimed), because the strict path produced **silence**; contextual
  disambiguation is deferred to an unshipped PR (#272). The only eSpeak-NG issue matching
  `polyphone`/`多音字` is open (#1805), and the dictionary-quality PR was closed unmerged
  (#738).
- **ChatTTS** publishes a 16,000-entry homophone-substitution table as its 多音字 fix,
  which is an explicit acknowledgement of the problem.
- **Kokoro**'s own G2P leaves homograph escalation as an open TODO, and its eight upstream
  Mandarin voices are graded "D".
- **MeloTTS** upstream exposes a full pinyin/tone-sandhi front-end (pypinyin + jieba POS +
  ToneSandhi + BERT); the only ONNX form usable on Android **has that BERT zeroed out**,
  and no dedicated Mandarin polyphone issue was found in its tracker.
- **eSpeak-NG (used alone)** has a built-in Han→pinyin table (`cmn_list`, 3,799 characters,
  single-reading) plus a large compound-exception list (`extra/cmn_listx`, 36,500 compound
  exceptions, enabled by default), but its multi-word lookup is reported broken and still
  open (#606) and it performs no word segmentation.

**No candidate has a published, independent Mandarin tone-accuracy or 多音字-accuracy
metric.** The only quantitative 多音字 figures found anywhere are Piper's own "95 %
coverage" claim for its first-reading fallback and Kokoro's "D" voice grades.

---

## Not determinable from public sources

Listed as blank spots, not negative results.

1. **On-device Android arm64 latency/RTF for any engine.** Every published RTF in this
   survey is x86 desktop or Raspberry Pi 4 — measurable only by shipping and measuring.
2. **The exact file list and per-file sizes inside `sherpa-onnx-v1.13.7-android.tar.bz2`**
   (45,287,000 B), and in particular whether it contains
   `libsherpa-onnx-c-api.so` and/or `libsherpa-onnx-cxx-api.so` in addition to the JNI
   library. The build script's generated README states both C-API `.so` files exist when
   `SHERPA_ONNX_ENABLE_C_API=ON`, and CI sets that flag — but the tarball contents were not
   inspected.
3. **Per-file sizes of the `kokoro-multi-lang-v1_1` archives** (only the 311 MB aggregate
   in the RTF table is published; v1_0's per-file sizes are published).
4. **`sherpa-onnx-vits-zh-ll` archive size** and the per-file sizes of
   `matcha-icefall-zh-en`.
5. **Any published Mandarin quality metric** — MOS, CER, tone accuracy, or a polyphone
   accuracy figure — for Piper, eSpeak-NG, sherpa-onnx, or any candidate except the
   isolated "95 % coverage" (Piper) and "D" grades (Kokoro).
6. **Whether `QtTextToSpeech` and its `Qt6AndroidTextToSpeech` jar ship by default in the
   Qt Online Installer's Android package** or require explicit selection. Qt's docs list it
   as an add-on and describe add-ons as an installer option, but publish no per-module
   matrix.
7. **Whether `TextToSpeech` can be driven from `QJniObject` with no app Java class at all.**
   Calling framework classes from C++ is documented, but `TextToSpeech` needs an
   `OnInitListener` and an `UtteranceProgressListener`; Qt's documented answer for interface
   callbacks is generated Java glue (Qt Jenny). No Qt example, forum thread or bug report
   showing a no-Java approach was found.
8. **The `com.google.android.tts` package name, Google's exact "Install voice data" doc
   wording, and whether Google Speech Services ships voice data pre-installed or requires a
   user download.** `developer.android.com`, `support.google.com` and `play.google.com`
   were unreachable, and the literal string "Install voice data" does not appear in the
   retrieved AOSP Settings sources.
9. **Samsung (`com.samsung.SMT*`) and Baidu TTS support for the standard
   `android.speech.tts` contract.** Samsung's developer/Galaxy Store/Knox docs and Baidu's
   client-rendered docs could not be read; iFlytek's docs describe proprietary SDKs only
   and never mention the standard engine contract.
10. **The concrete sample rate any real shipping Android engine uses for Mandarin** — such
    figures are not published in AOSP and are only discoverable at runtime via
    `onBeginSynthesis`.
11. **Piper's `espeak-ng-data` size** (no figure in either repo's docs/CHANGELOG/CMake; the
    source tree contains zero data files, generated at build time). The compiled
    `cmn_dict` size is known only from a third-party bundle.
12. **`dictsource/extra/cmn_listx`'s exact byte size and literal compound lines** (the
    fetched copy was truncated); the header-derived counts (21,611 characters + 36,500
    compounds) are confirmed.
13. **Comment bodies on the open Mandarin defects** — espeak-ng #606/#815/#1805 and
    rhasspy/piper #164 — and eSpeak-NG's `ChangeLog.md`; GitHub's REST API rate-limited
    these. So a maintainer reply *on* the open Mandarin defects remains unknown, and the
    statement "no maintainer reply defends Mandarin quality" is scoped to what was
    retrievable.
14. **Whether `https://huggingface.co/datasets/rhasspy/piper-voices` exists and what it
    contains** — the datasets API returned HTTP 401.
15. **PaddleSpeech's ONNX/checkpoint/Paddle-Lite archive sizes, the `G2PWModel_1.1.zip`
    size, its weight licence and commercial terms, and its dataset licences.**
16. **ekho's canonical Mandarin output sample rate, its Mandarin voice-data licence, and
    everything about the non-existent `hgneng/ekho-android` repository** (HTTP 404).
17. **CosyVoice's on-disk model file sizes and the text of its weights licence**; and
    whether F5-TTS is autoregressive.
18. **ChatTTS's CPU/ARM RTF**, an official ChatTTS paper, and any C++/ONNX/Android port.
19. **iFlytek's pricing/royalty terms** for the offline SDK.
20. **MeloTTS fp16/int8 variants** (none published) and **MeloTTS's own arm64 RTF** (none
    published).

## Documentation discrepancies found (reported as found, not resolved)

- sherpa-onnx's VITS page lists `aishell3` at **116 MB** in the summary table but **29M/30 MB**
  in the detail and RTF tables; the release asset `vits-icefall-zh-aishell3.tar.bz2` is
  31,559,701 B.
- ekho's `README.md` says "Voice files are not included." while `master` commits
  `pinyin.voice` and `jyutping.voice`.
- eSpeak-NG's `android/build.gradle` says `compileSdk 36` / `targetSdk 36` while another
  file in the same tree states 34/33.
- `kokoro-onnx`'s release notes round asset sizes differently from the actual byte counts.
- Piper's `libpiper` README says espeak-ng data installs to `install/espeak-ng-data/` while
  its CMake installs to `<prefix>/share/espeak-ng-data`.
- Qt's module page says LGPLv3 **or GPLv2**; the source headers say
  LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only; the Qt 6.11 licensing page says LGPLv3
  with a GPLv3-only module list that excludes TextToSpeech.
