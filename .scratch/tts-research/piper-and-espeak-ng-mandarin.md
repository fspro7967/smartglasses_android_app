# Piper + eSpeak-NG Mandarin (Chinese) — primary-source findings

**Target under evaluation:** Qt 6.11.1 / C++17 / CMake Android app, arm64-v8a, Android 9+ (API 28), zero Java/JNI. Requirements under consideration: offline Mandarin TTS, accurate pronunciation, small model, low latency.

**Companion artifacts (same research effort, same workspace):** deep-dive notes on the C++ embedding surface / Android state live in `.scratch/piper-embedding-research.md`, and on the eSpeak-NG Mandarin front-end in `.scratch/research/espeak-ng-mandarin-primary-sources.md`. This file is the consolidated report for Topics A–C.

**Method / environment caveats (read first — they affect reproducibility)**

- Every claim below carries an inline URL to the source that owns it.
- `not published` = no primary source publishes that number. `not determinable from public sources` = the fact could not be verified.
- Direct `https://huggingface.co/...` fetches **failed from this environment** (transport error, not 404). Hugging Face repo listings, `.onnx.json` configs and `MODEL_CARD` files were therefore read through the `hf-mirror.com` mirror of identical paths (same repo, same files) and are cited using the canonical `https://huggingface.co/...` URL. Exact byte counts come from the Hugging Face **repository tree API**, which reports sizes as stored.
- `github.com` HTML pages return mostly navigation boilerplate when fetched; `.md`/`.py`/`.h`/`.cpp` files were read raw from `raw.githubusercontent.com`, and issue metadata (state, labels, comments, `state_reason`) from the GitHub REST **search** API (`api.github.com/search/issues`). The non-search REST endpoints hit the anonymous rate limit repeatedly during this research; where a call returned HTTP 403 rate-limit, the affected fact is marked.
- `pwsh` in this sandbox has **no working network access** (TLS reset), so no numbers were computed by downloading artifacts.
- No recommendation is given, per the brief.

---

## Topic A — Piper (rhasspy/piper, thereafter OHF-Voice/piper1-gpl)

### A1. Is it maintained? Canonical repo, latest release, licences

**Canonical repo has moved; the old one is archived.**

- `rhasspy/piper` is **archived**: `"archived": true`, `"license": {"spdx_id": "MIT"}`, default branch `master`, `pushed_at` `2025-08-26T15:01:29Z`, 11,277 stars, 1,074 forks, 418 open issues — <https://api.github.com/repos/rhasspy/piper>
- Its current `master` README is a single line: *"Development has moved: https://github.com/OHF-Voice/piper1-gpl"* — <https://raw.githubusercontent.com/rhasspy/piper/master/README.md>
- The last **release** on `rhasspy/piper` is tag `2023.11.14-2` (published 2023-11-14T19:32:39Z); prior tags include `2023.11.14-1`, `2023.11.6-3`, `v1.2.0`, `v1.1.0`, `v1.0.0` — <https://github.com/rhasspy/piper/releases.atom>

**Current canonical repo: `OHF-Voice/piper1-gpl`, actively maintained.**

- `"archived": false`, `"license": {"spdx_id": "GPL-3.0", "name": "GNU General Public License v3.0"}`, default branch `main`, created `2025-03-28T21:47:10Z`, `pushed_at` `2026-09-09T21:40:27Z`, 5,524 stars, 531 forks, 131 open issues — <https://api.github.com/repos/OHF-Voice/piper1-gpl>
- **Latest release: `v1.8.0`**, published `2026-09-04T16:47:48Z` — <https://github.com/OHF-Voice/piper1-gpl/releases/tag/v1.8.0> (asset list from <https://api.github.com/repos/OHF-Voice/piper1-gpl/releases>)
- Recent release cadence (all from <https://api.github.com/repos/OHF-Voice/piper1-gpl/releases>): v1.8.0 (2026-09-04), v1.7.0 (2026-08-15), v1.6.1, v1.6.0 (2026-07-23), v1.5.0 (2026-07-17), v1.4.x, v1.3.0.
- The maintainer is still recruiting: the README carries a *"Looking for Maintainers — The Open Home Foundation is looking for maintainers for Piper!"* notice — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/README.md>

**Licence of the CODE**

| Component | Licence | Source |
| --- | --- | --- |
| `rhasspy/piper` (legacy, archived) | **MIT** | <https://api.github.com/repos/rhasspy/piper> |
| `OHF-Voice/piper1-gpl` (current) | **GPL-3.0** | <https://api.github.com/repos/OHF-Voice/piper1-gpl> |

- The relicense is recorded in the changelog: under **1.3.0** — *"Change license to GPLv3"*, in the same entry as *"Moved development to OHF-Voice org"* and *"Removed C++ code for now to focus on Python development — A C API `libpiper` written in C++ is planned"* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md>
- One file states a different licence than the repo: `src/piper/phonemize_chinese.py` header reads *"Partially written by ChatGPT (December 2025). This code is Apache 2.0 licensed."* inside the GPL-3.0 repo — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/src/piper/phonemize_chinese.py>

**Licence of the VOICE MODELS — per-voice, and NOT in `voices.json`**

- The `rhasspy/piper-voices` **model repo** card metadata is `license: mit`, with `language: [..., "zh"]` among 35 language codes — <https://huggingface.co/rhasspy/piper-voices> and <https://huggingface.co/api/models/rhasspy/piper-voices> (`cardData.license = "mit"`, `tags` includes `license:mit`).
- **`voices.json` carries no licence field.** Across the ~30 complete voice entries retrieved from it (`ar_JO-kareem-low`, `bg_BG-dimitar-medium`, `bn_BD-google-medium`, `ca_ES-upc_ona-medium`, `cs_CZ-jirka-*`, `cy_GB-bu_tts-medium`, `da_DK-talesyntese-medium`, `de_DE-*`, `en_US-libritts_r-medium`, …) each entry has exactly these keys: `key`, `name`, `language`, `quality`, `num_speakers`, `speaker_id_map`, `files`, `aliases` — a regex search for `licen[cs]e` over the retrieved `voices.json` content returned **zero matches** (<https://huggingface.co/rhasspy/piper-voices/resolve/main/voices.json>; the retrieved copy was truncated mid-file by the fetch tool, so "no licence field anywhere in the file" is inferred from, not proven by, this sample — the per-voice truth is in `MODEL_CARD`).
- The project points to `MODEL_CARD` as the authoritative per-voice licence. `docs/VOICES.md`: *"The `MODEL_CARD` file for each voice contains important licensing information. Piper is intended for personal use and text to speech research only; we do not impose any additional restrictions on voice models. Some voices may have restrictive licenses, however, so please review them carefully!"* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/VOICES.md>
- The legacy README worded the same policy as *"Piper is intended for text to speech research"* — <https://raw.githubusercontent.com/rhasspy/piper/v1.2.0/README.md>

**Actual per-voice dataset licence fields for the Chinese voices** (quoted from each voice's `MODEL_CARD`):

| Voice | `License:` line in MODEL_CARD | MODEL_CARD URL |
| --- | --- | --- |
| `zh_CN-huayan-medium` | *"License: Unknown"* | <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/medium/MODEL_CARD> |
| `zh_CN-huayan-x_low` | *"License: Unknown"* | <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/x_low/MODEL_CARD> |
| `zh_CN-chaowen-medium` | *"License: CC0"* | <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/chaowen/medium/MODEL_CARD> |
| `zh_CN-xiao_ya-medium` | *"License: Non-commercial use (see https://www.data-baker.com/data/index/TNtts/)"* | <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/xiao_ya/medium/MODEL_CARD> |

**Caveat on the dataset repo named in the brief:** `https://huggingface.co/datasets/rhasspy/piper-voices` could not be listed — the datasets API path returned **HTTP 401 `{"error":"Invalid username or password."}`** through the mirror (<https://hf-mirror.com/api/datasets/rhasspy/piper-voices>). Whether that repo exists as a gated/private dataset, or does not exist, is **not determinable from public sources** here. The authoritative voice repository is the **model** repo `rhasspy/piper-voices`.

### A2. Exact `zh_CN` voice names, published sizes, quality tier, speakers, sample rate

The `zh` tree contains **only** `zh_CN` (no `zh_TW`, no other Chinese locale) — <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh> → `[{"type":"directory","path":"zh/zh_CN"}]`.

`zh_CN` contains **three voice families**, not one — <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN>: `chaowen`, `huayan`, `xiao_ya`.

**Four published Chinese voice models exist in total:**

| Voice | `.onnx` bytes | `.onnx.json` bytes | Quality tier | Speakers | Sample rate |
| --- | --- | --- | --- | --- | --- |
| `zh_CN-huayan-medium` | **63,201,294** | **4,822** | `medium` | 1 | **22,050 Hz** |
| `zh_CN-huayan-x_low` | **20,628,813** | **4,164** | `x_low` | 1 | **16,000 Hz** |
| `zh_CN-chaowen-medium` | **63,221,984** | **2,927** | `medium` | 1 | **22,050 Hz** |
| `zh_CN-xiao_ya-medium` | **63,221,984** | **2,927** | `medium` | 1 | **22,050 Hz** |

Sources (direct file listings, as requested):

- `zh_CN/huayan` has exactly two quality directories, `medium` and `x_low` — <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan>
- `zh_CN/huayan/medium` → `samples/`, `MODEL_CARD` (276 B), `zh_CN-huayan-medium.onnx` (63,201,294 B), `zh_CN-huayan-medium.onnx.json` (4,822 B) — <https://huggingface.co/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/medium>
- `zh_CN/huayan/x_low` → `samples/`, `ALIASES` (19 B), `MODEL_CARD` (237 B), `zh_CN-huayan-x_low.onnx` (20,628,813 B), `zh_CN-huayan-x_low.onnx.json` (4,164 B) — <https://huggingface.co/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/x_low>
- `zh_CN/chaowen` → only `medium` — <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/chaowen>; `medium` → `MODEL_CARD` (273 B), `zh_CN-chaowen-medium.onnx` (63,221,984 B), `.onnx.json` (2,927 B) — <https://huggingface.co/rhasspy/piper-voices/tree/main/zh/zh_CN/chaowen/medium>
- `zh_CN/xiao_ya` → only `medium` — <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/xiao_ya>; `medium` → `MODEL_CARD` (434 B), `zh_CN-xiao_ya-medium.onnx` (63,221,984 B), `.onnx.json` (2,927 B) — <https://huggingface.co/rhasspy/piper-voices/tree/main/zh/zh_CN/xiao_ya/medium>

Sample rate, speaker count and quality are also asserted inside the configs and model cards:

- `zh_CN-huayan-medium.onnx.json`: `"audio": {"sample_rate": 22050, "quality": "medium"}`, `"num_speakers": 1`, `"speaker_id_map": {}`, `"num_symbols": 256`, `"piper_version": "1.0.0"`, `"language": {"code": "zh_CN", "name_native": "简体中文"}`, `"dataset": "huayan"` — <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/medium/zh_CN-huayan-medium.onnx.json>
- `zh_CN-huayan-x_low.onnx.json`: `"sample_rate": 16000`, `"quality": "x_low"`, `"num_speakers": 1`, `"num_symbols": 130`, `"piper_version": "0.2.0"` — <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/x_low/zh_CN-huayan-x_low.onnx.json>
- `zh_CN-chaowen-medium.onnx.json` and `zh_CN-xiao_ya-medium.onnx.json`: `"sample_rate": 22050`, `"quality": "medium"`, `"num_speakers": 1`, `"num_symbols": 256`, `"piper_version": "1.3.0"`, `"hop_length": 256` — <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/chaowen/medium/zh_CN-chaowen-medium.onnx.json> and <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/xiao_ya/medium/zh_CN-xiao_ya-medium.onnx.json>
- `huayan` MODEL_CARD: *"Samplerate: 22,050Hz"* (medium) / *"Samplerate: 16,000Hz"* (x_low); both *"Speakers: 1"*; dataset URL `https://github.com/PlayVoice/HuaYan_TTS`; medium is *"Finetuned from U.S. English lessac voice (medium quality)"*, x_low is *"Trained from scratch"*.
- `xiao_ya` MODEL_CARD (小雅 Xiǎo Yǎ): dataset `https://huggingface.co/openspeech/BZNSYP`, *"Trained from scratch"*, plus the note quoted in A4.
- `chaowen` MODEL_CARD (超文): dataset `https://github.com/OHF-Voice/voice-datasets`, *"Finetuned from Xiao Ya voice (medium quality)"*.

**Quality tiers present for Chinese: `x_low` and `medium` only.** There is no `low` and no `high` tier published for any Chinese voice. (A historical `zh-cn-huayan-low.onnx` filename appears in issue #34 — <https://github.com/rhasspy/piper/issues/34> — but no `low` Chinese voice exists in the current tree; the API path `zh/zh_CN/huayan/low` returns `{"error":"zh/zh_CN/huayan/low does not exist on \"main\""}` — <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/low>.)

**What the quality tiers mean (published definitions).** The Piper samples page defines the four tiers by sample rate and parameter count, and publishes **no RTF column**:

> `x_low - 16Khz audio, 5-7M params` / `low - 16Khz audio, 15-20M params` / `medium - 22.05Khz audio, 15-20M params` / `high - 22.05Khz audio, 28-32M params`

— <https://rhasspy.github.io/piper-samples/> (table header is `Language Voice Quality Speaker`). Under this scheme `zh_CN-huayan-x_low` (16 kHz) is a 5–7 M-parameter model and the three `medium` Chinese voices (22.05 kHz) are 15–20 M-parameter models. The exact parameter count of each Chinese voice is **not published** per-voice.

### A3. Quantised variants (fp32 / fp16 / int8)

**Official `rhasspy/piper-voices`: none for Chinese.** Each Chinese voice directory was enumerated in full and contains only `MODEL_CARD`, the `.onnx`, the `.onnx.json`, and `samples/` (plus `ALIASES` for `huayan/x_low`). **No `.fp16.onnx`, no `.int8.onnx`, no separately published fp32 variant** — URLs as in A2 (`.../huayan/medium`, `.../huayan/x_low`, `.../chaowen/medium`, `.../xiao_ya/medium`).

**Third-party quantised Chinese Piper voices do exist on Hugging Face**, published by the sherpa-onnx author `csukuangfj`:

| Repo | File | Bytes | Provenance |
| --- | --- | --- | --- |
| `csukuangfj2/vits-piper-zh_CN-xiao_ya-medium` | `zh_CN-xiao_ya-medium.onnx` | 63,221,984 (unquantised) | third-party re-export |
| `csukuangfj2/vits-piper-zh_CN-xiao_ya-medium-fp16` | `zh_CN-xiao_ya-medium.onnx` | **31,922,716** | **fp16** |
| `csukuangfj2/vits-piper-zh_CN-xiao_ya-medium-int8` | `zh_CN-xiao_ya-medium.onnx` | **18,652,795** | **int8** |
| `csukuangfj2/vits-piper-zh_CN-chaowen-medium` | `zh_CN-chaowen-medium.onnx` | 63,222,141 | third-party re-export |
| `csukuangfj2/vits-piper-zh_CN-chaowen-medium-int8` | `zh_CN-chaowen-medium.onnx` | **18,652,796** | **int8** |
| `csukuangfj2/vits-piper-zh_CN-chaowen-medium-fp16` | `zh_CN-chaowen-medium.onnx` | 31,922,716-class (fp16) | **fp16** |

- Listings: <https://huggingface.co/api/models/csukuangfj2/vits-piper-zh_CN-xiao_ya-medium-fp16/tree/main>, <https://huggingface.co/api/models/csukuangfj2/vits-piper-zh_CN-xiao_ya-medium-int8/tree/main>, <https://huggingface.co/api/models/csukuangfj2/vits-piper-zh_CN-chaowen-medium/tree/main>, <https://huggingface.co/api/models/csukuangfj2/vits-piper-zh_CN-chaowen-medium-int8/tree/main>; repo search that surfaced them: <https://huggingface.co/api/models?search=piper-zh&limit=30>
- These int8/fp16 re-exports also ship sherpa-onnx front-end assets that the official repo does not: `lexicon.txt` (2,064,505 B), `date.fst` (59,154 B), `number.fst` (64,482 B), `phone.fst` (88,630 B), `tokens.txt` (485 B).
- `csukuangfj/vits-piper-zh_CN-huayan-medium` (no int8/fp16 sibling found) and `csukuangfj/vits-piper-zh_CN-huayan-x_low` exist as re-exports; the `medium` one additionally **bundles `espeak-ng-data/`** — <https://huggingface.co/api/models/csukuangfj/vits-piper-zh_CN-huayan-medium/tree/main>, <https://huggingface.co/api/models/csukuangfj/vits-piper-zh_CN-huayan-x_low/tree/main>.
- **No huayan int8 or fp16 variant was found on Hugging Face** in this search; whether one exists elsewhere is **not determinable from public sources**.

**A separate, first-party quantisation fact:** the g2pW front-end model used by the `pinyin` path is itself dynamically quantised upstream — changelog 1.4.0: *"Add Chinese phonemizer based on g2pW — Using a quantized version of the original model with `quantize_dynamic`"* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md>.

Also note the `medium` and `x_low` onnx files carry **no dtype suffix and no datatype metadata** in `.onnx.json`; the stored numeric precision of the released weights is **not published** in any source found.

### A4. How Piper does Chinese phonemisation — two entirely different paths

**This is the single most consequential finding: the Chinese voices are not one pipeline. `huayan` uses eSpeak-NG; `xiao_ya` and `chaowen` use a g2pW/pinyin front-end instead.**

**Path 1 — eSpeak-NG `cmn` (huayan only)**

- `zh_CN-huayan-medium.onnx.json` contains `"phoneme_type": "espeak"` and `"espeak": {"voice": "cmn"}` — <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/medium/zh_CN-huayan-medium.onnx.json>
- `zh_CN-huayan-x_low.onnx.json` also has `"espeak": {"voice": "cmn"}` (it predates the explicit `phoneme_type` key; its `piper_version` is `0.2.0`) — <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/x_low/zh_CN-huayan-x_low.onnx.json>
- So yes: **`zh_CN` here means eSpeak-NG voice `cmn`** (the ISO 639-3 code for Mandarin), *not* a pinyin dictionary front-end, and *not* the eSpeak-NG `zh` alias. The `huayan-medium` phoneme inventory is the 256-entry IPA/espeak set and **includes the ASCII digits `"1"`–`"5"`** (ids 130–135), i.e. tone numbers are carried through as phonemes.
- Consistent with this, the legacy C++ `piper` binary was built against a *patched* espeak-ng and required `piper-phonemize`: *"You must download and extract [piper-phonemize] to `lib/Linux-$(uname -m)/piper_phonemize`"* — <https://raw.githubusercontent.com/rhasspy/piper/v1.2.0/README.md>

**Path 2 — g2pW + pinyin (xiao_ya, chaowen)**

- Both configs carry `"phoneme_type": "pinyin"` and `"espeak": {"voice": "zh"}`, with a completely different 256-symbol inventory of **pinyin initials/finals plus tone digits**: `"Ø"` (zero initial), `"b","p","m","f",…,"zh","ch","sh",…,"y","w"` (initials), `"a","o","e","ai","ei","ao","ou","an","en","ang","eng","ong",…,"v","ve","van","vn","er","ue"` (finals), and `"1": [64], "2": [65], "3": [66], "4": [67], "5": [68]` alongside Chinese punctuation `。？！—…、，：；` — <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/xiao_ya/medium/zh_CN-xiao_ya-medium.onnx.json>, <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/chaowen/medium/zh_CN-chaowen-medium.onnx.json>. Here `"espeak": {"voice": "zh"}` is set but the phoneme inventory is *not* espeak IPA — the field is vestigial for this path.
- The `xiao_ya` MODEL_CARD says so explicitly: *"**Only works on the Python version of Piper 1.4+ due to a dependency on [g2pW](https://github.com/GitYCC/g2pW/)**"* — <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/xiao_ya/medium/MODEL_CARD>
- The front-end is `src/piper/phonemize_chinese.py`, which: instantiates `G2PWOnnxConverter(model_dir=model_dir, style="pinyin", enable_non_tradional_chinese=True)`; splits text to sentences via `sentence_stream.stream_to_sentences`; converts numbers/temperatures/percentages with `unicode_rbnf`'s `RbnfEngine.for_language("zh")`; splits each g2pW syllable into initial/final/tone with `PINYIN_INITIALS` sorted longest-first (`zh`,`ch`,`sh` before `z`,`c`,`s`); and normalises g2pW's `u:`/`ü` to `v` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/src/piper/phonemize_chinese.py>
- The g2pW model is **downloaded at runtime, not vendored**: `G2PW_URL = "https://huggingface.co/datasets/rhasspy/piper-checkpoints/resolve/main/zh/zh_CN/_resources/g2pw.tar.gz?download=true"`, extracted into `model_dir` as `g2pw.onnx` — same file.
- Changelog lineage: 1.4.0 added the g2pW Chinese phonemizer and `--data.phoneme_type pinyin`; **1.6.1** replaced `g2pw.api` with `piper.g2pw_onnx`, *"dropping `torch` (~750 MB installed) and `requests` from the `zh` extra"* and *"`g2pW` is still required, for its pinyin/bopomofo lookup tables"* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md>. Relevant source files in the current repo: `src/piper/phonemize_chinese.py` (9,001 B) and `src/piper/g2pw_onnx.py` (20,945 B) — <https://api.github.com/repos/OHF-Voice/piper1-gpl/contents/src/piper>

**What the C++ library does with Chinese — a third, reduced path**

`libpiper` contains `src/chinese_phonemizer.cpp`, which implements a **mono-reading-only** front-end and documents the polyphone gap in its own comments:

- *"Phase 1 (monophonic fallback) directory should contain MONOPHONIC_CHARS.txt (118K) and/or char_bopomofo_dict.json and bopomofo_to_pinyin_wo_tune_dict.json. **g2pw.onnx and POLYPHONIC_CHARS.txt are not required in Phase 1; they will be used in Phase 2 for contextual polyphone disambiguation.**"* — this text is in the public header doc comment for `piper_create_options.g2pw_model_dir`, <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h>
- In `chinese_phonemizer.cpp`: *"Phase 1 mono-only: accept only unambiguous one-reading entries… ambiguous (polyphonic) chars are treated as unsupported… This prevents 重庆/银行/长江 from being mis-assigned when only char_bopomofo_dict.json is present. **Relaxed: use first reading for poly to avoid empty -> silence.**"* and `// relaxed: first reading for polyphonic` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/chinese_phonemizer.cpp>
- i.e. the shipped C++ path at `main` uses the **first reading** for polyphonic characters (多音字) rather than context-dependent disambiguation.
- `libpiper/CMakeLists.txt` compiles `src/piper.cpp` and `src/chinese_phonemizer.cpp` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt>
- **The verbatim failure mode this behaviour was written to fix** (PR #289 "fix: Phase 1 relaxed poly fallback for mobile", merged 2026-08-29): *"Phase 1 strict mono returned **empty for any poly char (虹/称/绛/简/重/行/长)**, causing long sentences like 彩虹，又称天弓… to be empty -> **PIPER_ERR_GENERIC -> silent** in piper-app iOS/macOS."* The accepted trade-off is stated as *"95% coverage, tradeoff: 重庆 may pick first sense until Phase 2, acceptable vs silence"* — <https://github.com/OHF-Voice/piper1-gpl/pull/289>. So the C++ Chinese path has already produced a **silent-failure** mode in a shipped app, and the fix trades correctness for output.
- **The Chinese front-end rollout by PR (exact titles and merge dates):**
  - **PR #271 "Add Chinese pinyin support – Phase 1 (monophonic dict fallback, honest scoping)"** — **merged 2026-08-22**; adds the non-espeak Chinese pinyin path; verbatim *"Polyphonic chars (`重/行/长/好` etc.) treated as unsupported → empty, preventing silent mis-assignment of `重庆/银行/长江`."* — <https://github.com/OHF-Voice/piper1-gpl/pull/271>
  - **PR #289 "fix: Phase 1 relaxed poly fallback for mobile"** — merged 2026-08-29 (quoted above).
  - **PR #272 — Phase 2**, i.e. full contextual polyphone disambiguation via the g2pw BERT ONNX model. Referenced verbatim from #271: *"Full contextual polyphone disambiguation (g2pw BERT ONNX `g2pw.onnx`) deferred to #272."* **Phase 2 is not shipped** — <https://github.com/OHF-Voice/piper1-gpl/pull/272>
  - **PR #269 "Run g2pW under onnxruntime without torch"** — merged 2026-08-13; verbatim: *"Separately, output was compared against upstream `g2pw.G2PWConverter` on the same `g2pw.onnx`: identical results across 19 sentences (polyphone disambiguation, mixed ASCII/digits, …)."* — <https://github.com/OHF-Voice/piper1-gpl/pull/269>
  - **PR #153 "Add Chinese phonemization using g2pW"** — closed **without merge**; superseded by #271.

**Primary evidence that the eSpeak-NG Chinese dictionary is the weak link**

- `rhasspy/piper` issue **#164, "Better Chinese phonemization"** (opened 2023-08-04 by KYShek, **closed as `not_planned` on 2024-09-14**, 8 comments): *"I'm working on applying some other word segmentation module like jieba to piper-phonemize, because **the Chinese dict in espeak-ng is far too small and rigid**. I want to know if espeak-ng can keep my splits (seems espeak-ng may remove all the space in chinese text before dicting Pinyin)."* — <https://github.com/rhasspy/piper/issues/164> (state/`state_reason` from <https://api.github.com/search/issues?q=repo%3Arhasspy%2Fpiper+chinese&per_page=30>)
- That issue is the historical motivation for the g2pW path that shipped in piper1-gpl 1.4.0 (A4, Path 2), and the PR that first attempted it, `OHF-Voice/piper1-gpl` **#153 "Add Chinese phonemization using g2pW"**, was **closed without merge** (`merged_at: null`, closed 2026-01-20); the follow-up **#163 "Add Chinese phonemization and phoneme ids in training"** was **merged 2026-01-20** — <https://api.github.com/search/issues?q=repo%3AOHF-Voice%2Fpiper1-gpl+chinese&per_page=30>

### A5. Inference runtime, the C++ embedding API, `espeak-ng-data`, Android

**Inference runtime: ONNX Runtime. No custom C++ inference engine.**

- `libpiper/CMakeLists.txt` uses `find_package(onnxruntime QUIET)`, defines an imported target `onnxruntime::onnxruntime`, and links it into the library: `target_link_libraries(piper ${ESPEAKNG_STATIC_LIB} ${UCD_STATIC_LIB} onnxruntime::onnxruntime)`. Default `ONNXRUNTIME_VERSION` is **`1.22.0`**, downloaded from `https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/…` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt>
- **Correction to a common assumption (verified):** `src/cpp/CMakeLists.txt` does **not** exist. `https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/CMakeLists.txt` → **HTTP 404**, and `https://api.github.com/repos/rhasspy/piper/contents/src/cpp` lists only `json.hpp`, `main.cpp`, `piper.cpp`, `piper.hpp`, `test.cpp`, `utf8.h`, `utf8/`, `wavfile.hpp`. The legacy build file is at the **repo root**: <https://raw.githubusercontent.com/rhasspy/piper/master/CMakeLists.txt>
- **Legacy link line, verbatim** (<https://raw.githubusercontent.com/rhasspy/piper/master/CMakeLists.txt>): `target_link_libraries(piper fmt spdlog espeak-ng piper_phonemize onnxruntime ${PIPER_EXTRA_LIBRARIES})`, with the source comment `# NOTE: onnxruntime is pulled from piper-phonemize`. There is **no `find_package(onnxruntime)`** in the legacy file; `piper-phonemize` is fetched as an ExternalProject from `https://github.com/rhasspy/piper-phonemize/archive/refs/heads/master.zip` (fmt 10.0.0, spdlog 1.12.0).
- `piper.hpp` confirms the dependency at include level: `#include <onnxruntime_cxx_api.h>` and `#include <piper-phonemize/phonemize.hpp>`, `<piper-phonemize/phoneme_ids.hpp>`, `<piper-phonemize/tashkeel.hpp>` — <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.hpp>
- ONNX Runtime **session options actually used** by the current `libpiper/src/piper.cpp`: `DisableCpuMemArena()`, `DisableMemPattern()`, `DisableProfiling()`, **`SetIntraOpNumThreads(1)`**, **`SetInterOpNumThreads(1)`**, `SetGraphOptimizationLevel(ORT_ENABLE_BASIC)`, `SetExecutionMode(ORT_SEQUENTIAL)` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/piper.cpp>. The legacy code used `ORT_DISABLE_ALL` and carried these maintainer comments: `// Slows down performance by ~2x` (on `SetIntraOpNumThreads(1)`), `// Roughly doubles load time for no visible inference benefit` (on `ORT_ENABLE_EXTENDED`), `// Slows down performance very slightly` (on `ORT_PARALLEL`) — <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.cpp>. **No measured latency figures accompany these comments.**
- **Two different espeak-ng pins exist in the active repo:** `libpiper/CMakeLists.txt` pins `212928b394a96e8fd2096616bfd54e17845c48f6  # 2025-Mar-22`, while the root `CMakeLists.txt` (Python module) pins `724808c  # 2026-Apr-06` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt>, <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CMakeLists.txt>
- Voice models are trained with VITS and *"exported to the [onnxruntime](https://onnxruntime.ai/)"* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/VOICES.md>
- The legacy C++ build was *"Last tested with onnxruntime 1.14.1"* and expected the runtime at `lib/Linux-$(uname -m)` — <https://raw.githubusercontent.com/rhasspy/piper/v1.0.0/README.md>
- **libtashkeel is Arabic-only** (diacritisation), not part of the Chinese path — legacy release notes v1.0.0: *"Support Arabic diacritization with libtashkeel (model included)"* — <https://github.com/rhasspy/piper/releases.atom>. Current Arabic uses `src/piper/tashkeel/` — <https://api.github.com/repos/OHF-Voice/piper1-gpl/contents/src/piper>

**`libpiper` is a shared C-ABI library (not a CLI-only tool)** — it was introduced as an executable in 1.5.0 and is now built as a library:

- `add_library(piper SHARED ...)` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt>
- README title: *"Piper C/C++ API — A shared library for Piper with a C-style API."* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md>
- Changelog 1.5.0: *"Add `libpiper` C++ CLI executable ported from the legacy Piper repository, plus a C++ test suite"*; changelog 1.7.0: *"`libpiper`: add `piper_create_options` and `piper_create_with_options()`, with `piper_create()` kept as a wrapper for ABI compatibility"* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md>
- **The legacy repo published NO shared library at all.** Its root CMakeLists declares only `add_executable(piper src/cpp/main.cpp src/cpp/piper.cpp)` and `add_executable(test_piper src/cpp/test.cpp src/cpp/piper.cpp)`; there is no `add_library` of any kind, and `install(TARGETS ...)` installs the executable only — <https://raw.githubusercontent.com/rhasspy/piper/master/CMakeLists.txt>. So "libpiper as a shared library" is a **piper1-gpl (≥1.5.0) only** artifact.
- **The current CLI is a separate target named `piper_exe`**, not the library: `add_executable(piper_exe main.cpp)` with `target_link_libraries(piper_exe PRIVATE piper main_utils)` in `libpiper/src/main/CMakeLists.txt` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/CMakeLists.txt>
- Version 1.5.0 release body, verbatim from <https://api.github.com/repos/OHF-Voice/piper1-gpl/releases/tags/v1.5.0>: `- Add \`libpiper\` C++ CLI executable ported from the legacy Piper repository, plus a C++ test suite`

**There are TWO incompatible C++ APIs — the legacy one is not ABI-stable.**

Legacy `src/cpp/piper.hpp` sits entirely inside `namespace piper { … }` with **no `extern "C"` and no export/visibility macro**, and includes the ONNX Runtime C++ headers in the same translation unit (<https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.hpp>):

```cpp
void initialize(PiperConfig &config);                        // must be called before textTo* functions
void terminate(PiperConfig &config);
void loadVoice(PiperConfig &config, std::string modelPath,
               std::string modelConfigPath, Voice &voice,
               std::optional<SpeakerId> &speakerId, bool useCuda);
void textToAudio(PiperConfig &config, Voice &voice, std::string text,
                 std::vector<int16_t> &audioBuffer, SynthesisResult &result,
                 const std::function<void()> &audioCallback);
void textToWavFile(PiperConfig &config, Voice &voice, std::string text,
                   std::ostream &audioFile, SynthesisResult &result);
```

- Legacy structs: `PiperConfig{eSpeakDataPath, useESpeak=true, useTashkeel, tashkeelModelPath, tashkeelState}`, `SynthesisConfig{noiseScale=0.667f, lengthScale=1.0f, noiseW=0.8f, sampleRate=22050, sampleWidth=2, channels=1, speakerId, sentenceSilenceSeconds=0.2f, phonemeSilenceSeconds}`, `SynthesisResult{inferSeconds, audioSeconds, realTimeFactor}`, `ModelSession{Ort::Session onnx; …}`, `Voice` — same URL.
- **Legacy audio output is `std::vector<int16_t>` (16-bit mono).** The current C API outputs **32-bit `float`** (`const float *samples`). This is a breaking change between the two APIs — <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.hpp> vs <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h>
- **Latency instrumentation was dropped.** Legacy exposed `SynthesisResult::realTimeFactor`, computed as `result.audioSeconds = (double)audioCount / (double)synthesisConfig.sampleRate; … result.realTimeFactor = result.inferSeconds / result.audioSeconds;` and logged by the CLI as `"Real-time factor: {} (infer={} sec, audio={} sec)"` (<https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.cpp>, <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/main.cpp>). **The current `piper_audio_chunk` has no timing fields at all** — a C-API embedder must time chunks itself — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h>
- The internal (not installed) `libpiper/include/piper_impl.hpp` defines `enum class PhonemeType { Invalid = 0, Text, Espeak, Pinyin };` plus `DEFAULT_HOP_LENGTH = 256`, `ID_PAD = 0`, `ID_BOS = 1`, `ID_EOS = 2` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper_impl.hpp>
- `espeak_Initialize` is only called when `phoneme_type == "espeak"`; the `"text"` (Unicode NFD via `uni_algo`) and `"pinyin"` (g2pW dictionaries) paths **do not touch espeak-ng at all** — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/piper.cpp>. The legacy path always went through `espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, config.eSpeakDataPath.c_str(), 0)` plus `phonemize_eSpeak(...)` from `piper-phonemize` — <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.cpp>

**The current C++ API surface** (all `extern "C"`, `EXPORT_SYMBOL`, from <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h>):

```c
piper_synthesizer *piper_create(const char *model_path, const char *config_path,
                                const char *espeak_data_path);
piper_synthesizer *piper_create_with_options(const piper_create_options *options);
void piper_free(piper_synthesizer *synth);
piper_synthesize_options piper_default_synthesize_options(piper_synthesizer *synth);
int piper_synthesize_start(piper_synthesizer *synth, const char *text,
                           const piper_synthesize_options *options);
int piper_synthesize_next(piper_synthesizer *synth, piper_audio_chunk *chunk);
char const *piper_version(void);
```

- `piper_audio_chunk` carries `const float *samples`, `size_t num_samples`, `int sample_rate`, `bool is_last`, plus alignment data (`char32_t *phonemes`, `int *phoneme_ids`, `int *alignments`). Output is **raw 32-bit float mono** samples, not WAV — the README's comment is `// aplay -r 22050 -c 1 -f FLOAT_LE -t raw output.raw` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md>
- `piper_create_options` is versioned via `struct_size`, and has fields `model_path`, `config_path`, `espeak_data_path`, `g2pw_model_dir`, `data_dir`; initialise with the inline `piper_init_create_options()`. `espeak_data_path` is documented as *"Path to espeak-ng data directory, **or NULL if not needed (text phonemes)**"* — same URL.
- `piper_synthesize_options` exposes `speaker_id`, `length_scale` (*"0.5 means to speak twice as fast"*), `noise_scale`, `noise_w_scale` — same URL.
- Legacy equivalent (`piper.hpp` / `piper.cpp`) existed from v1.0.0: *"Merge code into `piper.hpp` and `piper.cpp` for use as a library"* — <https://github.com/rhasspy/piper/releases.atom>
- Legacy usage in `main.cpp` was documented as requiring a downloaded `piper-phonemize` tree: `lib/Linux-x86_64/piper_phonemize/lib/libpiper_phonemize.so` — <https://raw.githubusercontent.com/rhasspy/piper/v1.2.0/README.md>

**Does the C++ API need `espeak-ng-data` bundled? Yes — for the espeak path.**

- README: *"To use `libpiper`, you will need to: Include `piper.h` (`install/include/`) … Provide `piper_create` with **the path to espeak-ng's data (`install/espeak-ng-data/`)**"* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md>
- CMake installs both the library, the header, the onnxruntime shared libs, and the data directory: `install(DIRECTORY ${ESPEAKNG_DATA_SRC} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})`, where `ESPEAKNG_DATA_SRC = ${CMAKE_BINARY_DIR}/espeak_ng-install/share/espeak-ng-data` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt>
- espeak-ng is built **from source as a static library** at a pinned commit: `GIT_REPOSITORY https://github.com/espeak-ng/espeak-ng.git`, `GIT_TAG 212928b394a96e8fd2096616bfd54e17845c48f6  # 2025-Mar-22`, `-DBUILD_SHARED_LIBS:BOOL=OFF`, plus `-DUSE_MBROLA:BOOL=OFF -DUSE_LIBSONIC:BOOL=OFF -DUSE_KLATT:BOOL=OFF -DUSE_SPEECHPLAYER:BOOL=OFF` — same URL
- **Mandarin data is an opt-in espeak-ng build flag:** the same CMake passes **`-DEXTRA_cmn:BOOL=ON`** (alongside `-DEXTRA_ru:BOOL=ON`) — same URL. i.e. the `cmn` dictionary is not part of espeak-ng's default data set. The same CMake also disables MBROLA, libsonic, libpcaudio, Klatt and speechPlayer (`-DUSE_*:BOOL=OFF`) — so Piper's data directory is a reduced build, not the full espeak-ng data set.
- **Documentation/CMake path discrepancy (verified).** `libpiper/README.md` tells embedders the data lives at `install/espeak-ng-data/`, but `libpiper/CMakeLists.txt` installs it under the data root (`DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}`, default `share`), i.e. the actual installed path is **`<prefix>/share/espeak-ng-data`** — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md>, <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt>. The legacy repo installed it at the prefix root instead: `install(DIRECTORY ${PIPER_PHONEMIZE_DIR}/share/espeak-ng-data DESTINATION ${CMAKE_INSTALL_PREFIX})` → `<prefix>/espeak-ng-data` — <https://raw.githubusercontent.com/rhasspy/piper/master/CMakeLists.txt>
- **`piper1-gpl`'s source tree contains no `espeak-ng-data` directory.** The recursive tree API returns `"truncated":false` with **zero** paths matching `espeak-ng-data`; `.gitignore` contains the line `espeak-ng-data/`; and the root CMakeLists creates it at build time via a `copy_espeak_ng_data` target that copies into `src/piper/espeak-ng-data` — <https://api.github.com/repos/OHF-Voice/piper1-gpl/git/trees/main?recursive=1>, <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.gitignore>, <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CMakeLists.txt>. So there is **no checked-in file count to report** (0 files in-tree).
- The bundled CLI resolves the data directory relative to its own executable — `runConfig.eSpeakDataPath = std::filesystem::absolute(exePath.parent_path().append("espeak-ng-data"))`, throwing `"eSpeak data path not set"` otherwise — and the library additionally falls back to `data_dir + "/espeak-ng-data"` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/main.cpp>, <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/piper.cpp>. CLI flags: `--espeak_data DIR` and `--data_dir DIR` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/utils/main_utils.cpp>
- **`PIPER_ESPEAKNG_DATA_DIRECTORY` does not appear in any Piper source, CMake or README file inspected** (legacy `piper.hpp`/`piper.cpp`/`main.cpp`/`test.cpp`/root CMakeLists/Makefile/READMEs, and the piper1-gpl `libpiper/*`, docs and workflows). A whole-repo text grep was not performed, so an absolute negative is **not determinable from public sources**; within the inspected surface the variable does not exist. `ESPEAK_DATA_PATH` likewise appears only in a **third-party** Termux guide (`https://raw.githubusercontent.com/gyroing/piper-tts-for-termux/main/README.md`), which reads it via `std::getenv("ESPEAK_DATA_PATH")` and passes it to `piper_create`.
- `piper1-gpl`'s own Python wheel embeds espeak-ng through `espeakbridge.c` using Python's limited API/stable ABI; it relies on `espeak_TextToPhonemesWithTerminator` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/BUILDING.md>. (Note this bridge is the *Python wheel* mechanism; the `libpiper` shared library links espeak-ng directly.)

**Exact per-file figures for `espeak-ng-data` (no published aggregate)**

The only exact, verifiable figures found are per-file, from a real bundled copy of the tree in `csukuangfj/vits-piper-zh_CN-huayan-medium/espeak-ng-data/` (<https://huggingface.co/api/models/csukuangfj/vits-piper-zh_CN-huayan-medium/tree/main/espeak-ng-data>):

| File | Bytes |
| --- | --- |
| **`cmn_dict`** (compiled Mandarin dictionary) | **1,566,335** |
| `ru_dict` | 8,532,392 |
| `phondata` | 550,424 |
| `yue_dict` (Cantonese) | 563,571 |
| `en_dict` | 166,944 |
| `phontab` | 55,796 |
| `phonindex` | 39,074 |
| `phondata-manifest` | 21,821 |
| `intonations` | 2,040 |
| `th_dict` (Thai, for contrast) | 2,301 |

That directory also contains `lang/` (a 33-subdirectory language-family tree plus loose files) and `voices/!v`, all with further files, so the **total** size of `espeak-ng-data` could not be summed exactly from any published listing.

**Independent scale reference (not a Piper figure).** A downstream Linux packager does publish an installed size for the same data set: Debian `espeak-ng-data` version `1.52.0+dfsg-5` is reported as **Package Size 8,450.7 kB / Installed Size 24,034.0 kB**, identical for `amd64` and `arm64` — <https://packages.debian.org/stable/sound/espeak-ng-data> (file inventory: <https://packages.debian.org/trixie/amd64/espeak-ng-data/filelist>). **Caveat:** Piper builds espeak-ng from a pinned commit with MBROLA/Klatt/speechPlayer/libsonic/libpcaudio all disabled and `EXTRA_cmn`/`EXTRA_ru` enabled, so the Debian figure is indicative of magnitude only, not the size of Piper's own data directory.

**Piper-published size of `espeak-ng-data`: `not published`** — no byte size, file count or archive size appears in either repo's READMEs, docs, CHANGELOG or CMake files. It is also not derivable from the published wheels, because each wheel bundles the data *together with* ONNX Runtime in one archive (`piper_tts-1.8.0-cp39-abi3-macosx_10_9_x86_64.whl` = 34,111,822 B; `…manylinux_2_17_aarch64…whl` = 34,131,751 B — <https://api.github.com/repos/OHF-Voice/piper1-gpl/releases>). The legacy `piper-phonemize` release-asset sizes could not be obtained (that API call returned HTTP 403 rate-limit), so the legacy bundle size is **not determinable from public sources** in this pass.

**Prebuilt Android arm64 library: none official. No Android build support in the build system.**

- `libpiper/CMakeLists.txt` enumerates exactly these platforms for onnxruntime, with `message(FATAL_ERROR "Unsupported architecture for onnxruntime")` otherwise: **`WIN32`** (x64), **`APPLE`** (x86_64, arm64), **Linux** (`x86_64`, **`aarch64`**, `armv7l`). **There is no Android/NDK branch, no `ANDROID_ABI`, and no `arm64-v8a` target** — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt>. Note the Linux `aarch64` path pulls the *Linux glibc* onnxruntime tarball, which is not usable on Android/bionic.
- Release assets for **v1.8.0** are Python wheels and an sdist only: `piper_tts-1.8.0-cp39-abi3-macosx_10_9_x86_64.whl`, `…macosx_11_0_arm64.whl`, `…manylinux_2_17_aarch64…whl`, `…manylinux_2_17_x86_64…whl`, `…win_amd64.whl`, `piper_tts-1.8.0.tar.gz`. **No `.aar`, no `.so`, no Android asset** — <https://api.github.com/repos/OHF-Voice/piper1-gpl/releases>. The same is true of v1.7.0 and v1.6.0 (same URL).
- The legacy releases were Linux/Raspberry Pi binaries only: *"amd64 (64-bit desktop Linux) / arm64 (64-bit Raspberry Pi 4) / armv7 (32-bit Raspberry Pi 3/4)"* — <https://raw.githubusercontent.com/rhasspy/piper/v1.2.0/README.md>. No Android artifact.
- **`libpiper` is not published as a prebuilt shared library for ANY platform.** Although the target is declared `SHARED` and `install(TARGETS piper DESTINATION ${CMAKE_INSTALL_LIBDIR})` exists, no release asset in either repo is a `.so`/`.dll`/`.dylib`/`.aar`/`.apk`. CI uploads a `libpiper-${{ matrix.os }}` **workflow artifact** of `libpiper/install/` for ubuntu/macos/windows only — not a release asset, and with no Android variant — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.github/workflows/build-libpiper.yml>. CI OS matrix: build-libpiper `os: [ubuntu-latest, macos-latest, windows-latest]`; wheels `os: [ubuntu-latest, ubuntu-24.04-arm, windows-latest, macos-13, macos-latest]` — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.github/workflows/wheels.yml>
- **Issue-tracker evidence that Android is an open user request, never an implementation:**
  - `repo:rhasspy/piper+android` → `"total_count":16` — <https://api.github.com/search/issues?q=repo:rhasspy/piper+android&per_page=50>, including issue **#103 `[Request] Please make it run on Android`** (open, 8 comments) — <https://github.com/rhasspy/piper/issues/103>; issue **#180 `Piper on termux android`** (open, 18 comments) whose body reproduces `ld.lld: error: unable to find library -lc++_shared` on Android aarch64 — <https://github.com/rhasspy/piper/issues/180>; and issue **#370 `The ONNX multi-speaker model cannot work properly in an Android app`** (closed) — <https://github.com/rhasspy/piper/issues/370>
  - `repo:OHF-Voice/piper1-gpl+android` → `"total_count":6` — <https://api.github.com/search/issues?q=repo:OHF-Voice/piper1-gpl+android&per_page=50>, including issue **#130 `How to build and integrate into an Android Studio project?`** (open, **0 comments**) — <https://github.com/OHF-Voice/piper1-gpl/issues/130>; issue **#7 `New build - first impressions on the install process on Android`** (open, 20 comments; body records `ld.lld: error: unable to find library -lgcc` and a `phondata`/`phsource` compile error) — <https://github.com/OHF-Voice/piper1-gpl/issues/7>; PR **#21 `feat: Termux build improvements and unified CMakeLists.txt`** (open, `"merged_at":null` → **unmerged**) — <https://github.com/OHF-Voice/piper1-gpl/pull/21>; and issue **#78 `Implementing piper for mobile phone`** (open) — <https://github.com/OHF-Voice/piper1-gpl/issues/78>
- **Termux does not package Piper.** `https://raw.githubusercontent.com/termux/termux-packages/master/packages/piper/build.sh` → **HTTP 404** and `…/packages/piper-tts/build.sh` → **HTTP 404**, while the control path `…/packages/espeak/build.sh` → HTTP 200 (`TERMUX_PKG_VERSION="1.52.0"`, `TERMUX_PKG_LICENSE="GPL-2.0"`). So Termux packages espeak-ng but not Piper — <https://raw.githubusercontent.com/termux/termux-packages/master/packages/espeak/build.sh>. Only those exact paths were probed, so a differently-named package cannot be fully excluded. (A dedicated `repo:rhasspy/piper+termux` issue count returned HTTP 403 rate-limit and is **not determinable** in this pass.)

**Third-party Android paths that do exist — all clearly NON-official:**

| Project | What it is | Android arm64 | Chinese support | Cross-check |
| --- | --- | --- | --- | --- |
| **sherpa-onnx** (`k2-fsa`) | **A separate engine** that re-implements Piper-model inference; it does **not** use Piper's code or `piper-phonemize` | **Yes** — publishes **arm64-v8a APKs including Mandarin Piper models**: `sherpa-onnx-1.13.7-arm64-v8a-zho-tts-vits-piper-zh_CN-huayan-medium.apk`, `…zh_CN-chaowen-medium.apk`, `…zh_CN-xiao_ya-medium.apk` | Yes, 3 of the 4 Chinese voices | <https://k2-fsa.github.io/sherpa/onnx/tts/apk.html> (states *"Models from piper have their names prefixed with **vits-piper-**"*), APK host prefix <https://huggingface.co/csukuangfj2/sherpa-onnx-apk/resolve/main/tts-new/1.13.7/>, source <https://github.com/k2-fsa/sherpa-onnx/tree/master/android/SherpaOnnxTts>; non-dependency stated in <https://github.com/rhasspy/piper/issues/251> |
| **`nihui/ncnn-android-piper`** | Third-party Android sample on **Tencent ncnn**, *not* ONNX Runtime | Yes (ships an APK) | Not specified; *"This project uses a custom dictionary to implement phonemizer"* | <https://raw.githubusercontent.com/nihui/ncnn-android-piper/master/README.md> |
| **`gyroing/piper-tts-for-termux`** | Third-party Termux guide that builds the **current** `piper1-gpl` `libpiper.so` by hand, links it against Microsoft's `onnxruntime-android-1.22.0.aar` (Maven Central) and runs `patchelf --replace-needed libonnxruntime.so.1 libonnxruntime.so install/lib/libpiper.so` | Yes (hand-built) | Not specified; default voice is `zh_CN-huayan-medium` in the guide | <https://raw.githubusercontent.com/gyroing/piper-tts-for-termux/main/README.md> — proves a third party produced an Android arm64 `libpiper.so`; upstream still has no support |
| **`jvoice-project/piper-jni`** | The Java/JNI binding that the **official** piper1-gpl README lists under *"Bindings to use Piper in programming languages other than Python and C/C++"* | **No** — its platform list is Windows x86_64, Linux x86_64/arm64, macOS x86_64/arm64 | n/a | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/README.md>, <https://raw.githubusercontent.com/jvoice-project/piper-jni/main/README.md> |
| **VoxSherpa TTS** | Third-party offline Android TTS app using Piper models via sherpa-onnx, referenced in PR #224 — which has `"merged_at":null` (closed unmerged), so it is **not** in the README as fetched | Yes | Not specified | <https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/224> |

**Context, not a Piper artifact:** Microsoft ships an official Android ONNX Runtime AAR at the exact version `libpiper` pins — `onnxruntime-android-1.22.0.aar` = **28,515,295 bytes**, dated 2025-05-09 — <https://repo1.maven.org/maven2/com/microsoft/onnxruntime/onnxruntime-android/1.22.0/>.
- A useful build-system escape hatch (fact, not advice): the onnxruntime platform block in `libpiper/CMakeLists.txt` is guarded by `if(NOT TARGET onnxruntime::onnxruntime AND NOT DEFINED ONNXRUNTIME_DIR)`. If `ONNXRUNTIME_DIR` is predefined, the platform dispatch is skipped and only `find_library(ONNXRUNTIME_LIB NAMES onnxruntime PATHS ${ONNXRUNTIME_DIR}/lib REQUIRED)` runs — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt>
- **Summary: no official Piper Android library, AAR, APK, NDK toolchain file or Android CI job exists in either repository.** The only prebuilt arm64-v8a Mandarin-Piper-capable Android artifacts found are **sherpa-onnx** APKs, which are a different engine.

### A6. Published speed / RTF numbers

**Piper itself publishes no RTF or latency figure.** The upstream READMEs claim only qualitative speed:

- *"A fast, local neural text to speech system that sounds great and is **optimized for the Raspberry Pi 4**"* — <https://raw.githubusercontent.com/rhasspy/piper/v1.2.0/README.md> and <https://raw.githubusercontent.com/rhasspy/piper/v1.0.0/README.md>
- The current README says only *"A fast and local neural text-to-speech engine that embeds espeak-ng for phonemization"* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/README.md>
- **A first-party RTF number for Piper: `not published`.** No RTF figure for any voice quality appears in either repo's README, docs, CHANGELOG or CMake files, and the samples page has no RTF column (see A2).
- RTF existed only as a **runtime measurement in the legacy API**, not as a published benchmark: `result.audioSeconds = (double)audioCount / (double)synthesisConfig.sampleRate; … result.realTimeFactor = result.inferSeconds / result.audioSeconds;` and the CLI log line `"Real-time factor: {} (infer={} sec, audio={} sec)"` — <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.cpp>, <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/main.cpp>. **The current C API removed it** — `piper_audio_chunk` has no timing fields — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h>
- **No published quantisation scheme exists for Piper voice `.onnx` files.** The word "quantized" appears in the changelog only for the Chinese g2pW *text* model (1.4.0, `quantize_dynamic`). The voices themselves are *"trained with VITS and exported to the onnxruntime"* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/VOICES.md>, <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md>. (Third-party int8/fp16 re-exports of two Chinese voices do exist — see A3.)

**A credible third-party RTF table does exist, with named hardware.** sherpa-onnx publishes RTF for VITS/Piper models measured on **`Raspberry Pi 4 Model B Rev 1.5`**, by number of threads, with model size:

| Model | 1 thread | 2 | 3 | 4 | Size |
| --- | --- | --- | --- | --- | --- |
| `vits-piper-en_US-lessac-medium` | **0.774** | 0.482 | 0.390 | 0.357 | 61 MB |
| `vits-piper-en_US-glados` | 0.812 | 0.480 | 0.391 | 0.349 | 61 MB |
| `vits-piper-en_US-libritts_r-medium` | 0.790 | 0.493 | 0.392 | 0.357 | 75 MB |
| `vits-melo-tts-zh_en` | 6.727 | 3.877 | 2.914 | 2.518 | 163 MB |
| `matcha-icefall-zh-baker` | 0.892 | 0.536 | 0.432 | 0.391 | 73 MB |
| `vits-model-aishell3` | 0.365 | 0.220 | 0.171 | 0.156 | 30 MB |

Source: <https://raw.githubusercontent.com/k2-fsa/sherpa/master/docs/source/onnx/tts/pretrained_models/rtf.rst> (fetched from the `k2-fsa/sherpa` master docs; the same file is served in the sherpa-onnx docs tree). RTF < 1 means faster than real time.

- **RTF for a `zh_CN` Piper voice specifically: `not published`.** The `zh_CN-*-medium` Piper models are 63,201,294–63,221,984 bytes, i.e. the same "medium" architecture/size class as `en_US-lessac-medium` (61 MB) in the table above, but that is a size comparison, not a measurement of the Chinese models.
- **RTF for Piper on Android/arm64 phoneme hardware: `not published`** in any source found.

---

## Topic B — eSpeak-NG Mandarin support quality

### B1. Official status of Mandarin (`cmn` / `zh`)

- `docs/languages.md` lists eSpeak-NG's languages in a table and states *"Development version of eSpeak NG supports 127 languages and accents"*. The Mandarin row is: `sit` | **`cmn`** | Sino-Tibetan | Chinese | **Mandarin**. Related Chinese rows are `sit`/`yue`/Cantonese and `sit`/`hak`/Hakka — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages.md>
- So: **Mandarin is listed as an officially supported language**, identified by the BCP 47 primary subtag `cmn` (not `zh`), grouped under ISO 639-5 family `sit` (Sino-Tibetan).
- **The documentation states nothing about Mandarin quality or limitations.** In that table, seven footnotes qualify various languages (Latin/Cyrillic/Arabic alphabet notes, Japanese *"With Hiragana and Katakana syllabary"*, etc.); **`cmn` carries no footnote at all**, and no prose caveat about Mandarin appears anywhere in `docs/languages.md` — same URL. Two structural confirmations that this is not merely a terse file:
  - There is **no** `docs/languages/sit/cmn.md` (HTTP 404) — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages/sit/cmn.md>
  - `docs/index.md` links per-language notes only for English and Cherokee, so Mandarin has no dedicated documentation page at all — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/index.md>
- **The identifier `zh` does not appear in `docs/languages.md` at all** — only `cmn`, `yue` and `hak`. The `zh` alias is declared inside the voice data instead: the in-repo voice file `espeak-ng-data/lang/sit/cmn` contains `language cmn` / `language zh-cmn` / `language zh`, alongside `dictionary cmn`, `words 1`, `dict_min 100000` — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/lang/sit/cmn>. So `cmn` and `zh` resolve to the same dictionary in current eSpeak-NG, with `cmn` the documented name. (The default branch of `espeak-ng/espeak-ng` is `master` — <https://api.github.com/repos/espeak-ng/espeak-ng>.)
- Where eSpeak-NG quality *is* documented, it is documented for other languages, which makes the silence about Mandarin notable rather than an artefact of the docs' style:
  - Thai: the espeak-ng maintainer's own statement is cited by Piper as *"espeak-ng's Thai voice is a placeholder: its `th_dict` holds no lexicon, so unspaced Thai is never segmented; the leading vowels … are not reordered; and a tone mark deletes the syllable's vowel, collapsing ป่า/ป้า/ป๊า/ป๋า to the same phonemes"* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md>, and the underlying espeak-ng issue is <https://github.com/espeak-ng/espeak-ng/issues/757>.
  - Japanese: *"espeak-ng has no kanji coverage (it reads out Unicode character names) and no pitch accent"* — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md>
- The `zh` identifier is used as an espeak-ng voice name in practice. Evidence it resolves to the same Mandarin translator as `cmn`: `OHF-Voice/piper1-gpl` PR #128 uses the locale string `"cmn"` in a test named for the Chinese voice — `("cmn", "你好，世界！省略号……一、二，三。", …)` — <https://github.com/OHF-Voice/piper1-gpl/issues/128>; and espeak-ng PR #340 describes `SelectTranslator` being *"called 2 times for both Chinese dialects, for first call it loads default settings with `zh` for `yue` and `cmn`"* — <https://github.com/espeak-ng/espeak-ng/pull/340>. So `zh` is the shared base voice, and `cmn` / `yue` select the Mandarin / Cantonese dictionaries on top.
- Open user-facing question about this exact ambiguity, still unanswered in the piper tracker: issue **#505 "Is the Chinese language code currently using cmn or zh_cN?"** (open, 1 comment, opened 2024-05-21) — <https://github.com/rhasspy/piper/issues/505>

### B2. Tones, the Han→pinyin mechanism, and 多音字

**Does eSpeak-NG handle Chinese tones? Yes — tones and even third-tone sandhi are produced.**

- espeak-ng issue **#364** (closed with label `resolved/fixed`) contains a direct demonstration from the espeak-ng `-x -X` trace, showing Mandarin third-tone sandhi applying automatically. Input 雨傘: the trace shows `Replace: 雨   yu3` / `Replace: 傘   san3`, then the output line `;'y35_| s'a214n_|` with the comment *"#'y214 is changed to 'y35 automatically"*. The reporter states: *"However, the tone sandhi of Mandarin (the tone of tone-214 syllable in front of another tone-214 tone is changed to tone-35) **works**… Even though the tone sandhi rule is not defined in `zh_list` or `zh_rules`, **the tone-sandhi rule works**."* — <https://github.com/espeak-ng/espeak-ng/issues/364>
- Tone is carried as a trailing **digit on the pinyin syllable** in the dictionary, i.e. `yu3`, `san3`, `di4mian4`. Formats visible in primary sources:
  - character entries: `Replace: 雨   yu3`, `Replace: 面   mian4` — <https://github.com/espeak-ng/espeak-ng/issues/364> and <https://github.com/espeak-ng/espeak-ng/issues/606>
  - **multi-character word entries**: `(地 面) di4mian4` — a parenthesised multi-syllable rule — <https://github.com/espeak-ng/espeak-ng/issues/606>
  - the tone digits are then expanded to tone contours by the phoneme table: the trace in #606 shows `22  299:  4 [51]` and `22  299:  3 [214]` — i.e. tone 4 → `[51]`, tone 3 → `[214]` — <https://github.com/espeak-ng/espeak-ng/issues/606>. (Corroborated by the Piper-side issue #305, which lists the mapping as *"1(55), 2(35), 3(214), 4(51), 5(11)"* — <https://github.com/rhasspy/piper/issues/305>.)
**The actual source files (the brief's `zh_listx` / `zh_pinyin` filenames do not exist — they were renamed `zh` → `cmn`)**

- `dictsource/zh_listx`, `dictsource/zh_pinyin` and `dictsource/zh_rules` all return **HTTP 404**; the rename from `zh_*` to `cmn_*` (and `yue_*`, `hak_*`) was espeak-ng **PR #940, merged** — <https://github.com/espeak-ng/espeak-ng/pull/940>
- The Mandarin source files that do exist, with exact published byte sizes: **`dictsource/cmn_list` = 36,237 B**, **`dictsource/cmn_rules` = 3,997 B**, **`dictsource/cmn_emoji` = 91,353 B**, plus **`dictsource/extra/cmn_listx`** — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_list>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_rules>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_emoji>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/extra/cmn_listx>
- **`cmn_list` is one Han character per line → pinyin + tone digit**, declared `$textmode`. Its own header states it covers *"Most frequent pronunciations of the 3799 most common characters (from Unihan … kHanyuPinlu field …)"*. Literal entries: `涉⇥she4`, `五⇥wu3`, `的⇥de5` (5 = neutral tone) — same URL.
- **`cmn_rules` performs the tone-digit → tone-contour mapping**: `1->55`, `2->35`, `3->214`, `4->51`, `5->11`, and contains a `.replace` block converting pinyin tone diacritics into digits, carrying the in-file comment `// TODO: àn is not handled` — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_rules>. (This matches the mapping quoted independently from the Piper side in issue #305: *"1(55), 2(35), 3(214), 4(51), 5(11)"* — <https://github.com/rhasspy/piper/issues/305>.)

**多音字 (polyphonic characters): the mechanism is a separate "extra" list, and it IS compiled by default**

- `cmn_list` itself is **single-reading by construction** (one pronunciation per character). Context-sensitive readings live in **`dictsource/extra/cmn_listx`**, whose own header states it contains *"21611 single characters plus 36500 compound exceptions (includes 320 added 'yi' and 10721 added 'bu' exceptions, and 9700 extra 2-syllable words for 3rd-tone sandhi blocking)"* — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/extra/cmn_listx>. That is the whole of eSpeak-NG's Mandarin polyphone/sandhi exception apparatus: **36,500 compound exceptions** plus **21,611 single characters**.
- That file is **compiled by default** in the espeak-ng build: `cmake/data.cmake` defines `EXTRA_cmn` **ON** — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/cmake/data.cmake>. (Piper's `libpiper` also passes `-DEXTRA_cmn:BOOL=ON` explicitly — see A5 — consistent with this.)
- **The compiled dictionary is generated at build time, not checked in.** `espeak-ng-data/cmn_dict` returns **HTTP 404** in the source repo; it is produced by `${DATA_DIST_DIR}/${_dict_name}_dict` in `cmake/data.cmake` — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/cmn_dict> (404), <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/cmake/data.cmake>. Its **compiled size as shipped is 1,566,335 bytes** (`cmn_dict` inside a real bundled `espeak-ng-data` tree — <https://huggingface.co/api/models/csukuangfj/vits-piper-zh_CN-huayan-medium/tree/main/espeak-ng-data>); for scale in that same tree this is larger than `en_dict` (166,944 B) and `phondata` (550,424 B) but far smaller than `ru_dict` (8,532,392 B).
- **There is NO jieba/pypinyin-style word segmentation in eSpeak-NG's Mandarin path.** The multi-word path relies on the dictionary's own multi-word entries, and a **merged maintainer PR states the requirement explicitly**: *"each hanzi must be a separate word so that words can match their multi-word `*_list` entries, which map them to pinyin/jyutping via a second `$textmode` replacement"*, with the fix making *"ideographs become separate words"* — **espeak-ng PR #2455** — <https://github.com/espeak-ng/espeak-ng/pull/2455>
- An external word segmenter (jieba) was proposed by a Piper contributor precisely because eSpeak-NG has none: *"I'm working on applying some other word segmentation module like jieba to piper-phonemize, because the Chinese dict in espeak-ng is far too small and rigid"* — <https://github.com/rhasspy/piper/issues/164>

**多音字, part 2 — the multi-word matching bug, and downstream evidence that the path is broken**

- **Answering the mechanism question directly: yes, there is a built-in Han→pinyin dictionary** (`cmn_list`, single-reading, 3,799 characters) **plus a compound-exception list** (`extra/cmn_listx`, 36,500 compound exceptions); context-dependent readings are expressed as **multi-character dictionary entries** rather than by any context model.
- The word-rule syntax is a parenthesised multi-syllable entry: `(地 面) di4mian4`, meant to fix the reading of both characters as a unit — <https://github.com/espeak-ng/espeak-ng/issues/606>
- espeak-ng issue **#606, "Chinese dictionary multiple match"** (opened 2019-04-09 by rongcuid, 3 comments, **still open**) reports that this mechanism does not work. The reporter's trace shows `地面` being correct for the word rule and *then* `面` being re-translated anyway, producing duplicate phonemes (`ti53m'iE51n_| m'iE51n_|`). Their root-cause analysis: *"the problem comes from `dictionary.c:LookupDict2`, which sets the global variable `dictionary_skipwords`. In a GDB session, I notice that `dictionary_skipwords` is set to 1, instead of an expected 2, which means that each character would be looked up and translated."* — <https://github.com/espeak-ng/espeak-ng/issues/606>
- **Critically for multi-character (and therefore polyphone) rules, #606 also reports that the extended list file was not being loaded at all:** *"That is done so that `zh_listx` is actually loaded. **In this repo, `zh_listx` is not loaded and has no effect.** If you want to fix this issue without trying my commit, then you may need to modify `zh_extra` to add in the entry I described."* — same URL. (This observation is from 2019 against the then-current `zh_*` filenames.) **Current-state nuance:** the extra list does now appear to be compiled in by default (`EXTRA_cmn` ON in `cmake/data.cmake` — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/cmake/data.cmake>), the files were renamed by merged PR #940 (<https://github.com/espeak-ng/espeak-ng/pull/940>), and merged PR #2455 changed segmentation so that *"ideographs become separate words"* to let multi-word entries match (<https://github.com/espeak-ng/espeak-ng/pull/2455>). Issue **#606's core `dictionary_skipwords` bug, however, is still open**, so whether the multi-word/polyphone path is now correct end-to-end is **not determinable from public sources** on this evidence.
- The attempted fix was never merged: **PR #738 "Improves on some Chinese pronunciation"** by the same reporter (rongcuid), *"I improved part of the Chinese dictionaries and their priorities, fixing some common unclear/wrong pronunciations. These are not comprehensive, just some I discovered when using espeak-ng for my project."* — **closed, `merged_at: null`** (closed 2020-05-04) — <https://github.com/espeak-ng/espeak-ng/pull/738>
- **Downstream confirmation that polyphone disambiguation is the reason Chinese needed a different front-end:** Piper's own C++ `chinese_phonemizer.cpp` names the hard cases explicitly — *"This prevents 重庆/银行/长江 from being mis-assigned"* — and falls back to the **first reading** for polyphonic characters — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/chinese_phonemizer.cpp>. The Python path replaced exactly this with the g2pW contextual model (<https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/src/piper/phonemize_chinese.py>).
- Related mechanism-level defects reported and their outcomes:
  - Non-Han text inside Mandarin: issue **#347, "Chinese Mandarin: English words pronunciation problem"** (closed `completed`) — English words rendered as `55Nl'i55s._| l,i55th@55z.,A55thu55z.'o-55_` instead of English phonemes — <https://github.com/espeak-ng/espeak-ng/issues/347>
  - Numbers: issue **#257, "Mistake in pronounced of 9 in Mandarin Chinese"** (opened 2017-05-08, **still open**, labels `bug` + `languages/pronunciation`, 12 comments) — *"9 is pronounced, as xiǔ, as a number, not character (logogram). Logogram for nine in chinese is pronounced corectly, as 'jiǔ', but normal arabic numerals not."* — <https://github.com/espeak-ng/espeak-ng/issues/257>
  - Decimal points: issue **#1669, "Add _dpt pronounce for cmn_list(chinese), But didn't work"** (closed `completed`, 2023-03-03) — *"Decimal points are not supported in `dictsource/cmn_list` (Chinese), so I try to add in `dictsource/cmn_list` that refers to `en_list`… I Add `_dpt dian3`… it speaks 'liN35 liN35', WHERE IS THE 'dian3'?"* — <https://github.com/espeak-ng/espeak-ng/issues/1669>
  - Fallback behaviour for uncovered Han characters is documented at the source level by a *merged* espeak-ng PR: in the Arabic voice, *"Chinese characters (Hanzi) were previously spoken by reading their descriptive character names (e.g., 'Chinese letter') instead of pronouncing them properly"* — fixed by falling back to the `zh` voice — **espeak-ng PR #2511, merged 2026-09-01** — <https://github.com/espeak-ng/espeak-ng/pull/2511>. A user-side report of the same symptom from the Japanese voice path is issue **#1851, "it says 'Chinese Letter'"** (open, 14 comments) — <https://github.com/espeak-ng/espeak-ng/issues/1851>

### B3. Concrete GitHub issues about Chinese/Mandarin pronunciation

Method: `api.github.com/search/issues` with `repo:` qualifiers. Totals returned: **19** Chinese-matching items in `rhasspy/piper` (<https://api.github.com/search/issues?q=repo%3Arhasspy%2Fpiper+chinese&per_page=30>), **16** in `OHF-Voice/piper1-gpl` (<https://api.github.com/search/issues?q=repo%3AOHF-Voice%2Fpiper1-gpl+chinese&per_page=30>), **19** Chinese-in-title items in `espeak-ng/espeak-ng` (<https://api.github.com/search/issues?q=repo%3Aespeak-ng%2Fespeak-ng+chinese+in%3Atitle&per_page=40>).

**eSpeak-NG — pronunciation/comprehension defects**

| # | Title | State | Labels / disposition | URL |
| --- | --- | --- | --- | --- |
| 257 | Mistake in pronounced of 9 in Mandarin Chinese | **open** (2017-05-08, 12 comments) | `bug`, `languages/pronunciation` | <https://github.com/espeak-ng/espeak-ng/issues/257> |
| 606 | Chinese dictionary multiple match | **open** (2019-04-09, 3 comments) | no labels; `dictionary_skipwords`=1 instead of 2, so multi-word rules consume only one word | <https://github.com/espeak-ng/espeak-ng/issues/606> |
| 815 | Wrong tone sandhi | **open** | concrete third-tone sandhi defect: 好旅馆 yields `X'Au35` where `X'Au214` is expected | <https://github.com/espeak-ng/espeak-ng/issues/815> |
| 1028 | The pronunciation of Mandarin Chinese using ESpeak NJ in NVDA is not normal | closed | 10 comments; tones read aloud as English numbers — *"今One 天 One 的 Five…"* | <https://github.com/espeak-ng/espeak-ng/issues/1028> |
| **1163** | Please default ESpeak NG's voice role to "Chinese (Mandarin, latin as Pinyin)" for Chinese to fix #1028 | closed | **strongest quality complaint inside eSpeak-NG's own tracker**: body states *"#1028 makes ESpeak **completely unusable for Chinese users**."* | <https://github.com/espeak-ng/espeak-ng/issues/1163> |
| **1805** | Mandarin Pinyin issue | **open** (2 comments) | **the only eSpeak-NG issue returned for the queries `多音字` and `polyphone`** — i.e. the polyphone term appears only here; comment bodies not retrieved | <https://github.com/espeak-ng/espeak-ng/issues/1805> |
| **1044** | questions about mandarin data packet | closed (19 comments) | reports `Full dictionary is not installed for 'zh'` and `Error processing file 'zh_rules': No such file or directory` **while `cmn_rules`/`cmn_list` exist** — a `zh` vs `cmn` data-naming failure mode | <https://github.com/espeak-ng/espeak-ng/issues/1044> |
| 664 | the Chinese pronunciation is wrong | closed | — | <https://github.com/espeak-ng/espeak-ng/issues/664> |
| 1236 | Recognition of Chinese characters Error | closed | `-v cmn "好"` sounds wrong | <https://github.com/espeak-ng/espeak-ng/issues/1236> |
| 1370 | Cmn voice not correctly translated | closed | fix tracked in #1404 | <https://github.com/espeak-ng/espeak-ng/issues/1370> |
| 361 | can't split the syllables accurately & show the tone number | closed (same day, 2017-12-29) | — | <https://github.com/espeak-ng/espeak-ng/issues/361> |
| 1793 | dictrules for cmn is broken | closed | — | <https://github.com/espeak-ng/espeak-ng/issues/1793> |
| 1275 | Conversion from text to IPA phonemes seems incorrect when phonemes include tone changes | **open** | tone numerals mangled in IPA output | <https://github.com/espeak-ng/espeak-ng/issues/1275> |
| 1831 | Cantonese did not return as international phonetic alphabet (IPA), but jyutping instead. (with --ipa) | **open** | — | <https://github.com/espeak-ng/espeak-ng/issues/1831> |
| 2160 | Using mb-cn1 does not work well !!! | **open** | MBROLA `mb-cn1` voice | <https://github.com/espeak-ng/espeak-ng/issues/2160> |
| 788 (PR) | cmn: use voiceless uvular fricative as pronunciation of pinyin h | merged 2020-07-05 | indicates the pinyin `h` realisation was itself revised | <https://github.com/espeak-ng/espeak-ng/pull/788> |
| 933 | Unable to compile zhy dictionary on Windows | closed (15 comments) | led to PR #940 | <https://github.com/espeak-ng/espeak-ng/issues/933> |
| 1665 | Error trying to build latest stable release (espeak-ng 1.51) | closed | contains evidence of a **`--without-extdict-cmn` configure flag**, i.e. the Mandarin extra dictionary can be disabled at build time | <https://github.com/espeak-ng/espeak-ng/issues/1665> |
| 940 (PR) | `zh_*` → `cmn_*` file rename | **closed, merged** | explains why `zh_listx`/`zh_pinyin`/`zh_rules` 404 today | <https://github.com/espeak-ng/espeak-ng/pull/940> |
| 2455 (PR) | translate: segment `$textmode` replacement text like normal input | **closed, merged 2026-07-22**; **maintainer-authored** (`alex19EP`, MEMBER) | *"each hanzi must be a separate word so that words can match their multi-word `*_list` entries"*; fix makes *"ideographs become separate words"* | <https://github.com/espeak-ng/espeak-ng/pull/2455> |
| 685 | Voice Chinese (Mandarin): some characters are reported two times | closed | `completed` 2022-09-25 | <https://github.com/espeak-ng/espeak-ng/issues/685> |
| 347 | Chinese Mandarin: English words pronunciation problem | closed | `completed` 2022-09-24 | <https://github.com/espeak-ng/espeak-ng/issues/347> |
| 1669 | Add `_dpt` pronounce for `cmn_list`(chinese), But didn't work | closed | `completed` 2023-03-03 | <https://github.com/espeak-ng/espeak-ng/issues/1669> |
| 338 | eSpeak-ng 1.49.2: Chinese Mandarin dictionary (`zhy_dict`) problem | closed | `bug` + `resolved/fixed`, milestone **1.49.3** | <https://github.com/espeak-ng/espeak-ng/issues/338> |
| 346 | Big bug in Mandarin Chinese | closed | `resolved/duplicate` | <https://github.com/espeak-ng/espeak-ng/issues/346> |
| 348 | Bug around Chinese Mandarin in Espeak has been discovered | closed | `resolved/not-an-espeakng-bug` | <https://github.com/espeak-ng/espeak-ng/issues/348> |
| 499 | The Mandarin Chinese language cannot read the Chinese characters | closed | `resolved/duplicate` | <https://github.com/espeak-ng/espeak-ng/issues/499> |
| 364 | How to make tone sandhi works like Mandarin Chinese | closed | `bug` + `resolved/fixed` (evidence Mandarin sandhi *works*) | <https://github.com/espeak-ng/espeak-ng/issues/364> |
| 1851 | it says "Chinese Letter" | **open** (14 comments) | none | <https://github.com/espeak-ng/espeak-ng/issues/1851> |
| 2151 | Does it support mixed Chinese and English streaming mode? | **open** (0 comments) | none | <https://github.com/espeak-ng/espeak-ng/issues/2151> |
| 738 (PR) | Improves on some Chinese pronunciation | **closed, not merged** (`merged_at: null`) | dictionary priority fixes abandoned | <https://github.com/espeak-ng/espeak-ng/pull/738> |
| 2511 (PR) | fix(ar): fallback Chinese characters to Mandarin (zh) pronunciation | **closed, merged** 2026-09-01 | fixes Han→"Chinese letter" naming | <https://github.com/espeak-ng/espeak-ng/pull/2511> |
| 340 (PR) | Fix for issue #338: Chinese Cantonese dictionary (`zhy_dict`) problem | closed, merged | `SelectTranslator` loads `zh` then `zhy`; confirms `zh` base voice | <https://github.com/espeak-ng/espeak-ng/pull/340> |

**Maintainer responses:** for the pronunciation-defect issues above, **no maintainer reply asserting Mandarin quality is acceptable was found**, and none of #257, #606, #815, #1805 or #1851 has been closed. #338 and #364 were fixed for the `1.49.3` milestone; #346/#348/#499 were closed as duplicate/not-a-bug; **#738 (the dictionary-quality improvement PR) was closed without merge**. **Comment bodies for the open Mandarin defects (#606, #815, #1805, and `rhasspy/piper` #164) could not be retrieved** — GitHub's non-search REST endpoints returned HTTP 403 rate-limit — so a maintainer reply on those specific threads is **not determinable from public sources**.
- **Maintainer-side PRs that DO carry verbatim statements** (identity as returned by the API): espeak-ng **PR #2455**, authored by maintainer `alex19EP` (MEMBER) — merged, and it states the hanzi-segmentation requirement quoted in B2 (<https://github.com/espeak-ng/espeak-ng/pull/2455>); espeak-ng **PR #340**, authored by `valdisvi` (MEMBER) — merged, fixing the Chinese dictionary selection (<https://github.com/espeak-ng/espeak-ng/pull/340>); and Piper **PRs #271 / #289 / #269**, which carry the maintainer-team statements quoted in A4. espeak-ng PR #940's body adds: *"There are no tests for Cantonese… Checks for Mandarin pass."* — <https://github.com/espeak-ng/espeak-ng/pull/940>
- The most explicit statement anywhere that an eSpeak-NG voice is unusable comes from an eSpeak-NG **user**, in an eSpeak-NG issue, about Mandarin: *"#1028 makes ESpeak completely unusable for Chinese users."* — <https://github.com/espeak-ng/espeak-ng/issues/1163>. The most explicit **maintainer** statement that a voice is poor concerns **Thai** (issue #757, <https://github.com/espeak-ng/espeak-ng/issues/757>), cited by Piper, not Mandarin.

**`rhasspy/piper` — Chinese pronunciation/prosody complaints**

| # | Title | State | Notes (quoted) | URL |
| --- | --- | --- | --- | --- |
| 164 | Better Chinese phonemization | **closed `not_planned`** (2024-09-14, 8 comments) | *"the Chinese dict in espeak-ng is far too small and rigid"* | <https://github.com/rhasspy/piper/issues/164> |
| 305 | When espeak-ng translates Chinese (cmn), IPA tone symbols are not output correctly | **open** (2023-12-11, 3 comments, 👍2) | tone-symbol bug in `dictionary.c WritePhmnemonic`; **and** *"I feel the `zh_CN_huayan_medium.onnx` output voice, **the tone sounds strange**. In chinese there are 5 tones by number: 1 to 5… But in huayan model, **tone 1 sometimes sounds like tone 2, and tone 4 sometimes sounds like tone 1**. And the total sentence sounds strange."* | <https://github.com/rhasspy/piper/issues/305> |
| 278 | More natural Chinese voice, Please | **open** (2023-11-22, 3 comments, 👍1) | *"English is very natural, but **Chinese has an English accent and seems unnatural**. The segmentation of sentence pauses feels a bit mechanical."* / *"when I select the Chinese model, mixed reading of Chinese and English is not supported"* | <https://github.com/rhasspy/piper/issues/278> |
| 243 | incorrect output for simple Chinese phrase? | **open** (2023-10-14, 1 comment) | `echo "一点儿" \| piper-tts --model …/zh_CN-huayan-medium.onnx` → *"the output is incorrect"*, with a reference recording for comparison | <https://github.com/rhasspy/piper/issues/243> |
| 652 | Training a new model, hoping to receive assistance | **closed `not_planned`** (2024-11-24) | *"I am training a new Chinese model for Piper because **the pronunciation tone of the Piper project's Chinese model is incorrect**."* | <https://github.com/rhasspy/piper/issues/652> |
| 835 | Error pause for Chinese | **open** (`state_reason: reopened`, 0 comments, locked) | pause placement wrong in generated audio | <https://github.com/rhasspy/piper/issues/835> |
| 613 | Fine-tuning an English checkpoint (ckpt) using a Chinese speech dataset | **open** (4 comments) | *"I have a 85 hr of Chinese audio voice at 44100 hz to fintuning en-us/lessac/medium .ckpt, **but effect not good**."* | <https://github.com/rhasspy/piper/issues/613> |
| 740 | Does it support mixed Chinese and English streaming mode? | **open** (1 comment) | no body | <https://github.com/rhasspy/piper/issues/740> |
| 505 | Is the Chinese language code currently using cmn or zh_cN? | **open** (1 comment) | unanswered locale ambiguity | <https://github.com/rhasspy/piper/issues/505> |
| 34 | How to use zh-cn-huayan-low.onnx | closed `completed` | historical `low` filename | <https://github.com/rhasspy/piper/issues/34> |

**`OHF-Voice/piper1-gpl` — Chinese front-end and prosody complaints**

| # | Title | State | Notes (quoted) | URL |
| --- | --- | --- | --- | --- |
| 128 | Some prosody-relevant punctuation chars are dropped by espeakbridge | **open** (2025-11-26, 0 comments) | *"…and **all Chinese punctuation is dropped, so Chinese paragraphs just become a single run-on sentence**."* Test asserts `("cmn", "你好，世界！省略号……一、二，三。", [",","!",";",",",",","."])`; the reporter supplies a proposed `espeakbridge.c` fix. | <https://github.com/OHF-Voice/piper1-gpl/issues/128> |
| 29 | how to solve the error pause for Chinese | **open** (2025-07-31, 3 comments) | user trained a huayan-medium model on 12 h of data; pause placement differs from reference | <https://github.com/OHF-Voice/piper1-gpl/issues/29> |
| 134 | Submission Process for a New zh_CN TTS Model | **open** (2025-12-03, 16 comments) | community retrain from `zh/zh_CN/huayan/medium`; *"The current model still needs improvement in sentence-level transitions."* | <https://github.com/OHF-Voice/piper1-gpl/issues/134> |
| 205 (PR) | Use pypinyin as a fallback for g2pw | **open** (assigned to maintainer `synesthesiam`) | *"g2pw transitively depends on pytorch, which is not available on all target environments… **I asked a native speaker to compare the results of both, and they claim it the same, including intonation**."* | <https://github.com/OHF-Voice/piper1-gpl/pull/205> |
| 245 (PR) | Add `--extra-models-dir` flag for extra downloadable models | **open** (2026-07-23) | *"**Two of the Chinese voices require the g2pW model**, which Piper currently looks up in the current working directory and downloads there if missing… There is still a separate HTTP request to Hugging Face for the `bert-base-chinese` tokenizer… This is how the `g2pW` module resolves its tokenizer"* | <https://github.com/OHF-Voice/piper1-gpl/pull/245> |
| 153 (PR) | Add Chinese phonemization using g2pW | **closed, not merged** (`merged_at: null`) | superseded | <https://github.com/OHF-Voice/piper1-gpl/pull/153> |
| 163 (PR) | Add Chinese phonemization and phoneme ids in training | **closed, merged** 2026-01-20 | shipped the path | <https://github.com/OHF-Voice/piper1-gpl/pull/163> |
| 271 (PR) | Add Chinese pinyin support – Phase 1 (monophonic dict fallback, honest scoping) | **merged 2026-08-22** | adds the non-espeak Chinese pinyin path; *"Polyphonic chars (`重/行/长/好` etc.) treated as unsupported → empty, preventing silent mis-assignment of `重庆/银行/长江`."*; defers Phase 2 to #272 | <https://github.com/OHF-Voice/piper1-gpl/pull/271> |
| 289 (PR) | fix: Phase 1 relaxed poly fallback for mobile | **merged 2026-08-29** | *"Phase 1 strict mono returned empty for any poly char (虹/称/绛/简/重/行/长), causing long sentences like 彩虹，又称天弓… to be empty -> **PIPER_ERR_GENERIC -> silent** in piper-app iOS/macOS"*; *"95% coverage, tradeoff: 重庆 may pick first sense until Phase 2, acceptable vs silence"* | <https://github.com/OHF-Voice/piper1-gpl/pull/289> |
| 269 (PR) | Run g2pW under onnxruntime without torch | **merged 2026-08-13** | *"output was compared against upstream `g2pw.G2PWConverter` on the same `g2pw.onnx`: identical results across 19 sentences"* | <https://github.com/OHF-Voice/piper1-gpl/pull/269> |
| 272 (PR) | Phase 2 — full contextual polyphone disambiguation (g2pw BERT ONNX) | **reference only — not shipped**; title/state not retrieved (rate-limited) | referenced verbatim from #271: *"Full contextual polyphone disambiguation … deferred to #272."* | <https://github.com/OHF-Voice/piper1-gpl/pull/272> |
| 158 | Integrate goruut/pygoruut phonemizer | **open** (2026-01-02, 3 comments) | includes a phonemize→dephonemize success table with a **`chinese/mandarin | zh`** row | <https://github.com/OHF-Voice/piper1-gpl/issues/158> |

**Offline-relevance note on issue #245:** it documents that the `pinyin` path performs network fetches at runtime — a `g2pW` model download *and* a separate `bert-base-chinese` tokenizer HTTP request — and that the repository's proposed mitigation (`--extra-models-dir`) was still an **open, unmerged PR** as of 2026-07-23 — <https://github.com/OHF-Voice/piper1-gpl/pull/245>. Fact only; no recommendation.

### B4. Published comparisons of eSpeak-NG Mandarin quality vs pinyin-dictionary front-ends

- **eSpeak-NG itself publishes no Mandarin quality metric and no comparison to pinyin front-ends.** No eSpeak-NG document compares its Mandarin output to a pinyin-dictionary front-end, and **no MOS, accuracy or error-rate figure for eSpeak-NG Mandarin is published** anywhere in its docs (`docs/languages.md` has no Mandarin quality text — see B1). Its changelog could not be retrieved during this pass (`ChangeLog.md` download failed), so a changelog statement about Mandarin is **not determinable from public sources**.
- **A published coverage figure exists for the mono-reading (first-reading) fallback.** Piper PR #289, merged 2026-08-29, states: *"Change phonemize() to use first reading for poly chars instead of failing … **95% coverage**, tradeoff: 重庆 may pick first sense until Phase 2, acceptable vs silence"*, and documents the failure it replaced: *"Phase 1 strict mono returned empty for any poly char (虹/称/绛/简/重/行/长), causing long sentences like 彩虹，又称天弓… to be empty -> PIPER_ERR_GENERIC -> silent in piper-app iOS/macOS."* — <https://github.com/OHF-Voice/piper1-gpl/pull/289>. This is a coverage claim for the single-reading dictionary path, explicitly framed as a 多音字 tradeoff, not a quality benchmark.
- **One published implementation-equivalence check exists for the pinyin front-end:** Piper PR #269, merged 2026-08-13, *"Run g2pW under onnxruntime without torch"*, states: *"Separately, output was compared against upstream `g2pw.G2PWConverter` on the same `g2pw.onnx`: identical results across 19 sentences (polyphone disambiguation, mixed ASCII/digits, …)."* — <https://github.com/OHF-Voice/piper1-gpl/pull/269>. This checks that the ONNX port matches the reference g2pW implementation; it says nothing about whether either pronunciation is correct.
- **No equivalent check exists for the eSpeak-NG `cmn` path** — no published comparison of eSpeak-NG Mandarin output against any pinyin-dictionary front-end was found in any first-party source.
- **The most direct published comparison is a native-speaker A/B of g2pW vs pypinyin**, reported in a PR body: *"Use pypinyin as a fallback when g2pw is not available… My handling of Chinese is very basic, but **I asked a native speaker to compare the results of both, and they claim it the same, including intonation**."* — <https://github.com/OHF-Voice/piper1-gpl/pull/205>. Note this compares two *pinyin-dictionary* front-ends with each other, not either against eSpeak-NG, and it is a single untrained listener impression, not a benchmark.
- **A quantitative phonemize/dephonemize round-trip table is published in the Piper tracker**, with a Mandarin row. From the same body: *"Testing results (phonemize then dephonemize test)"* — `chinese/mandarin | zh | 9% | 83% | 8% | 83%` (columns: `word success rate`, `char success rate`, `word success rate (nostress)`, `char success rate (nostress)`), against e.g. `english/american | 84% | 92% | 31% | 78%` — <https://github.com/OHF-Voice/piper1-gpl/issues/158>. **Caveat: this table measures the third-party `pygoruut` phonemizer's own round-trip fidelity, not eSpeak-NG's, and it is a contributor-supplied measurement with no stated corpus or methodology** — the reporter proposes it as an argument for adopting pygoruut. It is the only published numeric quality figure for Mandarin found in any Piper or espeak-ng tracker.
- **A qualitative comparison of espeak-ng's Mandarin dictionary against a segmentation-based approach** is made by the Piper contributor who filed #164: eSpeak-NG's *"Chinese dict … is far too small and rigid"*, and they are moving to *"some other word segmentation module like jieba"* — <https://github.com/rhasspy/piper/issues/164>. This is a practitioner statement in an issue report, not a benchmark, and the issue was closed **`not_planned`**.
- **Comparison of eSpeak-NG `cmn` against a pinyin front-end by the Piper maintainers in a changelog or docs: not determinable from public sources.** The changelog documents *why* a non-espeak front-end was added for Japanese and Thai (both with explicit quality rationale, quoted in B1) but for Chinese it states only *"Add Chinese phonemizer based on g2pW"* with no comparative quality claim — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md>.
- The `phoneme_type` split visible in the configs (A4) is itself the project's structural statement: **`huayan` is the legacy eSpeak-NG `cmn` model, while every Chinese voice built afterwards (`xiao_ya`, `chaowen`) uses `phoneme_type: pinyin` with g2pW** — <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/medium/zh_CN-huayan-medium.onnx.json>, <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/xiao_ya/medium/zh_CN-xiao_ya-medium.onnx.json>, <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/chaowen/medium/zh_CN-chaowen-medium.onnx.json>.

---

## Topic C — Was Piper's Chinese reported as low quality anywhere credible?

Yes, in multiple independent, citable places. Every item below is a first-party artifact (issue, PR, changelog, model card), not a blog.

1. **Tone accuracy of `zh_CN-huayan-medium` is reported as wrong by a phonetician-style analysis.** In `rhasspy/piper` **#305** (open, 👍2), the reporter gives the eSpeak-ng tone numbering (`1(55), 2(35), 3(214), 4(51), 5(11)`) and then states of Piper's huayan medium voice: *"tone 1 sometimes sounds like tone 2, and tone 4 sometimes sounds like tone 1. And the total sentence sounds strange."* The reporter hypothesises the model was trained on espeak-ng IPA that was itself mis-generated, and asks the maintainer to *"please retrain the model after fixed the espeak-ng"*. The same issue documents a concrete espeak-ng IPA bug (`213`→`2`, `51`→`5`) with a proposed `dictionary.c` patch. — <https://github.com/rhasspy/piper/issues/305>
2. **Accent and prosody are reported as unnatural.** `rhasspy/piper` **#278**, "More natural Chinese voice, Please": *"English is very natural, but Chinese has an English accent and seems unnatural. The segmentation of sentence pauses feels a bit mechanical."* Also reports that mixed Chinese/English text is unsupported. — <https://github.com/rhasspy/piper/issues/278>
3. **A concrete wrong-pronunciation example on the official medium voice.** `rhasspy/piper` **#243** reproduces an incorrect rendering of `一点儿` with `zh_CN-huayan-medium.onnx`, attaching both the generated audio and a native reference recording. — <https://github.com/rhasspy/piper/issues/243>
4. **An independent user's reason for retraining from scratch was incorrect tone.** `rhasspy/piper` **#652** (closed `not_planned`): *"I am training a new Chinese model for Piper because the pronunciation tone of the Piper project's Chinese model is incorrect."* — <https://github.com/rhasspy/piper/issues/652>
5. **The root cause was acknowledged as a front-end limitation and the issue proposing a fix was closed `not_planned`.** `rhasspy/piper` **#164**: espeak-ng's Chinese dictionary is *"far too small and rigid"* — closed `not_planned` on 2024-09-14. — <https://github.com/rhasspy/piper/issues/164>
6. **Chinese punctuation was being dropped entirely downstream of the legacy path**, making paragraphs a single run-on sentence: `OHF-Voice/piper1-gpl` **#128** (open, 0 comments) — <https://github.com/OHF-Voice/piper1-gpl/issues/128>
7. **Sentence-level transitions in a community retrain of huayan are still described as needing work** by the author submitting them: `OHF-Voice/piper1-gpl` **#134** (open, 16 comments) — <https://github.com/OHF-Voice/piper1-gpl/issues/134>
8. **The project's own licensing/support text implicitly distances itself from voice quality**: `docs/VOICES.md` says *"Piper is intended for personal use and text to speech research only"* and each Chinese model card is thin (the `huayan` cards state the dataset licence is *"Unknown"*) — <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/VOICES.md>, <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/medium/MODEL_CARD>
9. **Counter-evidence to note for balance (facts only):** the same project added a dedicated g2pW Chinese front-end in 1.4.0 and ported it to ONNX-only in 1.6.1 specifically to keep it usable without PyTorch (<https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md>); two newer Chinese voices (`xiao_ya`, `chaowen`) exist with model cards dated to that newer path (<https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/xiao_ya/medium/MODEL_CARD>); and a contributor reported a native speaker finding g2pW and pypinyin output equivalent *"including intonation"* (<https://github.com/OHF-Voice/piper1-gpl/pull/205>) — note that comparison concerns the pinyin front-ends, not the eSpeak-NG `cmn` voice.

**Reports of low Chinese quality from a peer-reviewed or independent-benchmark source: not determinable from public sources.** No academic evaluation of Piper's Mandarin voices (e.g. MOS) was found, and no published MOS/RTF figure exists for any `zh_CN` Piper voice. Everything above is issue-tracker and model-card evidence.

---

## Consolidated facts most likely to matter (no recommendation implied)

| Fact | Value | Source |
| --- | --- | --- |
| Piper code licence (current) | **GPL-3.0** | <https://api.github.com/repos/OHF-Voice/piper1-gpl> |
| Piper code licence (legacy, archived) | MIT | <https://api.github.com/repos/rhasspy/piper> |
| Latest Piper release | **v1.8.0**, 2026-09-04 | <https://github.com/OHF-Voice/piper1-gpl/releases/tag/v1.8.0> |
| Mandarin voices published | **4** (`huayan-medium`, `huayan-x_low`, `chaowen-medium`, `xiao_ya-medium`) | <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN> |
| Chinese front-end split (**key**) | only `huayan` (both tiers) uses eSpeak-NG `cmn`; `xiao_ya` + `chaowen` use `phoneme_type: "pinyin"` + g2pW (runtime download) | <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/medium/zh_CN-huayan-medium.onnx.json>, <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/xiao_ya/medium/zh_CN-xiao_ya-medium.onnx.json> |
| g2pW path runtime network access | downloads `g2pw.tar.gz` **and** issues an HTTP request for the `bert-base-chinese` tokenizer | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/src/piper/phonemize_chinese.py>, <https://github.com/OHF-Voice/piper1-gpl/pull/245> |
| Smallest Mandarin voice | `huayan-x_low`, **20,628,813 B**, 16 kHz | <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/x_low> |
| Smallest Mandarin voice with eSpeak-NG `cmn` front-end | `huayan-x_low` (both huayan voices are `cmn`) | <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/x_low/zh_CN-huayan-x_low.onnx.json> |
| Native ONNX runtime dependency | onnxruntime **1.22.0** (default in CMake) | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt> |
| `libpiper` artifact type | **SHARED library**, C ABI | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h> |
| `espeak-ng-data` required at runtime | **yes** (`piper_create` needs the path) | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md> |
| Compiled Mandarin dictionary size | **1,566,335 B** (`cmn_dict`) | <https://huggingface.co/api/models/csukuangfj/vits-piper-zh_CN-huayan-medium/tree/main/espeak-ng-data> |
| `cmn` data build flag | `-DEXTRA_cmn:BOOL=ON` (not default) | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt> |
| Official Android artifact | **none** (no Android branch in CMake; no Android CI job; release assets are wheels/sdist only) | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt>, <https://api.github.com/repos/OHF-Voice/piper1-gpl/releases> |
| Legacy `libpiper` shared library | **did not exist** (executables only, no `add_library`) | <https://raw.githubusercontent.com/rhasspy/piper/master/CMakeLists.txt> |
| Current CLI target name | `piper_exe` (separate from the `piper` library target) | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/CMakeLists.txt> |
| Audio sample format | **32-bit float** in current C API; legacy was `int16_t` | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h> |
| Timing fields in current C API | **none** (legacy had `SynthesisResult::realTimeFactor`) | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h> |
| ONNX Runtime Android AAR (Microsoft, official) | `onnxruntime-android-1.22.0.aar` = **28,515,295 B** (matches libpiper's pinned 1.22.0) | <https://repo1.maven.org/maven2/com/microsoft/onnxruntime/onnxruntime-android/1.22.0/> |
| Prebuilt arm64-v8a Mandarin-Piper APKs (third-party engine) | sherpa-onnx `zh_CN-huayan-medium`, `zh_CN-chaowen-medium`, `zh_CN-xiao_ya-medium` | <https://k2-fsa.github.io/sherpa/onnx/tts/apk.html>, <https://huggingface.co/csukuangfj2/sherpa-onnx-apk/resolve/main/tts-new/1.13.7/> |
| Termux Piper package | **does not exist** (`packages/piper/build.sh` HTTP 404) | <https://raw.githubusercontent.com/termux/termux-packages/master/packages/espeak/build.sh> |
| Piper-published `espeak-ng-data` size | **not published** (Debian reference for the same data set: 8,450.7 kB package / 24,034.0 kB installed) | <https://packages.debian.org/stable/sound/espeak-ng-data> |
| Quality tier definitions | `x_low` 16 kHz 5–7 M params; `low` 16 kHz 15–20 M; `medium` 22.05 kHz 15–20 M; `high` 22.05 kHz 28–32 M | <https://rhasspy.github.io/piper-samples/> |
| First-party RTF for Piper | **not published** | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/README.md> |
| Third-party RTF on named hardware | 0.774 (1 thread) for `vits-piper-en_US-lessac-medium` on Raspberry Pi 4 Model B Rev 1.5 | <https://raw.githubusercontent.com/k2-fsa/sherpa/master/docs/source/onnx/tts/pretrained_models/rtf.rst> |
| RTF for a `zh_CN` Piper voice | **not published** | — |
| eSpeak-NG Mandarin status | listed (`cmn`, Sino-Tibetan, Mandarin), **no quality caveat in docs** | <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages.md> |
| eSpeak-NG Mandarin tone sandhi | **works** | <https://github.com/espeak-ng/espeak-ng/issues/364> |
| eSpeak-NG Chinese polyphone disambiguation | multi-word entries in `dictsource/extra/cmn_listx` (21,611 single chars + **36,500 compound exceptions**); compiled by default (`EXTRA_cmn` ON) | <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/extra/cmn_listx> |
| eSpeak-NG Chinese multi-word *matching* bug | **still open** — `dictionary_skipwords`=1 instead of 2 | <https://github.com/espeak-ng/espeak-ng/issues/606> |
| eSpeak-NG Mandarin source dict sizes | `cmn_list` **36,237 B**; `cmn_rules` **3,997 B**; `cmn_emoji` **91,353 B** | <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_list> |
| eSpeak-NG Mandarin dictionary coverage | "3799 most common characters" (single-reading); `EXTRA_cmn` adds compound exceptions | <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_list> |
| eSpeak-NG word segmentation | **none** (no jieba/pypinyin); segmentation is by dictionary multi-word entries | <https://github.com/espeak-ng/espeak-ng/pull/2455> |
| eSpeak-NG Mandarin tone mapping | `1->55 2->35 3->214 4->51 5->11` in `cmn_rules` | <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_rules> |
| Open Mandarin tone-sandhi defect | 好旅馆 → `X'Au35` instead of `X'Au214` | <https://github.com/espeak-ng/espeak-ng/issues/815> |
| Piper C++ polyphone handling | **first reading** used for polyphonic chars; PR #289 claims *"95% coverage, tradeoff: 重庆 may pick first sense until Phase 2"* | <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/chinese_phonemizer.cpp>, <https://github.com/OHF-Voice/piper1-gpl/pull/289> |
| Piper C++ Chinese **silent-failure precedent** | strict Phase 1 returned empty for any poly char → `PIPER_ERR_GENERIC` → **silent** in piper-app iOS/macOS; relaxed to first-reading 2026-08-29 | <https://github.com/OHF-Voice/piper1-gpl/pull/289> |
| Piper contextual polyphone disambiguation (Phase 2) | **not shipped** — deferred to PR #272 | <https://github.com/OHF-Voice/piper1-gpl/pull/272> |
| Strongest quality complaint in eSpeak-NG's own tracker | *"#1028 makes ESpeak completely unusable for Chinese users."* | <https://github.com/espeak-ng/espeak-ng/issues/1163> |

---

## All URLs used

**Piper repositories, APIs and docs**

- <https://github.com/rhasspy/piper>
- <https://api.github.com/repos/rhasspy/piper>
- <https://raw.githubusercontent.com/rhasspy/piper/master/README.md>
- <https://github.com/rhasspy/piper/releases.atom>
- <https://raw.githubusercontent.com/rhasspy/piper/v1.2.0/README.md>
- <https://raw.githubusercontent.com/rhasspy/piper/v1.0.0/README.md>
- <https://github.com/rhasspy/piper/issues/305>
- <https://github.com/rhasspy/piper/issues/278>
- <https://github.com/rhasspy/piper/issues/243>
- <https://github.com/rhasspy/piper/issues/164>
- <https://github.com/rhasspy/piper/issues/34>
- <https://github.com/rhasspy/piper/issues/505>
- <https://github.com/rhasspy/piper/issues/652>
- <https://github.com/rhasspy/piper/issues/835>
- <https://github.com/rhasspy/piper/issues/613>
- <https://github.com/rhasspy/piper/issues/740>
- <https://api.github.com/search/issues?q=repo%3Arhasspy%2Fpiper+chinese&per_page=30>
- <https://github.com/rhasspy/piper/discussions/608>
- <https://api.github.com/repos/OHF-Voice/piper1-gpl>
- <https://api.github.com/repos/OHF-Voice/piper1-gpl/releases>
- <https://github.com/OHF-Voice/piper1-gpl/releases/tag/v1.8.0>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/README.md>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CHANGELOG.md>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/VOICES.md>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/docs/BUILDING.md>
- <https://api.github.com/repos/OHF-Voice/piper1-gpl/contents/src/piper>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/src/piper/phonemize_chinese.py>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/src/piper/phonemize_espeak.py>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/README.md>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper.h>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/CMakeLists.txt>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/chinese_phonemizer.cpp>
- <https://api.github.com/search/issues?q=repo%3AOHF-Voice%2Fpiper1-gpl+chinese&per_page=30>
- <https://github.com/OHF-Voice/piper1-gpl/issues/128>
- <https://github.com/OHF-Voice/piper1-gpl/issues/29>
- <https://github.com/OHF-Voice/piper1-gpl/issues/134>
- <https://github.com/OHF-Voice/piper1-gpl/issues/158>
- <https://github.com/OHF-Voice/piper1-gpl/pull/205>
- <https://github.com/OHF-Voice/piper1-gpl/pull/245>
- <https://github.com/OHF-Voice/piper1-gpl/pull/153>
- <https://github.com/OHF-Voice/piper1-gpl/pull/163>
- <https://newreleases.io/project/github/OHF-Voice/piper1-gpl/release/v1.4.0>

**Piper voices (Hugging Face — canonical URLs; read via the hf-mirror.com mirror of identical paths)**

- <https://huggingface.co/rhasspy/piper-voices>
- <https://huggingface.co/api/models/rhasspy/piper-voices>
- <https://huggingface.co/rhasspy/piper-voices/resolve/main/voices.json>
- <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh>
- <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN>
- <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan>
- <https://huggingface.co/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/medium>
- <https://huggingface.co/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/x_low>
- <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/chaowen>
- <https://huggingface.co/rhasspy/piper-voices/tree/main/zh/zh_CN/chaowen/medium>
- <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/xiao_ya>
- <https://huggingface.co/rhasspy/piper-voices/tree/main/zh/zh_CN/xiao_ya/medium>
- <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/medium/zh_CN-huayan-medium.onnx.json>
- <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/x_low/zh_CN-huayan-x_low.onnx.json>
- <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/chaowen/medium/zh_CN-chaowen-medium.onnx.json>
- <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/xiao_ya/medium/zh_CN-xiao_ya-medium.onnx.json>
- <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/medium/MODEL_CARD>
- <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/huayan/x_low/MODEL_CARD>
- <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/chaowen/medium/MODEL_CARD>
- <https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/xiao_ya/medium/MODEL_CARD>
- <https://hf-mirror.com/api/datasets/rhasspy/piper-voices> (HTTP 401 — dataset listing unavailable)
- <https://api.github.com/repos/rhasspy/piper-voices/blame/5e74c24a88ed7d31e308633fa1542433ce2b28d4/voices.json> (surfaced via search: shows `zh_CN-huayan-x_low` as a `voices.json` key)

**Third-party quantised Chinese voices and bundled espeak-ng-data**

- <https://huggingface.co/api/models?search=piper-zh&limit=30>
- <https://huggingface.co/api/models/csukuangfj2/vits-piper-zh_CN-xiao_ya-medium/tree/main>
- <https://huggingface.co/api/models/csukuangfj2/vits-piper-zh_CN-xiao_ya-medium-int8/tree/main>
- <https://huggingface.co/api/models/csukuangfj2/vits-piper-zh_CN-xiao_ya-medium-fp16/tree/main>
- <https://huggingface.co/api/models/csukuangfj2/vits-piper-zh_CN-chaowen-medium/tree/main>
- <https://huggingface.co/api/models/csukuangfj2/vits-piper-zh_CN-chaowen-medium-int8/tree/main>
- <https://huggingface.co/api/models/csukuangfj/vits-piper-zh_CN-huayan-medium/tree/main>
- <https://huggingface.co/api/models/csukuangfj/vits-piper-zh_CN-huayan-medium/tree/main/espeak-ng-data>
- <https://huggingface.co/api/models/csukuangfj/vits-piper-zh_CN-huayan-x_low/tree/main>

**eSpeak-NG**

- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages.md>
- <https://api.github.com/search/issues?q=repo%3Aespeak-ng%2Fespeak-ng+chinese+in%3Atitle&per_page=40>
- <https://github.com/espeak-ng/espeak-ng/issues/257>
- <https://github.com/espeak-ng/espeak-ng/issues/606>
- <https://github.com/espeak-ng/espeak-ng/issues/685>
- <https://github.com/espeak-ng/espeak-ng/issues/347>
- <https://github.com/espeak-ng/espeak-ng/issues/1669>
- <https://github.com/espeak-ng/espeak-ng/issues/338>
- <https://github.com/espeak-ng/espeak-ng/issues/346>
- <https://github.com/espeak-ng/espeak-ng/issues/348>
- <https://github.com/espeak-ng/espeak-ng/issues/499>
- <https://github.com/espeak-ng/espeak-ng/issues/364>
- <https://github.com/espeak-ng/espeak-ng/issues/1851>
- <https://github.com/espeak-ng/espeak-ng/issues/2151>
- <https://github.com/espeak-ng/espeak-ng/issues/757>
- <https://github.com/espeak-ng/espeak-ng/pull/738>
- <https://github.com/espeak-ng/espeak-ng/pull/2511>
- <https://github.com/espeak-ng/espeak-ng/pull/340>
- <https://github.com/espeak-ng/espeak-ng/issues/1275> (search-surfaced: "Conversion from text to IPA phonemes seems incorrect when phonemes include tone changes")

**eSpeak-NG dictionary source files, docs and Mandarin-specific PRs/issues**

- <https://api.github.com/repos/espeak-ng/espeak-ng>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/index.md>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages/sit/cmn.md> (HTTP 404 — no per-language page)
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/lang/sit/cmn>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/cmn_dict> (HTTP 404 — generated at build time)
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/cmake/data.cmake>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_list>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_rules>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_emoji>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/extra/cmn_listx>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/zh_listx> (HTTP 404 — renamed)
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/zh_pinyin> (HTTP 404 — renamed)
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/zh_rules> (HTTP 404 — renamed)
- <https://github.com/espeak-ng/espeak-ng/issues/815>
- <https://github.com/espeak-ng/espeak-ng/issues/1028>
- <https://github.com/espeak-ng/espeak-ng/issues/1805>
- <https://github.com/espeak-ng/espeak-ng/pull/940>
- <https://github.com/espeak-ng/espeak-ng/pull/2455>
- <https://github.com/OHF-Voice/piper1-gpl/pull/271>
- <https://github.com/OHF-Voice/piper1-gpl/pull/289>
- <https://github.com/OHF-Voice/piper1-gpl/pull/269>
- <https://github.com/OHF-Voice/piper1-gpl/pull/272>
- <https://github.com/espeak-ng/espeak-ng/issues/361>
- <https://github.com/espeak-ng/espeak-ng/issues/664>
- <https://github.com/espeak-ng/espeak-ng/issues/933>
- <https://github.com/espeak-ng/espeak-ng/issues/1044>
- <https://github.com/espeak-ng/espeak-ng/issues/1163>
- <https://github.com/espeak-ng/espeak-ng/issues/1236>
- <https://github.com/espeak-ng/espeak-ng/issues/1370>
- <https://github.com/espeak-ng/espeak-ng/issues/1404>
- <https://github.com/espeak-ng/espeak-ng/issues/1665>
- <https://github.com/espeak-ng/espeak-ng/issues/1670>
- <https://github.com/espeak-ng/espeak-ng/issues/1793>
- <https://github.com/espeak-ng/espeak-ng/issues/1831>
- <https://github.com/espeak-ng/espeak-ng/issues/2160>
- <https://github.com/espeak-ng/espeak-ng/pull/788>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/CHANGELOG.md> (HTTP 404 — the file is `ChangeLog.md`, which could not be downloaded)
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+mandarin>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+cmn>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+pinyin>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+tone+in:title>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+%E5%A4%9A%E9%9F%B3%E5%AD%97>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+polyphone>
- <https://api.github.com/search/issues?q=repo:OHF-Voice/piper1-gpl+g2pw&per_page=20>
- <https://api.github.com/search/issues?q=repo:OHF-Voice/piper1-gpl+polyphone&per_page=20>

**Speed / RTF and adjacent projects**

- <https://raw.githubusercontent.com/k2-fsa/sherpa/master/docs/source/onnx/tts/pretrained_models/rtf.rst>
- <https://github.com/k2-fsa/sherpa-onnx/issues/2938>
- <https://github.com/rhasspy/piper/issues/545> (search-surfaced: "Failed to set eSpeak-ng voice")
- <https://github.com/GitYCC/g2pW/>
- <https://github.com/jvoice-project/piper-jni>

**Additional sources for the legacy build system, the two C++ APIs, Android/NDK state, and packaging**

- <https://raw.githubusercontent.com/rhasspy/piper/master/CMakeLists.txt>
- <https://raw.githubusercontent.com/rhasspy/piper/master/Makefile>
- <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/CMakeLists.txt> (HTTP 404 — file does not exist)
- <https://api.github.com/repos/rhasspy/piper/contents/src/cpp>
- <https://api.github.com/repos/rhasspy/piper/contents/>
- <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.hpp>
- <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/piper.cpp>
- <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/main.cpp>
- <https://raw.githubusercontent.com/rhasspy/piper/master/src/cpp/test.cpp>
- <https://api.github.com/repos/rhasspy/piper/releases>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/CMakeLists.txt>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.gitignore>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/include/piper_impl.hpp>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/piper.cpp>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/CMakeLists.txt>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/main.cpp>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/libpiper/src/main/utils/main_utils.cpp>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.github/workflows/build-libpiper.yml>
- <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/.github/workflows/wheels.yml>
- <https://api.github.com/repos/OHF-Voice/piper1-gpl/git/trees/main?recursive=1>
- <https://api.github.com/repos/OHF-Voice/piper1-gpl/releases/tags/v1.5.0>
- <https://api.github.com/search/issues?q=repo:rhasspy/piper+android&per_page=50>
- <https://api.github.com/search/issues?q=repo:OHF-Voice/piper1-gpl+android&per_page=50>
- <https://github.com/rhasspy/piper/issues/103>
- <https://github.com/rhasspy/piper/issues/180>
- <https://github.com/rhasspy/piper/issues/251>
- <https://github.com/rhasspy/piper/issues/370>
- <https://github.com/OHF-Voice/piper1-gpl/issues/7>
- <https://github.com/OHF-Voice/piper1-gpl/issues/78>
- <https://github.com/OHF-Voice/piper1-gpl/issues/130>
- <https://github.com/OHF-Voice/piper1-gpl/pull/21>
- <https://github.com/OHF-Voice/piper1-gpl/issues/224>
- <https://rhasspy.github.io/piper-samples/>
- <https://k2-fsa.github.io/sherpa/onnx/tts/apk.html>
- <https://huggingface.co/csukuangfj2/sherpa-onnx-apk/resolve/main/tts-new/1.13.7/>
- <https://github.com/k2-fsa/sherpa-onnx/tree/master/android/SherpaOnnxTts>
- <https://raw.githubusercontent.com/nihui/ncnn-android-piper/master/README.md>
- <https://raw.githubusercontent.com/gyroing/piper-tts-for-termux/main/README.md>
- <https://raw.githubusercontent.com/jvoice-project/piper-jni/main/README.md>
- <https://raw.githubusercontent.com/termux/termux-packages/master/packages/piper/build.sh> (HTTP 404)
- <https://raw.githubusercontent.com/termux/termux-packages/master/packages/piper-tts/build.sh> (HTTP 404)
- <https://raw.githubusercontent.com/termux/termux-packages/master/packages/espeak/build.sh>
- <https://repo1.maven.org/maven2/com/microsoft/onnxruntime/onnxruntime-android/1.22.0/>
- <https://packages.debian.org/stable/sound/espeak-ng-data>
- <https://packages.debian.org/trixie/amd64/espeak-ng-data/filelist>
- <https://huggingface.co/api/models/rhasspy/piper-voices/tree/main/zh/zh_CN/huayan/low> (does not exist)

**Rate-limited or failed fetches (blanks, not negatives)**

- <https://api.github.com/search/issues?q=repo:rhasspy/piper+termux&per_page=50> — HTTP 403 rate limit
- <https://api.github.com/repos/rhasspy/piper-phonemize/releases> — HTTP 403 rate limit
- <https://api.github.com/repos/OHF-Voice/piper1-gpl/issues/130/comments> — HTTP 403 rate limit
- <https://api.github.com/repos/espeak-ng/espeak-ng/contents/dictsource> — HTTP 403 rate limit (so exact `dictsource/cmn_list` byte sizes are not reported; the compiled `cmn_dict` size is given instead)
- <https://raw.githubusercontent.com/rhasspy/piper-samples/master/README.md> — transport error
- `https://huggingface.co/datasets/rhasspy/piper-voices` — HTTP 401 via mirror
