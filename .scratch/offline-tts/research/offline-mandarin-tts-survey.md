# Offline Mandarin (Chinese) TTS engines — primary-source survey

**Target under evaluation:** Qt 6.11.1 / C++17 / CMake Android app, arm64-v8a, Android 9+ (API 28), zero Java/JNI code, no Gradle files. Requirements under consideration: offline Mandarin TTS, accurate pronunciation, small model, low latency. `whisper.cpp` + `ggml` are already vendored.

**Method / caveats**

- Every claim below carries an inline URL to the source that owns it.
- `not published` means no primary source publishes that number.
- Direct `https://huggingface.co/...` fetches failed from this environment (transport error, not 404). Hugging Face model cards and file listings were therefore read through the `hf-mirror.com` mirror of the identical paths (same repo, same files) and are cited using the canonical `https://huggingface.co/...` URL. File sizes come from the Hugging Face repository tree API, which reports exact byte counts.
- GitHub HTML pages return mostly navigation boilerplate when fetched; `.md` files were read raw from `raw.githubusercontent.com` and repo/file metadata from the GitHub REST API.
- Architecture classification (autoregressive vs. feed-forward) is reported from the paper abstract or the upstream model card, quoted where it matters.

---

## 1. MeloTTS (myshell-ai/MeloTTS) — Chinese

| Item | Finding |
| --- | --- |
| Runtime dependency | PyTorch + HuggingFace `transformers`. No ONNX/ggml runtime in the upstream repo. |
| Prebuilt Android arm64 artifact | None official. Runnable Android arm64 path exists only via sherpa-onnx's ONNX conversion. |
| Architecture | Non-autoregressive / feed-forward (VITS-derived). |
| Code licence | MIT |
| Weights licence | MIT |

**Repo facts**

- Repo: <https://github.com/myshell-ai/MeloTTS> — `pushed_at` 2024-12-24T19:17:13Z, 7,633 stars, 232 open issues, language Python (<https://api.github.com/repos/myshell-ai/MeloTTS>).
- Root contains `LICENSE`, `melo/`, `docs/`, `requirements.txt`, `setup.py`, `test/`, `Dockerfile` — **no ONNX export script and no Android/Gradle/CMake directory** (<https://api.github.com/repos/myshell-ai/MeloTTS/contents/>).
- Development environment stated as Ubuntu 20.04 + Python 3.9, installed via `pip install -e .` then `python -m unidic download` — i.e. a Python install step is part of the documented workflow (<https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/docs/install.md>).

**Architecture (non-autoregressive)**

- Model is VITS-family; README acknowledgements: "This implementation is based on TTS, VITS, VITS2 and Bert-VITS2" (<https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/README.md>).
- VITS is explicitly parallel/feed-forward: "we present a parallel end-to-end TTS method"; the abstract frames prior work as "single-stage training and parallel sampling" (<https://arxiv.org/abs/2106.06103>).
- Config confirms a VITS-style synthesizer: `"n_layers_trans_flow": 3`, `"upsample_rates": [8,8,2,2,2]`, `"sampling_rate": 44100`, `"add_blank": true` (<https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/melo/configs/config.json>).

**Chinese model names, published sizes, quantisation**

- Official Chinese checkpoint: `myshell-ai/MeloTTS-Chinese` → `checkpoint.pth` = **207,770,124 bytes** (≈198 MiB); `config.json` = 2,296 bytes; `README.md` = 4,445 bytes. The repo contains **no fp16/int8 quantised variants** (<https://huggingface.co/api/models/myshell-ai/MeloTTS-Chinese/tree/main>).
- Published model-card licence metadata for that repo is `license: mit` (<https://huggingface.co/myshell-ai/MeloTTS-Chinese>).
- sherpa-onnx conversion `vits-melo-tts-zh_en` (Chinese + English, 1 speaker), published file listing: `model.onnx` **163 MB**, `lexicon.txt` 6.5 MB, `date.fst` 58K, `number.fst` 63K, `phone.fst` 87K, `tokens.txt` 655 B, plus a `dict/` directory (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html>).
- The download is `vits-melo-tts-zh_en.tar.bz2` from the `tts-models` release tag (<https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-melo-tts-zh_en.tar.bz2>).
- **Quantisation variants of MeloTTS-Chinese: not published** (neither upstream nor the sherpa-onnx conversion ships fp16/int8).

**Chinese text front-end — pypinyin + jieba + cn2an + ToneSandhi + BERT**

Source: <https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/melo/text/chinese.py>

- `import cn2an` — number → Chinese text before G2P.
- `from pypinyin import lazy_pinyin, Style` — `Style.INITIALS` and `Style.FINALS_TONE3`, with `neutral_tone_with_five=True`.
- `import jieba.posseg as psg` — jieba word segmentation **with POS tags**; POS tags drive tone-sandhi behaviour.
- `from .tone_sandhi import ToneSandhi` — a built-in `ToneSandhi` class (`tone_modifier.pre_merge_for_modify(...)`, `tone_modifier.modified_tone(...)`), i.e. a custom third-tone sandhi module, not an external library.
- Pinyin → phoneme symbol map is loaded from a repo data file: `opencpop-strict.txt` (`pinyin_to_symbol_map`).
- English words inside Chinese text are stripped by regex before G2P in this path (`re.sub("[a-zA-Z]+", "", seg)`).
- **BERT is required at inference.** `get_bert_feature` calls `from text import chinese_bert`; `melo/text/chinese_bert.py` hard-codes `model_id='hfl/chinese-roberta-wwm-ext-large'` loaded via `AutoModelForMaskedLM.from_pretrained(model_id)` and uses `res["hidden_states"][-3:-2]` → 1024-dim features (<https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/melo/text/chinese_bert.py>).
- That BERT's published size is **1,306,484,351 bytes** (`pytorch_model.bin`, ≈1.22 GiB); also `tf_model.h5` 1,302,594,480 B and `flax_model.msgpack` 1,302,196,529 B (<https://huggingface.co/api/models/hfl/chinese-roberta-wwm-ext-large/tree/main>).
- That BERT's own licence is `apache-2.0` — a **separate** licence from MeloTTS (<https://huggingface.co/hfl/chinese-roberta-wwm-ext-large>).

**ONNX export and the Android/C++ path**

- ONNX export is **not** in the MeloTTS repo. The question is GitHub issue #98 "Is it possible to export to onnx?" (closed, 18 comments) (<https://github.com/myshell-ai/MeloTTS/issues/98>).
- The **official** conversion lives in sherpa-onnx: `scripts/melo-tts/` contains `export-onnx.py` (8,829 B), `export-onnx-en.py`, `run.sh`, `test.py`, `show-info.py`, `README.md` (<https://api.github.com/repos/k2-fsa/sherpa-onnx/contents/scripts/melo-tts>).
- sherpa-onnx docs state the model "is converted from https://huggingface.co/myshell-ai/MeloTTS-Chinese", that the converting script is at `scripts/melo-tts`, that "You can convert more models from https://github.com/myshell-ai/MeloTTS by yourself", and that English words outside `lexicon.txt` cannot be pronounced (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html>).
- **Python export step needed: yes** (the exporter imports `torch`, `onnx`, `melo.api`, `pypinyin`, `jieba` at export time only) — <https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/master/scripts/melo-tts/export-onnx.py>.
- **The exporter discards BERT**: in `ModelWrapper.forward` it passes
  `bert = torch.zeros(x.shape[0], 1024, x.shape[1], dtype=torch.float32)` and
  `ja_bert = torch.zeros(x.shape[0], 768, x.shape[1], dtype=torch.float32)`.
  The ONNX metadata it writes still declares `"bert_dim": 1024` and `"jieba": 1`, and records `"license": "MIT license"` (<https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/master/scripts/melo-tts/export-onnx.py>). This is why the shipped ONNX is 163 MB rather than ≈207 MB + ≈1.22 GiB.
- sherpa-onnx's own converter README notes there is "only a single female speaker in the model for Chinese+English TTS" (<https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/master/scripts/melo-tts/README.md>).

**Published speed / RTF and hardware**

- MeloTTS README claims only "Fast enough for `CPU real-time inference`" with **no hardware specified** (<https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/README.md>).
- sherpa-onnx publishes RTF for `vits-melo-tts-zh_en` **on Raspberry Pi 4 Model B Rev 1.5**: 1 thread **6.727**, 2 threads **3.877**, 3 threads **2.914**, 4 threads **2.518** (RTF > 1 = slower than real time) — <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html> and <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html>.
- RTF on arm64 Android hardware: **not published**.

**Output format / sample rate**

- Written as WAV, mono, **16-bit signed PCM at 44100 Hz** per the published `soxi` output (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html>), matching `"sampling_rate": 44100` in the config (<https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/melo/configs/config.json>).

**Reported pronunciation problems (issues)**

- #206 "How is pronunciation decided?" (open) — homographs: "It is pronouncing the `wind` in `wind power` the wrong way… Strangely, it gets it right for the default voices, but wrong when I trained a new English voice." (<https://github.com/myshell-ai/MeloTTS/issues/206>)
- #241 "Wrong pronunciation in english(all voices)" (open) — "the word `plugin` will be randomly pronounced correctly and wrong (`ploogin`) depending on the sentence" (<https://github.com/myshell-ai/MeloTTS/issues/241>)
- A GitHub issue search for `多音字` (polyphone) in the repo returns 3 results, none of which is a dedicated polyphone bug report (#193, #66, #98) — i.e. **no published Mandarin-多音字 bug report found** (<https://api.github.com/search/issues?q=repo%3Amyshell-ai%2FMeloTTS+%E5%A4%9A%E9%9F%B3%E5%AD%97>).

---

## 2. ChatTTS (2noise/ChatTTS)

| Item | Finding |
| --- | --- |
| Runtime dependency | PyTorch + a Vocos vocoder (PyTorch). |
| Prebuilt Android arm64 artifact | None. No C++/ONNX port in the organisation. |
| Architecture | **Autoregressive** (GPT + DVAE + Vocos). |
| Code licence | AGPL-3.0 |
| Weights licence | CC BY-NC 4.0 (academic only) |

**Repo and licence**

- Repo: <https://github.com/2noise/ChatTTS> — GitHub REST metadata reports `license.spdx_id = AGPL-3.0`, `pushed_at` 2026-04-10T16:33:48Z, 39,830 stars, 61 open issues, 4,249 forks, language Python, topics include `chinese` (<https://api.github.com/repos/2noise/ChatTTS>).
- README licence section, verbatim: "The code is published under `AGPLv3+` license." and "The model is published under `CC BY-NC 4.0` license. It is intended for educational and research use, and should not be used for any commercial or illegal purposes." (<https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md>)
- README "Dataset & Model" block, verbatim: "The released model is for academic purposes only." (<https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md>)
- The Hugging Face model card YAML for `2Noise/ChatTTS` declares `license: cc-by-nc-4.0`, `pipeline_tag: text-to-audio` (<https://huggingface.co/2Noise/ChatTTS>).

**Model names, published sizes, quantisation**

From the Hugging Face repository tree for `2Noise/ChatTTS` (<https://huggingface.co/api/models/2Noise/ChatTTS/tree/main/asset>):

| File | Exact size (bytes) |
| --- | --- |
| `GPT.pt` | 900,746,442 |
| `Embed.safetensors` | 145,598,536 |
| `Decoder.pt` | 103,718,156 |
| `Decoder.safetensors` | 103,694,920 |
| `DVAE_full.pt` | 60,402,442 |
| `DVAE.safetensors` | 60,359,112 |
| `Vocos.pt` | 54,363,119 |
| `Vocos.safetensors` | 54,348,240 |
| `DVAE.pt` | 27,749,823 |
| `tokenizer.pt` | 336,680 |
| `spk_stat.pt` | 4,257 |

- Sum of the `.pt` set ≈ 1.25 GB. **Quantisation variants (fp16/int8) of the official ChatTTS weights: not published.**
- Model scope as stated by the authors: "The main model is trained with Chinese and English audio data of 100,000+ hours. The open-source version on HuggingFace is a 40,000 hours pre-trained model without SFT." (<https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md>)

**Architecture and speed implications (autoregressive)**

- README FAQ answer 2 states directly: "This is a problem that typically occurs with autoregressive models (for bark and valle)." — in answer to instability such as multi-speaker output or poor audio quality (<https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md>).
- Acknowledgements name `bark`, `XTTSv2`, `valle` as "an autoregressive-style system", and `vocos` "which is used as a pretrained vocoder" — the same README (<https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md>).
- **An official ChatTTS paper: not published** (no arXiv reference appears in the repo README).

**Published speed numbers and hardware**

- README FAQ 1, verbatim: "For a 30-second audio clip, at least 4GB of GPU memory is required. For the 4090 GPU, it can generate audio corresponding to approximately 7 semantic tokens per second. The Real-Time Factor (RTF) is around 0.3." (<https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md>)
- RTF on CPU or on arm64/Android: **not published**.

**Chinese support claims**

- README "Supported Languages": `[x] English`, `[x] Chinese`, `[ ] Coming Soon...` (<https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md>)
- Chinese text normalisation is not self-contained: the repo's optional dependency set added by PR #178 is `pynini==2.1.5`, `WeTextProcessing`, `nemo_text_processing` "to support webui and Chinese processing" (<https://github.com/2noise/ChatTTS/pull/178>); the API example calls `chat.normalizer.register("zh", normalizer_zh_tn())` where `tools/normalizer/zh.py` does `from tn.chinese.normalizer import Normalizer`, which fails with `ModuleNotFoundError: No module named 'tn.chinese'` when `pynini`/`WeTextProcessing` are missing (issue #906, <https://github.com/2noise/ChatTTS/issues/906>).

**Android feasibility**

- README roadmap contains an **unchecked** box: "- [ ] ChatTTS.cpp (new repo in `2noise` org is welcomed)" — i.e. the authors had not produced a C++ port and were inviting someone else to (<https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md>).
- `https://api.github.com/repos/2noise/ChatTTS.cpp` returns `{"message":"Not Found"}` — the repo does not exist (<https://api.github.com/repos/2noise/ChatTTS.cpp>).
- A GitHub repository search for `ChatTTS cpp` returns `total_count: 0` (<https://api.github.com/search/repositories?q=ChatTTS+cpp>).
- No Android/Gradle/NDK build files are documented in the README; installation is `pip install ChatTTS` or `pip install -r requirements.txt`, with vLLM/TransformerEngine/FlashAttention-2 offered as Linux/NVIDIA-only options (<https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md>).

**Output format / sample rate**

- Inference returns a numpy array; the README examples all save via `torchaudio.save(..., 24000)` and the model-card example uses `Audio(wavs[0], rate=24_000)` — output sample rate **24000 Hz** (<https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md>, <https://huggingface.co/2Noise/ChatTTS>).

**Reported Chinese pronunciation problems (多音字) — this is the strongest evidence found**

- **Merged PR #350**, "[Fix] Replace mispronounced words in TTS using hack method", adds `ChatTTS/homophones_map.json` containing **16,000 entries**. Its stated method: "1. Identify correctly pronounced characters by ChatTTS. 2. Replace the mispronounced characters with correctly pronounced ones." Worked example given in the PR body: original `关关雎鸠，在河之洲。窈窕淑女，君子好逑。` → corrected `关关居鸠，在河之洲。咬挑淑女，君子好求。` Declared limitations: "1. Some characters and words might not be covered. 2. Some characters do not have correctly pronounced homophones, e.g. 'sǒu 叟'." (<https://github.com/2noise/ChatTTS/pull/350>)
- Related open issue #659 reports garbled Chinese/English input text followed by `Segmentation fault (core dumped)` (<https://github.com/2noise/ChatTTS/issues/659>).

---

## 3. Kokoro / Kokoro-82M (hexgrad)

| Item | Finding |
| --- | --- |
| Runtime dependency | PyTorch (reference lib) or **ONNX Runtime** (kokoro-onnx, sherpa-onnx). |
| Prebuilt Android arm64 artifact | Yes — sherpa-onnx Android TTS-engine APKs; third-party Kotlin apps. |
| Architecture | StyleTTS 2 + ISTFTNet; model card: "Decoder only: no diffusion, no encoder release". |
| Code licence | Inference lib `hexgrad/kokoro` = Apache-2.0; `kokoro-onnx` = MIT. |
| Weights licence | Apache-2.0 |

**Repo, licence, parameter count**

- Inference library: <https://github.com/hexgrad/kokoro> — GitHub REST metadata reports `license.spdx_id = apache-2.0`, 8,764 stars, `pushed_at` 2025-08-06T22:28:53Z (<https://api.github.com/repos/hexgrad/kokoro>).
- README, verbatim: "Kokoro is an open-weight TTS model with 82 million parameters… With Apache-licensed weights, Kokoro can be deployed anywhere from production environments to personal projects." (<https://raw.githubusercontent.com/hexgrad/kokoro/main/README.md>)
- Model card YAML for v1.0 declares `license: apache-2.0`, `base_model: yl4579/StyleTTS2-LJSpeech` (<https://huggingface.co/hexgrad/Kokoro-82M>).
- Model card YAML for v1.1-zh declares `license: apache-2.0`, `base_model: hexgrad/Kokoro-82M` (<https://huggingface.co/hexgrad/Kokoro-82M-v1.1-zh>).
- `kokoro-onnx`'s own README states the split plainly: "kokoro-onnx: MIT / kokoro model: Apache 2.0" (<https://raw.githubusercontent.com/thewh1teagle/kokoro-onnx/main/README.md>).

**Architecture (feed-forward, non-autoregressive)**

- Model card "Model Facts → Architecture": "StyleTTS 2: https://arxiv.org/abs/2306.07691", "ISTFTNet: https://arxiv.org/abs/2203.02395", "**Decoder only: no diffusion, no encoder release**" (<https://huggingface.co/hexgrad/Kokoro-82M>).
- StyleTTS 2 paper: style is modelled "as a latent random variable through diffusion models" with a differentiable duration model; no autoregressive decoder is described in the abstract (<https://arxiv.org/abs/2306.07691>).

**Chinese support and the actual voice names**

- v1.0 (published 2025 Jan 27) ships "8 & 54" languages & voices. `VOICES.md` lists Mandarin Chinese as `lang_code='z'` in `misaki[zh]`, "Total Mandarin Chinese training data: H hours" (i.e. 1–10 hours), with **4 female + 4 male** voices (<https://huggingface.co/hexgrad/Kokoro-82M/blob/main/VOICES.md>):

| Voice | Traits | Target Quality | Training Duration | Overall Grade | SHA256 |
| --- | --- | --- | --- | --- | --- |
| `zf_xiaobei` | F | C | MM minutes | **D** | `9b76be63` |
| `zf_xiaoni` | F | C | MM minutes | **D** | `95b49f16` |
| `zf_xiaoxiao` | F | C | MM minutes | **D** | `cfaf6f2d` |
| `zf_xiaoyi` | F | C | MM minutes | **D** | `b5235dba` |
| `zm_yunjian` | M | C | MM minutes | **D** | `76cbf8ba` |
| `zm_yunxi` | M | C | MM minutes | **D** | `dbe6e1ce` |
| `zm_yunxia` | M | C | MM minutes | **D** | `bb2b03b0` |
| `zm_yunyang` | M | C | MM minutes | **D** | `5238ac22` |

- The same file's header carries an explicit quality caveat, verbatim: "**Support for non-English languages may be absent or thin due to weak G2P and/or lack of training data.**" and "**Weakness** on short utterances, especially less than 10-20 tokens." (<https://huggingface.co/hexgrad/Kokoro-82M/blob/main/VOICES.md>)
- Mandarin is **not** in the `lang_code='a'` path; `VOICES.md` maps Spanish/French/Hindi/Italian/Portuguese to espeak-ng codes but maps Chinese to `misaki[zh]` (<https://huggingface.co/hexgrad/Kokoro-82M/blob/main/VOICES.md>).
- v1.1-zh (published 2025 Feb 26) is the Chinese-heavy release: "This model is the result of a short training run that added 100 Chinese speakers from a professional dataset" (LongMaoData); card table row: `v1.1-zh | 2025 Feb 26 | >100 hours | 2 & 103`; "82 million parameters, same as https://hf.co/hexgrad/Kokoro-82M"; "This model is not a strict upgrade over its predecessor since it drops many voices" (<https://huggingface.co/hexgrad/Kokoro-82M-v1.1-zh>).
- sherpa-onnx's speaker map for `kokoro-multi-lang-v1_1` enumerates the 103 speakers: 2 `af_` + 1 `bf_` + **55 `zf_`** (sid 3–57) + **45 `zm_`** (sid 58–102), named `zf_001…zf_099`, `zm_009…zm_100` (<https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese-English/kokoro-multi-lang-v1_1.html>).
- sherpa-onnx's v1.0 speaker map (53 speakers) includes exactly the eight VOICES.md Chinese voices: `45->zf_xiaobei, 46->zf_xiaoni, 47->zf_xiaoxiao, 48->zf_xiaoyi, 49->zm_yunjian, 50->zm_yunxi, 51->zm_yunxia, 52->zm_yunyang` (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html>).

**Chinese phonemizer: jieba + pypinyin via misaki — NOT espeak-ng for Chinese**

- The `kokoro` library uses `misaki` for G2P (<https://raw.githubusercontent.com/hexgrad/kokoro/main/README.md>).
- `misaki` `pyproject.toml` optional-dependency group `zh = ["jieba", "ordered-set", "pypinyin", "cn2an", "pypinyin-dict"]` — the Chinese front-end is jieba + pypinyin + cn2an, all Python (<https://raw.githubusercontent.com/hexgrad/misaki/main/pyproject.toml>).
- `misaki` README, verbatim on Chinese: "The second gen Chinese tokenizer adapts better logic from paddlespeech's frontend. Jieba now cuts and tags, and pinyin-to-ipa is no longer used." — with `PaddleSpeech/tree/develop/paddlespeech/t2s/frontend` cited. The first gen "uses jieba to cut, pypinyin, and pinyin-to-ipa" (<https://raw.githubusercontent.com/hexgrad/misaki/main/README.md>).
- `misaki`'s own TODOs list unresolved homograph work: "**Homographs**: Escalate hard words like `axes bass bow lead tear wind` using BERT contextual word embeddings (CWEs) and logistic regression (LR) models" — i.e. context-dependent pronunciation is an open item (<https://raw.githubusercontent.com/hexgrad/misaki/main/README.md>).
- espeak-ng is a dependency, but for English OOD fallback and other languages: the `kokoro` README says espeak is "used for English OOD fallback and some non-English languages" (<https://raw.githubusercontent.com/hexgrad/kokoro/main/README.md>).

**Published model file sizes and quantisation variants**

Hugging Face `hexgrad/Kokoro-82M` (<https://huggingface.co/api/models/hexgrad/Kokoro-82M/tree/main>):
- `kokoro-v1_0.pth` = **327,212,226 bytes** (≈312 MiB), SHA256 `496dba11…`
- `voices/` contains **54** `.pt` files, each **≈523,420–523,440 bytes**; the eight Chinese ones are `zf_xiaobei.pt` 523,435 B, `zf_xiaoni.pt` 523,430 B, `zf_xiaoxiao.pt` 523,440 B, `zf_xiaoyi.pt` 523,430 B, `zm_yunjian.pt` 523,435 B, `zm_yunxi.pt` 523,425 B, `zm_yunxia.pt` 523,430 B, `zm_yunyang.pt` 523,435 B (<https://huggingface.co/api/models/hexgrad/Kokoro-82M/tree/main/voices>)
- **No ONNX file is published in the Hugging Face repo.**

ONNX builds — `thewh1teagle/kokoro-onnx` release `model-files-v1.1` (<https://github.com/thewh1teagle/kokoro-onnx/releases/tag/model-files-v1.1>):

| File | Exact size (bytes) | Published quality note |
| --- | --- | --- |
| `kokoro-v1.0.onnx` | 325,505,369 | fp32 baseline (~326 MB) |
| `kokoro-v1.0.fp16.onnx` | 163,527,961 | "spectral correlation 0.999 against fp32" |
| `kokoro-v1.0.int8.onnx` | 114,119,327 | "spectral correlation 0.916 against fp32" |
| `kokoro-v1.1-zh.onnx` | 325,506,167 | fp32 baseline |
| `kokoro-v1.1-zh.fp16.onnx` | 163,528,759 | "spectral correlation 0.994 against fp32" |
| `kokoro-v1.1-zh.int8.onnx` | 114,120,125 | "spectral correlation 0.874 against fp32" |
| `voices-v1.0.bin` | 28,214,398 | 54 voices (v1.0 models) |
| `voices-v1.1-zh.bin` | 53,815,880 | 103 voices (v1.1-zh models) |

- The older release `model-files-v1.0` body states rounded sizes that differ from the current assets: "`kokoro-v1.0.onnx`: (310MB)… `kokoro-v1.0.fp16.onnx`: (169MB)… `kokoro-v1.0.int8.onnx`: (88MB)… `voices-v1.0.bin`: npz (numpy) key value pairs of style vectors contains the following voices (26)" — the actual v1.0 assets were `kokoro-v1.0.onnx` 325,532,387 B, `kokoro-v1.0.fp16.onnx` 177,464,787 B, `kokoro-v1.0.int8.onnx` 92,361,271 B, `kokoro-v1.0.fp16-gpu.onnx` 177,464,787 B, `voices-v1.0.bin` 28,214,398 B (<https://github.com/thewh1teagle/kokoro-onnx/releases/tag/model-files-v1.0>).
- `kokoro-onnx` README claims "Lightweight: ~300MB (quantized: ~80MB)" and requires "ONNX Runtime >= 1.20.1" (<https://raw.githubusercontent.com/thewh1teagle/kokoro-onnx/main/README.md>).

sherpa-onnx-packaged Kokoro (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html>):
- `kokoro-multi-lang-v1_0` file listing: `model.onnx` **310M**, `voices.bin` **26M**, `lexicon-zh.txt` 2.3M, `lexicon-us-en.txt` 5.6M, `lexicon-gb-en.txt` 6.0M, `date-zh.fst` 58K, `number-zh.fst` 63K, `phone-zh.fst` 87K, `tokens.txt` 687 B, plus `espeak-ng-data/` and `dict/`. Sample rate is stated as fixed: "Sample rate of this model is fixed to `24000 Hz`."
- `kokoro-multi-lang-v1_1` publishes two archives — a plain one and an int8 one: `kokoro-multi-lang-v1_1.tar.bz2` ("No quantization") and `kokoro-int8-multi-lang-v1_1.tar.bz2` ("`int8` quantization"). **Per-file sizes inside the v1.1 archives are not published**; the RTF table lists the v1.1 model as **311 MB** and `kokoro-en-v0_19` as **330 MB**.
- The v1.0 ONNX originates from `taylorchu/kokoro-onnx` release v0.2.0 (<https://github.com/taylorchu/kokoro-onnx/releases/tag/v0.2.0>).

**Published RTF / speed and hardware**

- Official sherpa-onnx table, **Raspberry Pi 4 Model B Rev 1.5** (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html>):
  - `kokoro-multi-lang-v1_1` (Chinese + English, 103 speakers, 311 MB): 1 thread **7.635**, 2 **4.470**, 3 **3.430**, 4 **3.191**
  - `kokoro-en-v0_19` (English, 11 speakers, 330 MB): 1 thread **6.629**, 2 **3.870**, 3 **2.999**, 4 **2.774**
- `kokoro-onnx` README claim: "Fast performance near real-time on macOS M1" — **no RTF number and no arm64-Android measurement published** (<https://raw.githubusercontent.com/thewh1teagle/kokoro-onnx/main/README.md>).
- RTF on arm64 Android hardware: **not published**.

**ONNX runtime builds and Android arm64 artifacts**

- `kokoro-onnx` provides an ONNX Runtime Python binding and prebuilt ONNX graphs (<https://raw.githubusercontent.com/thewh1teagle/kokoro-onnx/main/README.md>).
- sherpa-onnx ships a **prebuilt Android TTS Engine APK per ABI** for `kokoro-multi-lang-v1_1` at v1.13.7, including `sherpa-onnx-1.13.7-arm64-v8a-eng-tts-engine-kokoro-multi-lang-v1_1.apk`; source at `android/SherpaOnnxTtsEngine` (<https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese-English/kokoro-multi-lang-v1_1.html>).
- sherpa-onnx exposes Kokoro through a **C API** (`sherpa-onnx/c-api/c-api.h`) and a **C++ API** (`sherpa-onnx/c-api/cxx-api.h`) with `OfflineTtsConfig.model.kokoro.{model,voices,tokens,data_dir,lexicon}`; the documented C++ compile line is `g++ -std=c++17 … -lsherpa-onnx-cxx-api -lsherpa-onnx-c-api -lonnxruntime` — i.e. usable from Qt without Java (<https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese-English/kokoro-multi-lang-v1_1.html>).
- Third-party Android ports found:
  - `puff-dayo/Kokoro-82M-Android` — "A minimal Android demo app for Kokoro-82M TTS model in **int8 quantization**", Kotlin, licence **GPL-3.0**, GitHub `archived: true`, `pushed_at` 2025-02-05 (<https://api.github.com/repos/puff-dayo/Kokoro-82M-Android>); its open TODO includes "Multi language support (might need a bit refactoring...)" (<https://raw.githubusercontent.com/puff-dayo/Kokoro-82M-Android/latest/README.md>).
  - `siva-sub/NekoSpeak` — "Private, offline AI Text-to-Speech for Android with Kokoro, KittenTTS, Pocket-tts and Piper models", Kotlin, MIT (<https://api.github.com/repos/siva-sub/NekoSpeak>).
  - `Mobile-Artificial-Intelligence/maise` — "open-source android speech engine… on the edge", Kotlin, MIT, topics include `kokoro`, `onnxruntime` (<https://api.github.com/repos/Mobile-Artificial-Intelligence/maise>).
  - `biaji/kokoro-tts` — "基于Kokoro的Android TTS引擎", Java, GPL-2.0 (<https://api.github.com/repos/biaji/kokoro-tts>).

**Reported Chinese pronunciation problems (issues)**

- `hexgrad/kokoro` issue **#238**, open: "KokoroTTS does not perform well, and even fails to recognize sentences that mix Chinese and English… I suspect it might be due to the system converting every Chinese character into phonetic symbols based on pinyin to generate pronunciation, which causes parts that are originally English words to be unrecognizable. This issue is even more pronounced in Kokoro-zh-v1.1, as it doesn't attempt to pronounce the English words at all." (<https://github.com/hexgrad/kokoro/issues/238>)
- `hexgrad/kokoro` PR **#313** (open) proposes replacing English-only `phonemizer` with `ephone` (eSpeak-NG WASM) and its test plan lists "Chinese (zf_xiaoxiao)" as unverified; it notes "Japanese — hiragana/katakana input; kanji not supported by eSpeak-NG" (<https://github.com/hexgrad/kokoro/pull/313>).

**Output format / sample rate**

- 24000 Hz float PCM arrays in Python (`sf.write(f'{i}.wav', audio, 24000)`) (<https://raw.githubusercontent.com/hexgrad/kokoro/main/README.md>).
- sherpa-onnx: "Sample rate of this model is fixed to `24000 Hz`"; written and reported as mono 16-bit signed PCM WAV (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html>).

---

## 4. PaddleSpeech (PaddlePaddle/PaddleSpeech) — Chinese

| Item | Finding |
| --- | --- |
| Runtime dependency | PaddlePaddle required; Paddle Lite / Paddle Inference for C++; ONNX Runtime path is official but Python-side |
| Prebuilt Android arm64 artifact | Yes for **Paddle Lite** `.nb` models + an official Android demo APK; no standalone TTS `.so` |
| Architecture | FastSpeech2 acoustic model = non-autoregressive; Tacotron2/TransformerTTS = autoregressive |
| Code licence | Apache-2.0 |
| Weights licence | **not published** |

**Repo state**

- Repo: <https://github.com/PaddlePaddle/PaddleSpeech> — GitHub API licence field `apache-2.0`; default branch `develop`, tree sha `6b25a400008d393f9c3af837b3c692b17f29ee1a` (<https://api.github.com/repos/PaddlePaddle/PaddleSpeech>).
- README states the dependency directly: "PaddleSpeech depends on paddlepaddle" (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/README.md>).

**Runtime dependency — three engines in the Python code**

- `paddlespeech/t2s/exps/syn_utils.py` shows all three: Paddle dynamic graph (`get_am_inference`/`get_voc_inference`), Paddle Inference (`from paddle import inference`, `inference.create_predictor` in `get_predictor`), and ONNX Runtime (`import onnxruntime as ort` in `get_sess`) (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/exps/syn_utils.py>).
- Paddle Lite is a separate export target with its own scripts (`lite_predict.py`, `lite_predict_streaming.py`, `lite_syn_utils.py`) and its own published `*_pdlite_*.zip` archives (<https://github.com/PaddlePaddle/PaddleSpeech/tree/develop/paddlespeech/t2s/exps>).

**ONNX export — official, first-party**

- Export is **official**, via the first-party script `examples/csmsc/tts3/local/paddle2onnx.sh`, which wraps the `paddle2onnx` CLI:
  `paddle2onnx --model_dir ${train_output_path}/${model_dir} --model_filename ${model}.pdmodel --params_filename ${model}.pdiparams --save_file ${train_output_path}/${output_dir}/${model}.onnx --opset_version 11 --enable_dev_version ${enable_dev_version}` (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/examples/csmsc/tts3/local/paddle2onnx.sh>).
- It is wired into the recipe: `examples/csmsc/tts3/run_cnndecoder.sh` stage 7 "paddle2onnx non streaming" and stage 9 "paddle2onnx streaming" (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/examples/csmsc/tts3/run_cnndecoder.sh>).
- ONNX Runtime inference entry point: `examples/csmsc/tts3/local/ort_predict.sh` → `ort_predict_e2e.py` / `ort_predict.py` (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/examples/csmsc/tts3/local/ort_predict.sh>).
- CLI support: `paddlespeech tts --use_onnx true`; `ONNX_SUPPORT_SET` covers `fastspeech2_csmsc`, `_aishell3`, `_vctk`, `_male`, `_mix`, `_canton`, `speedyspeech_csmsc`, `pwgan_*`, `mb_melgan_csmsc`, `hifigan_*` (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/cli/tts/infer.py>).
- README changelog: "2022.06.22: All TTS models support ONNX format."; "2022.08.03: Add ONNXRuntime infer for TTS CLI." (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/README.md>)
- There is **no script named `to_onnx`** in the inspected paths (`paddlespeech/t2s/exps/`, `paddlespeech/t2s/exps/fastspeech2/`, `examples/csmsc/tts3/`) — the documented entry point is `paddle2onnx.sh` (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/examples/csmsc/tts3/local/paddle2onnx.sh>).
- **Caveat**: the ONNX path still imports PaddlePaddle in Python — `import paddle` / `paddle.get_device()` in `cli/tts/infer.py`, and `import paddle` + `paddle.to_tensor` in `t2s/frontend/zh_frontend.py` (the frontend returns Paddle tensors unless `to_tensor=False`, which the ONNX path passes) (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/cli/tts/infer.py>, <https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/frontend/zh_frontend.py>).

**Chinese TTS model names, published download URLs and sizes**

Source of sizes: <https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/released_model.md>. The table publishes **one size per row, in a column headed "Size (static)"**. Sizes of the checkpoint / ONNX / Paddle-Lite archives are **not published** — the CLI registry carries only `url` + `md5` (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/resource/pretrained_models.py>). All archives live under `https://paddlespeech.cdn.bcebos.com/Parakeet/released_models/<subdir>/<file>`.

| Model | Published artifacts | Published size |
| --- | --- | --- |
| `fastspeech2_csmsc` | ckpt `fastspeech2/fastspeech2_nosil_baker_ckpt_0.4.zip`; static `fastspeech2_csmsc_static_0.2.0.zip`; **ONNX** `fastspeech2_csmsc_onnx_0.2.0.zip`; pdlite `fastspeech2_csmsc_pdlite_1.3.0.zip` | 157 MB (static) |
| `fastspeech2_cnndecoder_csmsc` (streaming) | ckpt; static; streaming-static; **ONNX**; streaming ONNX; pdlite; streaming pdlite | 84 MB (static) |
| `fastspeech2_aishell3` | ckpt `fastspeech2_aishell3_ckpt_1.1.0.zip`; static; **ONNX** `fastspeech2_aishell3_onnx_1.1.0.zip`; pdlite | 147 MB (static) |
| `fastspeech2_canton` | ckpt `fastspeech2_canton_ckpt_1.4.0.zip`; static; **ONNX** `fastspeech2_canton_onnx_1.4.0.zip` | 146 MB (static) |
| `fastspeech2_male` (zh) / `ljspeech` / `vctk` / `zh_en` mix | ckpt + static + ONNX (pdlite for some) | 146 / 145 / 145 / 145 MB (static) |
| `fastspeech2` (Conformer) csmsc | ckpt only (`fastspeech2_conformer_baker_ckpt_0.5.zip`) | **not published** |
| `speedyspeech_csmsc` | static / ONNX / pdlite | 13 MB (static) |
| `tacotron2_csmsc` (autoregressive) | static only | 103 MB (static) |
| `pwgan_csmsc` (vocoder) | ckpt `pwg_baker_ckpt_0.4.zip`; static; **ONNX** `pwgan_csmsc_onnx_0.2.0.zip`; pdlite | 4.8 MB (static) |
| `mb_melgan_csmsc` (vocoder) | ckpt `mb_melgan_csmsc_ckpt_0.1.1.zip`; static; **ONNX** `mb_melgan_csmsc_onnx_0.2.0.zip`; pdlite | 7.6 MB (static) |
| `hifigan_csmsc` (vocoder) | ckpt `hifigan_csmsc_ckpt_0.1.1.zip`; static; **ONNX** `hifigan_csmsc_onnx_0.2.0.zip`; pdlite | 46 MB (static) |
| `style_melgan_csmsc` | ckpt `style_melgan_csmsc_ckpt_0.1.1.zip` | **not published** |
| `wavernn_csmsc` | static | 18 MB (static) |

- Published md5 (size not published) for the CLI-resolvable ONNX archives: `fastspeech2_csmsc_onnx_0.2.0.zip` `fd3ad38d83273ad51f0ea4f4abf3ab4e`; `fastspeech2_aishell3_onnx_1.1.0.zip` `a1d6ee21de897ce394f5469e2bb4df0d`; `fastspeech2_canton_onnx_1.4.0.zip` `1c8d51ceb2f9bdd168e23be575c2ccf8`; `hifigan_csmsc_onnx_0.2.0.zip` `1a7dc0385875889e46952e50c0994a6b`; `pwgan_csmsc_onnx_0.2.0.zip` `711d0ade33e73f3b721efc9f20669f9c`; `mb_melgan_csmsc_onnx_0.2.0.zip` `5b83ec746e8414bc29032d954ffd07ec` (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/resource/pretrained_models.py>).
- Quantisation variants: Paddle Lite `.nb` variants are published per model (`*_pdlite_1.3.0.zip`). fp16/int8 ONNX variants: **not published** (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/released_model.md>).

**Autoregressive vs. non-autoregressive**

- FastSpeech 2 paper abstract opens: "Non-autoregressive text to speech (TTS) models such as FastSpeech…" (<https://arxiv.org/abs/2006.04558>).
- PaddleSpeech docs describe FastSpeech as "a novel feed-forward structure, which can generate a target mel spectrogram sequence in parallel" with "Non-autoregressive decode" (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/tts/models_introduction.md>).
- Same doc classifies Tacotron2 / TransformerTTS as autoregressive ("unidirectional RNN", "Autoregressive teacher force training", "The autoregressive decoder cannot be stopped"), and Parallel WaveGAN as "non-autoregressive both in training and prediction"; WaveRNN is listed under "Autoregression" (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/tts/models_introduction.md>).
- HiFi-GAN paper contrasts its GAN generator with "autoregressive and flow-based generative models" (<https://arxiv.org/abs/2010.05646>).

**Chinese text front-end — jieba + g2pM + g2pW (ONNX BERT) + pypinyin + tone sandhi**

- Main file `paddlespeech/t2s/frontend/zh_frontend.py` imports: `import jieba.posseg as psg`; `from g2pM import G2pM`; `from pypinyin import lazy_pinyin, load_phrases_dict, load_single_dict, Style`; `from pypinyin_dict.phrase_pinyin_data import large_pinyin`; `from paddlespeech.t2s.frontend.g2pw import G2PWOnnxConverter`; `from paddlespeech.t2s.frontend.polyphonic import Polyphonic`; `from paddlespeech.t2s.frontend.tone_sandhi import ToneSandhi`; `from paddlespeech.t2s.frontend.zh_normalization.text_normlization import TextNormalizer`; SSML `MixTextProcessor`; optional `RhyPredictor` (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/frontend/zh_frontend.py>).
- Default is `g2p_model="g2pW"` with `assert g2p_model in ('pypinyin', 'g2pM', 'g2pW')`; the g2pW path also loads g2pM and uses pypinyin "as backup for non polyphonic characters in g2pW" (same URL).
- **g2pW runs an ONNX BERT for polyphone disambiguation**: `paddlespeech/t2s/frontend/g2pw/onnx_api.py` uses `onnxruntime.InferenceSession(<dir>/g2pW.onnx)`, `BertTokenizer` from `paddlenlp.transformers`, `opencc.OpenCC('s2tw')`, `model_version = '1.1'`, assets `POLYPHONIC_CHARS.txt` / `MONOPHONIC_CHARS.txt`; header note "This code is modified from https://github.com/GitYCC/g2pW" (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/frontend/g2pw/onnx_api.py>).
- g2pW model archive: `G2PWModel_1.1.zip`, md5 `f8b60501770bff92ed6ce90860a610e6`, at `https://paddlespeech.cdn.bcebos.com/Parakeet/released_models/g2p/new/G2PWModel_1.1.zip`; **size not published** (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/resource/pretrained_models.py>).
- Official docs statement: "We use g2pM and pypinyin as the default g2p tools. They can solve the problem of polyphones to a certain extent… However, g2pM and pypinyin do not perform well in tone sandhi, we use rules to solve this problem" (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/tts/zh_text_frontend.md>).
- Frontend directory inventory (file names + byte sizes) at <https://api.github.com/repos/PaddlePaddle/PaddleSpeech/contents/paddlespeech/t2s/frontend?ref=develop> — `zh_frontend.py` 28,027 B, `g2pw/`, `polyphonic.py`, `polyphonic.yaml`, `tone_sandhi.py`, `zh_normalization/`, `normalizer/`, `ssml/`, `rhy_prediction/`, `canton_frontend.py`, `mix_frontend.py`, `en_frontend.py`.
- `nltk` is **not** imported by `zh_frontend.py` (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/frontend/zh_frontend.py>).

**Published speed / RTF numbers and the exact hardware**

- PPTTS doc claim: "Using ONNXRuntime to optimize the inference of TTS models, so that the TTS system can also achieve RTF < 1 on low-voltage" — **hardware not specified and no numeric RTF given** (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/tts/PPTTS.md>).
- Wiki TTS-Benchmark, "Paddle 2.2" tables, hardware given as `8x Tesla V100-SXM2-32GB, 24 core Intel(R) Xeon(R) Gold 6148, 100Gbps RDMA network (GPU 是在单卡上执行)`, headless offline inference, text frontend excluded. GPU / CPU RTF: `fastspeech2` am-only 0.01358 / 0.07090; `fastspeech2`+`pwgan` 0.03191 / **2.371**; `fastspeech2`+`mb_melgan` 0.01831 / **0.2733**; `fastspeech2`+`hifigan` 0.0218 / **1.7128**; `tacotron2` am-only 0.3249 / 0.67911; vocoder-only `pwgan` 0.01776 / 2.2521, `mb_melgan` 0.00347 / 0.1958, `style_melgan` 0.01271 / 2.9131, `hifigan` 0.00599 / 1.62216 (<https://raw.githubusercontent.com/wiki/PaddlePaddle/PaddleSpeech/TTS-Benchmark.md>).
- Same wiki, streaming TTS (Device `cpu`, `merge_sentences=True`, `cpu_thread=4`, AM `fastspeech2_cnndecoder_onnx` `am_block=72`/`am_pad=12`, vocoder `hifigan_onnx` `voc_block=36`/`voc_pad=14`, ONNXRuntime 1.10.0). Machines: server `28 Intel Xeon CPU E5-2680 v4 @ 2.40GHz`; Windows laptop `Intel Core i5-8250U @ 1.60GHz`; Mac laptop `Intel Core i5-8257U @ 1.40GHz`. RTF http/websocket: 0.488 / 0.485; 0.623 / 0.588; 0.580 / 0.584. Mean first-packet 0.233 / 0.221 s; 0.311 / 0.278 s; 0.405 / 0.420 s (same wiki).
- Wiki "Paddle 2.3" tables are largely empty; filled cells: GPU vocoder-only `pwgan` 0.01508; CPU vocoder-only `pwgan` 1.60, `hifigan` 0.6 (same wiki).
- Demo README log lines (hardware not stated): RTF 0.8216266382753459 (http) and 0.8363677006141812 (websocket) (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/demos/streaming_tts_server/README.md>).
- RTF on arm64 Android hardware: **not published**.

**Android support — yes, but it is a Paddle Lite path and has no text front-end**

- Official demo `demos/TTSAndroid` (added 2022-11-30 per README changelog). Its README: requires Android Studio + NDK; "该过程会自动下载 Paddle Lite 预测库和模型"; uses the **Paddle Lite Java API** (`com.baidu.paddle.lite.demo.tts.Predictor`, `PaddlePredictor.jar`, `jniLibs/arm64-v8a/libpaddle_lite_jni.so`); models are Paddle-Lite `.nb` files `fastspeech2_csmsc_arm.nb` + `mb_melgan_csmsc_arm.nb` from `fastspeech2_cnndecoder_csmsc_pdlite_1.3.0.zip` / `mb_melgan_csmsc_pdlite_1.3.0.zip`. **Explicitly no text front-end**: "本 Demo 不包含文本前端模块 … 如需文本前端模块请自行处理", pointing to community projects `lym0302/paddlespeech_tts_cpp` and `yazone/g2pE_mobile`. Prebuilt APK: `https://paddlespeech.cdn.bcebos.com/demos/TTSAndroid/2022-11-29-app-release.apk` (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/demos/TTSAndroid/README.md>).
- `demos/TTSArmLinux` (added 2023-03-07, "TTS ARM Linux C++ Demo (with C++ Chinese Text Frontend)"): Paddle Lite **C++** API, prebuilt libs pinned to Paddle-Lite commit `68b66fd356c875c92167d311ad458e6093078449`; notes "Paddle-Lite 2.12 与 TTS 不兼容，无法导出或运行 TTS 模型"; "目前只支持中文合成，出现任何英文都会导致程序崩溃" (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/demos/TTSArmLinux/README.md>).
- `demos/TTSCppFrontend`: standalone C++ Chinese text frontend (`front_interface.cpp`, `text_normalize.cpp`, dictionary generation tools), no neural inference (<https://github.com/PaddlePaddle/PaddleSpeech/tree/develop/demos/TTSCppFrontend>).
- Documented conclusion from these sources: the on-device Android/ARM deployment path **requires Paddle Lite**.

**C++ inference without Paddle Lite / PaddlePaddle**

- All in-repo C++ paths require a Paddle runtime: `demos/TTSArmLinux` → Paddle Lite C++ API; `demos/TTSCppFrontend` → text frontend only, no inference; `runtime/` → built against Paddle Inference (`libpaddle.so`, requires `paddlepaddle`) (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/runtime/README.md>).
- ONNX inference in this repo is documented **only in Python** (`get_sess`, CLI `--use_onnx`, `ort_predict*.py`).
- Exported files are ONNX **opset 11** (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/examples/csmsc/tts3/local/paddle2onnx.sh>).
- **No in-repo documentation was found for running the exported ONNX with onnxruntime's C++ API** — no C++ onnxruntime demo, CMake target or doc page in the inspected paths. Whether that path works is therefore **not documented by these sources**.

**Licences, code vs. weights**

- Code: **Apache-2.0** — the repo `LICENSE` is the unmodified Apache-2.0 text (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/LICENSE>); README: "PaddleSpeech is provided under the Apache-2.0 License." (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/README.md>).
- Weights: **no separate model licence is published.** Evidence: (a) no `NOTICE` file in the repo-root listing (root entries enumerated from <https://api.github.com/repos/PaddlePaddle/PaddleSpeech/git/trees/develop?recursive=1>); (b) `docs/source/released_model.md` carries no licence column or statement; (c) the machine-readable model registry entries contain only `url` / `md5` / file names (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/resource/pretrained_models.py>); (d) weights are hosted on Baidu's CDN `paddlespeech.cdn.bcebos.com` with no licence text in the docs.
- Commercial-use restrictions on weights: **not published**. Dataset licences (CSMSC / AISHELL-3 / VCTK / LJSpeech): **not published** in those docs.
- Third-party code inside the repo with its own provenance note: `paddlespeech/t2s/frontend/g2pw/onnx_api.py` — "This code is modified from https://github.com/GitYCC/g2pW" (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/frontend/g2pw/onnx_api.py>).

**Output format / sample rate**

- README Quick Start: "Output 24k sample rate wav format audio" (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/README.md>).
- CLI writes WAV via soundfile at the acoustic model's `fs`: `sf.write(output, self._outputs['wav'].numpy(), samplerate=self.am_config.fs)` and the ONNX variant with `self.am_fs`; CLI `--fs` default 24000 (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/cli/tts/infer.py>).
- CSMSC HiFiGAN vocoder config `fs: 24000` (`n_shift: 300`, `win_length: 1200`, `n_mels: 80`) (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/examples/csmsc/voc5/conf/default.yaml>). Registry `sample_rate`: 24000 for all csmsc/aishell3/canton/male models and speedyspeech; **22050** for ljspeech models (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/resource/pretrained_models.py>).
- The streaming server emits **raw int16 PCM** chunks, not WAV: `wav = float2pcm(wav)  # float32 to int16`, then `tobytes()` + base64; `self.sample_rate = self.executor.am_config.fs` with an assert that AM and vocoder sample rates match (<https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/server/engine/tts/online/python/tts_engine.py>). The WebSocket API wiki documents the TTS `audio` field as base64 and status 1/2 for last-chunk signalling, **without specifying the payload encoding** (<https://raw.githubusercontent.com/wiki/PaddlePaddle/PaddleSpeech/PaddleSpeech-Server-WebSocket-API.md>).

**Reported Chinese pronunciation problems (tones / 多音字) — API-verified via <https://api.github.com/search/issues?q=repo:PaddlePaddle/PaddleSpeech+%E5%A4%9A%E9%9F%B3%E5%AD%97>**

- **#3737** "TTS多音字问题" — closed 2024-04-12, 2 comments. Report: the HTTP TTS server pronounces 行 as `xing` instead of `hang` in "是否有我行人员或其他人员向你收取手续费？" (<https://github.com/PaddlePaddle/PaddleSpeech/issues/3737>).
- **#3297** "TTS多音字问题" — closed 2025-06-27, 7 comments. Report: 银行/行人 are handled correctly but "欢迎来到我行办理业务" is not (<https://github.com/PaddlePaddle/PaddleSpeech/issues/3297>).
- **#3664** "fastspeech2_aishell3效果很差" — closed 2025-06-27. Multiple voices despite `spk_id`, some characters unclear (<https://github.com/PaddlePaddle/PaddleSpeech/issues/3664>).
- **#4171** "[TTS]Bug Report: PaddleSpeech 1.5.0 TTS CPU segfault on paddlepaddle 2.6.2" — **open**, created 2026-06-15 (<https://github.com/PaddlePaddle/PaddleSpeech/issues/4171>).
- Title-only (not API-verified; the REST API rate limit was reached during research): #1283 "[tts] 基于 BERT 实现语音合成文本前端的多音字预测" (<https://github.com/PaddlePaddle/PaddleSpeech/issues/1283>); #3311, #3210, #3422, #678 also retrieved.

**Explicitly "not published" for PaddleSpeech**

ONNX / checkpoint / Paddle-Lite archive sizes (md5 only); `G2PWModel_1.1.zip` size; RTF for any current Paddle version on any specific phone/SoC; model-weight licence and any commercial-use terms; dataset licences; hardware for the PPTTS "RTF < 1 on low-voltage" claim; hardware for the streaming-demo README log RTFs.

---

## 5. ekho (hgneng/ekho) — Chinese TTS

| Item | Finding |
| --- | --- |
| Runtime dependency | **Not** dependency-free: libcurl + libsndfile + libespeak-ng mandatory; libpulse mandatory by default |
| Prebuilt Android arm64 artifact | Not in `hgneng/ekho` itself; the separate `hgneng/ekho-android-cantonese` repo has a working ndk-build with `arm64-v8a` |
| Synthesis method | Concatenative — one recorded audio file per pinyin syllable, concatenated |
| Code licence | GPL-2.0-or-later |
| Voice-data licence | **Separate per-voice `COPYING-*` files; none exists for Mandarin** |

**Repo identity**

- `hgneng/ekho`, description "Chinese text-to-speech engine", created 2016-04-14T01:18:43Z, `pushed_at` 2026-09-02T00:48:00Z, default branch `master`, GitHub-reported licence `GPL-2.0`, size 164,099 KB, 1,212 stars, 266 forks (<https://api.github.com/repos/hgneng/ekho>).
- README states the repo is a fork of the eGuideDog SourceForge tree at r2418 (patched to r2478) (<https://raw.githubusercontent.com/hgneng/ekho/master/README.md>).
- Supported languages listed in the plain-text README: Cantonese, Mandarin, Hakka, Tibetan, Ngangien, English (through Festival); Korean in trial (<https://raw.githubusercontent.com/hgneng/ekho/master/README>).

**Licence — code vs. voice data**

- Code licence is **GPL-2.0-or-later**, not a fixed version. `COPYING`, verbatim: "This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or any later version." Copyright "Cameron Wong (name in passport: HUANG GUANNENG) 2008-2022" (<https://raw.githubusercontent.com/hgneng/ekho/master/COPYING>).
- A separate `LICENSE` file exists and is verbatim GPL v2 text (June 1991) (<https://raw.githubusercontent.com/hgneng/ekho/master/LICENSE>). GitHub's detector reports `spdx_id: GPL-2.0` (<https://api.github.com/repos/hgneng/ekho>).
- **Voice-data licence is documented separately.** `COPYING`, verbatim: "For copyright information of voice data of Ekho, please refer their copyright description files that begin with `COPYING-`." (<https://raw.githubusercontent.com/hgneng/ekho/master/COPYING>)
- The per-voice `COPYING-*` files that exist in the tree are: `ekho-data/jyutping/COPYING-cantonese-wong`, `ekho-data/hakka/COPYING-hakka-weicheng`, `ekho-data/hangul/COPYING-korean-haesung`, `ekho-data/ngangien/COPYING-ngangien-qianshan`, `ekho-data/toisanese/COPYING-toisanese-StephenLi` (<https://api.github.com/repos/hgneng/ekho/git/trees/master?recursive=1>).
- Example (Cantonese): "Copyright (C) 2008-2022 by Cameron Wong (name in passport: HUANG GUANNENG)" followed by the GPL v2 text, pointing to `http://creativecommons.org/licenses/GPL/2.0/` and `http://www.gnu.org/licenses/old-licenses/gpl-2.0.html` (<https://raw.githubusercontent.com/hgneng/ekho/master/ekho-data/jyutping/COPYING-cantonese-wong>).
- **There is no `COPYING-*` file for the Mandarin (`pinyin`) voice set.** The only file in `ekho-data/pinyin/` is `ekho-data/pinyin/README`; the Mandarin audio ships inside `ekho-data/pinyin.voice` / `ekho-data/pinyin.index` (<https://api.github.com/repos/hgneng/ekho/git/trees/master?recursive=1>). **The Mandarin voice-data licence is therefore "not published" as a per-voice COPYING file.**
- eGuideDog's own project page states "License: GPL v2" (<https://www.eguidedog.net/ekho.php>).

**Synthesis method — concatenative**

- `README.md` does **not** describe the method (<https://raw.githubusercontent.com/hgneng/ekho/master/README.md>).
- The Mandarin voice README states the mechanism (Chinese), verbatim: "Ekho语音合成软件的原理很简单，为每一个拼音录制一个音频文件，然后拼接起来合成句子" — i.e. **one recorded audio file per pinyin syllable, concatenated to synthesise sentences**. The same file notes the default package's audio is `.gsm` (smaller size/quality) and that newer Mandarin versions are compressed into `pinyin.voice` (<https://raw.githubusercontent.com/hgneng/ekho/master/ekho-data/pinyin/README>).
- The concatenation/overlap algorithm is `EkhoImpl::writeToSonicStream` with `OverlapType` cases `OVERLAP_NONE`, `OVERLAP_DEFAULT`, `OVERLAP_QUIET_PART`, `OVERLAP_HALF_PART` (overlap of low-amplitude frames between adjacent units) (<https://raw.githubusercontent.com/hgneng/ekho/master/src/libekho_impl.cpp>).
- Voice-data index format, documented in `src/README`: 5 header bytes = `unsigned short samplerate; unsigned byte (1 = 16bit wav, 2 = 16bit gsm); unsigned short symbolCount;` then per-syllable records with `unsigned int frameOffset` and a 3-byte `frames` count (<https://raw.githubusercontent.com/hgneng/ekho/master/src/README>).
- Data flow (`src/README`): look up the Chinese string in `Dict`, split into words/English, query the phonetic symbol per part, synthesise each phonSymbol; English is synthesised with eSpeak or Festival (same URL).

**Mandarin and Cantonese voice names / file names / sizes**

- CLI voice selection: `ekho "text"` (default Mandarin), `ekho -v Mandarin "..."`, `ekho -v Cantonese "..."` (<https://raw.githubusercontent.com/hgneng/ekho/master/README>).
- Voice-name aliases map onto data-directory names in code: `Mandarin`/`zh`/`cmn` → `pinyin`; `Cantonese`/`yue` → `jyutping`; also `hangul`, `toisanese`, `hakka`, `tibetan`/`bo`, `ngangien`, `English` (<https://raw.githubusercontent.com/hgneng/ekho/master/src/libekho_impl.cpp>).
- In-repo Mandarin artefacts with exact sizes: `ekho-data/pinyin.voice` **1,803,172 B**, `ekho-data/pinyin.index` **16,395 B**, `ekho-data/zh.dict` 703,508 B, `ekho-data/zh_list` 36,496 B, `ekho-data/zh_listx` 1,256,746 B, `ekho-data/zh_patch` 4,861 B, `ekho-data/zhy.dict` 236,554 B, `ekho-data/zhy_list` 501,083 B (<https://api.github.com/repos/hgneng/ekho/contents/ekho-data>).
- In-repo Cantonese artefacts: `ekho-data/jyutping.voice` **5,634,328 B**, `ekho-data/jyutping.index` **50,125 B** (same URL).
- **Discrepancy, reported as found**: `README.md` states "Voice files are not included." (<https://raw.githubusercontent.com/hgneng/ekho/master/README.md>) yet the master tree at the fetched commit does contain `ekho-data/pinyin.voice` (1,803,172 B) and `ekho-data/jyutping.voice` (5,634,328 B) as committed blobs (<https://api.github.com/repos/hgneng/ekho/git/trees/master?recursive=1>).
- The `*.voice`/`*.index` pair is a **cache** regenerated from the audio directory on first run, verbatim: "在第一次运行之后会自动生成pinyin.voice和pinyin.index，它们是缓存文件" (<https://raw.githubusercontent.com/hgneng/ekho/master/ekho-data/pinyin/README>).
- **Published downloadable voice-data sizes** (SourceForge "Ekho Voice Data 0.2") (<https://sourceforge.net/projects/e-guidedog/files/Ekho-Voice-Data/0.2/>):

| Package | Date | Published size |
| --- | --- | --- |
| `pinyin-yali-44100-wav-v10.tar.xz` | 2021-03-22 | 51.8 MB |
| `pinyin-huang-44100-wav-v3.tar.xz` | 2020-07-13 | 86.3 MB |
| `pinyin-huang-16000-wav.tar.xz` | 2018-08-31 | 34.2 MB |
| `pinyin-yali-16000-1x.tar.bz2` | 2008-10-10 | 16.6 MB |
| `pinyin-yali-44100.tar.bz2` | 2008-07-27 | 43.0 MB |
| `jyutping-wong-44100-v9.tar.bz2` | 2011-03-12 | 103.1 MB |
| `jyutping-wong-16000-v6.tar.bz2` | 2009-03-08 | 19.6 MB |
| `toisanese-stephenli-44100-wav-v3.tar.xz` | — | 84.8 MB |
| `hakka-weicheng-24000-wav-v2.tar.xz` | — | 64.2 MB |
| `tibetan-trinley-44100-wav-v1.tar.xz` | — | 94.5 MB |
| `hangul-haesung-44100.tar.bz2` | — | 39.3 MB |
| `alphabet-wong-44100.tar.bz2` | — | 2.7 MB |
| **Totals line** | | **16 items, 767.5 MB** |

- Naming convention is documented: the number in the filename is the sample rate (e.g. `44100` = CD quality), and the extracted directory must be renamed to the plain voice name (e.g. `tibetan-trinley-44100-wav` → `tibetan`) (<https://raw.githubusercontent.com/hgneng/ekho/master/ekho-data/pinyin/README>).
- Installer helper referenced for the main voices: `./install.pl Mandarin | Cantonese | Tibetan` (speech-dispatcher setup) (<https://raw.githubusercontent.com/hgneng/ekho/master/INSTALL>).
- Repo topics: `cantonese`, `chinese`, `tibetan`, `tts` (<https://api.github.com/repos/hgneng/ekho>).

**Android / arm64 build support**

- **The `ekho` master tree contains no Android build files.** The complete recursive tree (`"truncated": false`) has no `android_build.sh`, no `Android.mk`, no `AndroidManifest.xml`, no `jni/` directory, and no `CMakeLists.txt`; build files present are `Makefile.am`, `configure.ac`, `autogen.sh`, `Dockerfile`, `ekho.iss`, `deploy.sh`, `deploy_tts.sh`, `install.pl` (<https://api.github.com/repos/hgneng/ekho/git/trees/master?recursive=1>).
- The build system is autotools only: `AC_INIT([ekho],[11.0],...)`, `LT_INIT`, `AC_CONFIG_FILES([Makefile])` (<https://raw.githubusercontent.com/hgneng/ekho/master/configure.ac>).
- **No arm64/aarch64 cross-compile support in either build file.** `configure.ac` offers exactly: `--with-soxr`, `--without-pulseaudio`, `--enable-speechd`, `--enable-flite`, `--enable-festival`, `--enable-win32`, `--with-mp3lame`, `--disable-dist-voice` — no `--host`/target-arch handling and no Android/arm option (<https://raw.githubusercontent.com/hgneng/ekho/master/configure.ac>). `Makefile.am` defines `host_cpu=\`uname -p\`` but never uses `$(host_cpu)`; it defines `noinst_LIBRARIES = libekho.a` and `bin_PROGRAMS = test_ekho ekho`, with no aarch64 target or toolchain variable (<https://raw.githubusercontent.com/hgneng/ekho/master/Makefile.am>).
- Android support appears only as conditional logging hooks in the source: `#ifdef DEBUG_ANDROID LOGD(...)` / `LOGI(...)` in `src/audio.cpp` (<https://raw.githubusercontent.com/hgneng/ekho/master/src/audio.cpp>). The ChangeLog records: "4.0: Dictionary module is rewritten. Much faster. Android is supported" (<https://raw.githubusercontent.com/hgneng/ekho/master/ChangeLog>).
- eGuideDog's project page states Ekho "supports Linux, Windows and Android platforms" and lists for Android 5.0+: `ekho-cantonese-9.0.apk (22M)` (<https://www.eguidedog.net/ekho.php>).
- eGuideDog's own Android how-to (updated 2012-04-03) documents package `net.eguidedog.ekho` on Google Play ("Ekho TTS" engine), installing voice data separately because "语音数据文件较大，没有包含在APK包中" (the voice data files are large and are not included in the APK), then selecting Ekho TTS as the default engine (<https://www.eguidedog.net/doc/doc_use_ekho_android_cn.php>).

**ekho-android: the repo at `hgneng/ekho-android` does not exist**

- `GET https://api.github.com/repos/hgneng/ekho-android` returns HTTP 404 `{"message":"Not Found"}` (<https://api.github.com/repos/hgneng/ekho-android>).
- `https://github.com/hgneng/ekho-android` returns HTTP 404 ("Page not found · GitHub") (<https://github.com/hgneng/ekho-android>).
- It is absent from the owner's repository list (`GET https://api.github.com/users/hgneng/repos?per_page=100&sort=updated`) (<https://api.github.com/users/hgneng/repos?per_page=100&sort=updated>).
- Therefore, for `ekho-android`: last commit date — **not published** (repo unreachable/removed, so no `pushed_at`); what it was — **not published** at any reachable primary source; why deprecated — **not published**; Android build instructions — **not published** at that URL; licence — **not published** at that URL. (`web.archive.org` was unreachable from the research session, so the archived copy could not be checked either.)
- A GitHub repository search for `ekho-android` returns exactly one repository, **`hgneng/ekho-android-cantonese`** (not `ekho-android`): created 2022-08-31T02:37:37Z, `pushed_at` 2024-07-09T05:27:08Z, language `C`, default branch `main`, licence `GPL-2.0`, size 32,566 KB, 4 stars (<https://api.github.com/search/repositories?q=ekho-android>, <https://api.github.com/repos/hgneng/ekho-android-cantonese>).
- That successor repo contains a working Android build:
  - `jni/Android.mk` builds `libsndfile` (`BUILD_STATIC_LIBRARY`) from vendored `libsndfile/src` (including `GSM610` and `G72x`), then builds the ekho sources from `EKHO_SRC_PATH := ../../ekho/src` (blocklisting `ekho.cpp` and `test_ekho.cpp`), plus `sr-convert/dsp.cpp`, `sonic/sonic.c` and all `jni/*.c*`, compiled with `-fexceptions -DOUTPUT16BIT -DNO_SSE -O0 -DANDROID -DDEBUG_ANDROID` and `LOCAL_LDLIBS := -llog -lstdc++`, plus static flite archives (`libflite_cmu_us_kal16.a`, `libflite_cmulex.a`, `libflite_usenglish.a`, `libflite.a` from `flite/build/$(TARGET_ARCH_ABI)-android/lib`). Final module: `LOCAL_MODULE := libttsekho`, `include $(BUILD_SHARED_LIBRARY)` (<https://raw.githubusercontent.com/hgneng/ekho-android-cantonese/main/jni/Android.mk>).
  - `build.gradle`: `namespace "net.eguidedog.ekho"`, `applicationId "cameronhuang150.ekho"`, `compileSdk 35`, `minSdkVersion 21`, `targetSdk 35`, `versionCode 42`, `versionName "4.2"`, and explicitly `ndk { abiFilters 'arm64-v8a', 'armeabi-v7a', 'x86', 'x86_64' }` with `externalNativeBuild { ndkBuild { path "jni/Android.mk" } }`, AGP `com.android.tools.build:gradle:8.5.0` (<https://raw.githubusercontent.com/hgneng/ekho-android-cantonese/main/build.gradle>).
  - Caveats visible in the build file: the ekho source tree is consumed **out of tree** (`../../ekho/src`, i.e. user-managed checkout placement) and the flite static libraries must be pre-built per ABI (<https://raw.githubusercontent.com/hgneng/ekho-android-cantonese/main/jni/Android.mk>).
  - `README.md` on that repo's `main` branch returns 404 — no README is published there (<https://raw.githubusercontent.com/hgneng/ekho-android-cantonese/main/README.md>).

**Output sample rate**

- There is **no fixed output-sample-rate constant** for Chinese. `EkhoImpl::initStream()` sets `mDict.mSfinfo.samplerate = 16000; mDict.mSfinfo.channels = 1;` **only for the English path**; for every other language it uses whatever sample rate was read from the voice data and errors with `"Sample rate not detected: "` if it is 0. It then calls `audio->setInputSampleRate(mDict.mSfinfo.samplerate)` and `initProcessor()`; `outputSampleRate` defaults to the input rate when unset (<https://raw.githubusercontent.com/hgneng/ekho/master/src/libekho_impl.cpp>, <https://raw.githubusercontent.com/hgneng/ekho/master/src/audio.cpp>).
- Consequently, **output sample rate = the sample rate stored in the voice-data index header**, which per the documented index format is the first `unsigned short` of `*.index` (<https://raw.githubusercontent.com/hgneng/ekho/master/src/README>).
- WAV export writes at `this->audio->outputSampleRate` with `SF_FORMAT_WAV | SF_FORMAT_PCM_16` (<https://raw.githubusercontent.com/hgneng/ekho/master/src/audio.cpp>).
- Published data rates (filename convention + voice-data listing): **16000 Hz and 44100 Hz** for Mandarin and Cantonese, 24000 Hz for Hakka (<https://sourceforge.net/projects/e-guidedog/files/Ekho-Voice-Data/0.2/>, <https://raw.githubusercontent.com/hgneng/ekho/master/ekho-data/pinyin/README>).
- A single canonical default output sample rate for the in-repo `pinyin.voice`: **not published** — the value lives in the binary header of `ekho-data/pinyin.index` (16,395 bytes) (<https://api.github.com/repos/hgneng/ekho/contents/ekho-data>).
- Note: `EkhoImpl::setSpeed()` contains the literals `44100` and `20362` in a Mandarin tempo-normalisation formula (`baseDelta = round(frames * 2 * 44100 * 100 / samplerate / 20362) - 100`) — a normalisation constant, not the output rate (<https://raw.githubusercontent.com/hgneng/ekho/master/src/libekho_impl.cpp>).

**Dependencies — mandatory vs. optional (answers "pure C++ with no runtime dependency": no)**

- `configure.ac`, **mandatory** (hard failure if missing):
  - libcurl — `PKG_CHECK_MODULES([CURL],[libcurl],[],[AC_MSG_ERROR([libcurl is required but not found...])])`
  - libsndfile — `AC_CHECK_LIB([sndfile],[main],[],[AC_MSG_FAILURE([sndfile test failed])])`
  - espeak-ng — `AC_CHECK_LIB([espeak-ng],[main],[],[AC_MSG_FAILURE([espeak-ng test failed])])`
  - pulseaudio — enabled unless `--without-pulseaudio`; failure is `AC_MSG_FAILURE([pulseaudio test failed (--without-pulseaudio to disable)])`
  (<https://raw.githubusercontent.com/hgneng/ekho/master/configure.ac>)
- **Optional**: sonic (`LIB_SONIC`, defines `HAVE_SONIC`), soxr (`--with-soxr`), mpg123 (`HAVE_MPG123`), mp3lame (`--with-mp3lame`), ncurses/Festival (`--enable-festival`), flite (`--enable-flite`, path hardcoded to `${srcdir}/flite/build/x86_64-darwin21.6.0/lib/*.a`), GTK2 (block commented out with `# don't use GTK any more`) (same URL).
- **libmusicxml is not a dependency**: both its `--enable-musicxml` block and its sources/libs in `Makefile.am` are commented out. Friso is likewise commented out (same URL, <https://raw.githubusercontent.com/hgneng/ekho/master/Makefile.am>).
- Mandatory linkage is unconditional in `Makefile.am`: `LIBEKHO_LIB = @LIB_PULSEAUDIO@ @LIB_FESTIVAL@ @LIB_FLITE@ @LIB_MP3LAME@ @LIB_MPG123@ @LIB_SONIC@ -lcurl -lsndfile -lespeak-ng @LIB_SOXR@`; `libekho_a_CXXFLAGS = ... -pthread -lcurl` (<https://raw.githubusercontent.com/hgneng/ekho/master/Makefile.am>).
- Additional runtime dependency found in code: `#include "utf8.h"` (utfcpp) with `utf8::next`, `utf8::not_enough_room`, `utf8::invalid_utf8` (<https://raw.githubusercontent.com/hgneng/ekho/master/src/libekho_impl.cpp>).
- `INSTALL` confirms the same set for Linux (`libsndfile1-dev`, `libpulse-dev`, `libcurl4-openssl-dev`, `libespeak-ng-dev`, `libutfcpp-dev`, `libmpg123-dev`, `libsonic-dev` "optionally required by change speed"), and for macOS (`brew install libsndfile pulseaudio utf8cpp espeak-ng sonic`) (<https://raw.githubusercontent.com/hgneng/ekho/master/INSTALL>).
- Packaging inconsistency: `Makefile.am`'s `dist-hook` copies `$(srcdir)/zhtts.tar.gz` and `$(srcdir)/ekho-data/piper`, neither of which appears in the master tree (<https://raw.githubusercontent.com/hgneng/ekho/master/Makefile.am> vs <https://api.github.com/repos/hgneng/ekho/git/trees/master?recursive=1>).

---

## 6. eSpeak-NG used directly (espeak-ng/espeak-ng)

| Item | Finding |
| --- | --- |
| Runtime dependency | Custom C, no ML runtime. Can be built as a shared library. |
| Prebuilt Android arm64 artifact | **None published** — release assets are APK/MSI/tarball only. Official Android build path exists in-tree. |
| Synthesis method | **Formant synthesis** (not diphone); MBROLA diphone voices optional |
| Code licence | GPL-3.0-or-later |
| Voice/weights licence | Same GPL-3.0; **no linking exception published** |

**Licence**

- `COPYING` is the verbatim **GNU General Public License Version 3, 29 June 2007** (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/COPYING>). GitHub detector: `spdx_id: GPL-3.0`, `key: gpl-3.0` (<https://api.github.com/repos/espeak-ng/espeak-ng>).
- README "License Information", verbatim: "eSpeak NG Text-to-Speech is released under the [GPL version 3](COPYING) or later license. The `getopt.c` compatibility implementation for getopt support on Windows is taken from the NetBSD `getopt_long` implementation, which is licensed under a [2-clause BSD](COPYING.BSD2) license." (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md>)
- `speak_lib.h` carries the same "version 3 of the License, or (at your option) any later version" header (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/speak_lib.h>).
- **No linking exception or library-specific licence statement is published.** The README's only library statement is that eSpeak NG is available as "a shared library version for use by other programs" and that "This C API is API and ABI compatible with espeak." The GPL-3.0 boilerplate in `COPYING` (which notes the GPL "does not permit incorporating your program into proprietary programs" and suggests LGPL instead) is FSF text, **not** an eSpeak-NG project statement (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/COPYING>).
- Repo metadata: created 2015-12-08T20:42:42Z, `pushed_at` 2026-09-09T15:53:17Z, default branch `master`, language `C`, 6,822 stars, 1,287 forks, 610 open issues, topics include `android` (<https://api.github.com/repos/espeak-ng/espeak-ng>). Project version `1.53.0` in `CMakeLists.txt` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/CMakeLists.txt>).

**Building `libespeak-ng` for Android arm64 — official support is in the main repo**

- **Yes.** `docs/building.md` has an `## Android` section: "The espeak-ng sources contain the code for the Android™ port of the application. This is published as the eSpeak for Android program on the Google Play store. It is based on the eyes-free port of eSpeak to the Android platform." Required tooling: Android Studio with API 34 support, the Android NDK, Gradle 8.13+, JDK 17. Build: `cd android && ./gradlew assembleRelease`, output `android/build/outputs/apk/release/espeak-release-unsigned.apk` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/building.md>).
- README platform table lists `Android | 4.0 |` (minimum version 4.0) (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md>).
- `android/jni/CMakeLists.txt` (project `espeak-android`) fetches and builds libsonic from source as a PIC OBJECT library because "the NDK sysroot has no sonic", pre-seeds `SONIC_LIB`/`SONIC_INC` for `cmake/deps.cmake`, then `add_subdirectory(../../ espeakng)`, and adds a custom target `espeak-data` that builds the `data` target and copies `espeak-ng-data` into `android/build/generated/espeak-ng-data`. It then builds **`add_library(ttsespeak SHARED jni/eSpeakService.c)`** linking `espeak-ng` and `log` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/jni/CMakeLists.txt>).
- `android/build.gradle`: `compileSdk = 36`, `ndkVersion = "29.0.14206865"`, `minSdk = 21`, `targetSdk = 36`, `versionCode = 23`, `versionName = "1.53.0"`, `namespace = "com.reecedunn.espeak"`, CMake `path = "jni/CMakeLists.txt"`, `version = "3.22.1"`, native arguments `-DUSE_ASYNC:BOOL=OFF -DUSE_MBROLA:BOOL=OFF -DUSE_LIBSONIC:BOOL=ON`, `targets = ["ttsespeak", "espeak-data"]`. **No `abiFilters` block is present** (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/build.gradle>).
- `android/CLAUDE.md` states: "CMake builds `libttsespeak.so` (JNI) + generates `espeak-ng-data/`"; the native build "disables `USE_ASYNC` and `USE_MBROLA` (not needed on Android) and enables `USE_LIBSONIC`"; architecture is `libespeak-ng (../../src/libespeak-ng/)` ← `eSpeakService.c` ← `SpeechSynthesis` ← `TtsService`; voice data is extracted from `res/raw/espeakdata.zip` to device-protected storage on first launch, validated against `version`, `intonations`, `phondata`, `phonindex`, `phontab`, `en_dict`. (Its stated `compileSdk 34 / targetSdk 33` conflicts with the values in `build.gradle` above.) (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/CLAUDE.md>)
- **There is no CMake target literally named `libespeak-ng`.** The library target is `add_library(espeak-ng common.c … espeak_api.c)` with `set_target_properties(espeak-ng PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR} VERSION ${PROJECT_VERSION})` and `install(TARGETS espeak-ng LIBRARY)`; the produced file is `libespeak-ng`, which is why `docs/building.md` tells users to set `LD_LIBRARY_PATH=build/src/libespeak-ng` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/libespeak-ng/CMakeLists.txt>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/building.md>).
- Shared vs. static is a build option, **default static**: `option(BUILD_SHARED_LIBS "Build shared libraries" OFF)` in the top-level `CMakeLists.txt`; `src/libespeak-ng/CMakeLists.txt` compiles with `-fPIC -fvisibility=hidden` and only wires `LIBESPEAK_NG_EXPORT` for Windows DLL export (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/CMakeLists.txt>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/libespeak-ng/CMakeLists.txt>).
- Cross-compilation note relevant to arm64, verbatim: "Because the eSpeak NG build process uses the built program to compile the language and voice data, you need to build it natively first. Once you have built it natively you can perform the cross compilation by pointing CMake to the native build: `cmake -Bbuild-cross -DCMAKE_TOOLCHAIN_FILE=... -DNativeBuild_DIR=build/src`". The top-level `CMakeLists.txt` emits `message(STATUS "Not building intonations as the build is a cross compile.")` when `CMAKE_CROSSCOMPILING` and no native binary is found (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/building.md>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/CMakeLists.txt>).
- **No prebuilt Android library binaries are published as release assets.** Assets found: for `1.52.0` (published 2024-12-12T13:47:27Z) — `espeak-1.52.0-signed.apk` (10,446,659 bytes, 25,338 downloads) and `espeak-ng.msi` (12,765,862 bytes); for `1.51` (2022-04-02) — `espeak-ng-1.51.tar.gz` (1,081,162 B), `espeak-ng-X64.msi` (5,999,185 B), `espeak-ng-X86.msi` (5,978,696 B), `espeak-release-signed.apk` (6,100,253 B). **No `.so`/`.aar`/prebuilt `libespeak-ng` for Android is published** (<https://api.github.com/repos/espeak-ng/espeak-ng/releases>).
- The 1.52.0 release notes include an "### android" section: "Added directBoot support -- beqabeqa473" and "Integrated ndk-build step into main gradle pipeline"; 1.51 notes include "Fix build scripts for Android (Peter Vágner, Minas Tirith Citizen)" (<https://api.github.com/repos/espeak-ng/espeak-ng/releases>).
- **Specific Android/NDK GitHub issue or PR numbers: not retrieved** — `api.github.com` returned HTTP 403 `API rate limit exceeded` for further calls. The repo has 610 open issues with no per-label breakdown retrievable unauthenticated (<https://api.github.com/repos/espeak-ng/espeak-ng>).

**Size of `espeak-ng-data`**

- **Not published in the project's own documentation.** The README says only: "Compact size. The program and its data, including many languages, totals about few Mbytes." — qualitative, no number (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md>).
- Measured from a distribution package (Debian 12 "bookworm", `espeak-ng-data` version `1.51+dfsg-10+deb12u2`) — package size / installed size: **arm64: 4,156.4 kB / 11,782.0 kB**; amd64: 4,156.1 / 11,782.0 kB; armhf 4,155.5/11,782.0; i386 4,156.3/11,782.0; ppc64el 4,156.1/11,782.0; s390x 4,155.0/11,782.0. Package description: "This package contains necessary synthesizer data files needed for the espeak-ng program and the shared library." (<https://packages.debian.org/bookworm/espeak-ng-data>)
- The Android app ships this data as a zip in APK resources: `res/raw/espeakdata.zip`, produced by the `createDataArchive` Gradle task from `build/generated/espeak-ng-data/`, with a SHA-256 written to `res/raw/espeakdata_version` and extracted to device-protected storage at first launch (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/build.gradle>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/CLAUDE.md>).
- A **Mandarin-only subset size is not published**.
- Related figure from sherpa-onnx: their shared `espeak-ng-data.tar.bz2` is **7,252,012 bytes** (<https://api.github.com/repos/k2-fsa/sherpa-onnx/releases/tags/tts-models>).

**Chinese (Mandarin, `cmn`) front-end**

- The language file is at **`espeak-ng-data/lang/sit/cmn`** (grouped by ISO 639-5 family), **not** `espeak-ng-data/lang/zh/cmn` — the latter returns HTTP 404 (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/lang/zh/cmn>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/lang/sit/cmn>).
- Contents of `espeak-ng-data/lang/sit/cmn`, key lines verbatim:
  ```
  name Chinese (Mandarin, latin as English)
  language cmn
  language zh-cmn
  language zh

  phonemes cmn
  dictionary cmn
  words 1
  pitch 80 118

  dict_min 100000
  ```
  (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/lang/sit/cmn>)
- `docs/languages.md` entry: `| sit | cmn | Sino-Tibetan | Chinese | Mandarin |` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages.md>).
- Dictionary sources that exist in `dictsource/`:
  - **`dictsource/cmn_rules`** — the pinyin→phoneme rule file. Header comment: "Default is to handle latin characters as pinyin", with a commented-out `?1: speak latin characters as English words`. It contains `.replace` entries for tone diacritics (ā á ǎ à ō ó … ǖ ǘ ǚ ǜ) and pinyin variants (ŋ→ng, ɑ→a, ɡ→g, ẑ→zh, ĉ→ch, ŝ→sh), letter groups `.L01 j q x y`, `.L02 1 2 3 4 5 // tone number`, `.L03 a o e i u v ai ei ui ao ou iu ie ve er an en in un vn ang eng ing ong ua ue uo uai uan uang ia iao ian iang`, per-initial `.group` blocks (b→p, c→tsh, ch→ts.h, d→t, g→k, h→X, j→tS;, k→kh, p→ph, q→tS;h, r→z., s→s, sh→s., t→th, x→S;, z→ts, zh→ts.), and a tone-number group `1 55 / 2 35 / 3 214 / 4 51 / 5 11` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_rules>).
  - **`dictsource/cmn_list`** — the main exceptions/numbers list. Contains `_?? @:11 // unrecognized character`, number readings (`_0 liN35`, `_1 ji55`, `_2 @r51`, `_3 san55`, `_4 si[51`, `_5 wu214`, `_6 liou51`, `_7 tS;hi55`, `_8 pA55`, `_9 tS;iou214`, `_0M1 tS;h'iE55n_| // 1,000`, `_0M2 w'A51n_| // 10,000`), bopomofo letter readings (`ㄅ po55` … `ㄦ @r55`), then `$textmode` followed by character readings. Header comment: "Most frequent pronunciations of the 3799 most common characters (from Unihan database ftp://ftp.unicode.org/Public/UNIDATA/Unihan.zip, kHanyuPinlu field with some corrections)" (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_list>).
  - **`dictsource/extra/cmn_listx`** — the **extended** Mandarin dictionary (i.e. yes, an extended pinyin dictionary exists). Header comment, verbatim: "From Unihan database ftp://ftp.unicode.org/Public/UNIDATA/Unihan.zip kMandarin entries (except the ones that have kHanyuPinlu, which are in zh_list) with compounds from CC-CEDICT http://www.mdbg.net/chindict/chindict.php?page=cedict and some corrections — **21611 single characters plus 36500 compound exceptions** (includes 320 added 'yi' and 10721 added 'bu' exceptions, and 9700 extra 2-syllable words for 3rd-tone sandhi blocking)". Format is `character<TAB>pinyin-with-tone-number`, e.g. `〇 ling2`, `㐀 qiu1`, `涉 she4` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/extra/cmn_listx>).
- **A separate dictionary/data build step is required.** `docs/building.md`, verbatim: "The data (language dictionaries, phoneme tables, intonation) can be built with: `cmake --build build --target data`", and "Specific languages can be compiled by running the built binary directly: `ESPEAK_DATA_PATH=\`pwd\`/build build/src/espeak-ng --compile=LANG` where LANG is the language code of the given language (e.g. `fr` for French)." (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/building.md>)
- There is also a library API for this, quoted from the header: `ESPEAK_API void espeak_CompileDictionary(const char *path, FILE *log, int flags);` — "Compile pronunciation dictionary for a language which corresponds to the currently selected voice. The required voice should be selected before calling this function. path: The directory which contains the language's `_rules` and `_list` files. `path` should end with a path separator character ('/')." Plus the NG form `ESPEAK_NG_API espeak_ng_STATUS espeak_ng_CompileDictionary(const char *dsource, const char *dict_name, FILE *log, int flags, espeak_ng_ERROR_CONTEXT *context);` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/speak_lib.h>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/espeak_ng.h>).
- Extended dictionaries are **on by default** and toggled per language: `docs/building.md` lists `-DEXTRA_ru` (Russian, ON), **`-DEXTRA_cmn` (Mandarin Chinese, ON)**, `-DEXTRA_yue` (Cantonese, ON), with the note "The extended dictionaries are taken from http://espeak.sourceforge.net/data/ and provide better coverage for those languages, while increasing the resulting dictionary size." (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/building.md>)
- `docs/index.md` lists **no Mandarin-specific documentation page**; the only language-specific pronunciation guide published is English (`docs/languages/gmw/en.md`), plus a Cherokee implementation note (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/index.md>).
- Historical naming, from the release notes: 1.51 — "Rename zh to cmn (Mandarin)" and "Rename zhy to yue (Cantonese)", and "cmn (Mandarin) now assumes all latin characters all English text — Use cmn-latn-pinyin for interpreting latin characters as pinyin"; 1.52.0 lists "cmn (Mandarin) -- Cameron Wong" among updated languages (<https://api.github.com/repos/espeak-ng/espeak-ng/releases>).

**Getting raw PCM programmatically — exact signatures**

All quoted from `src/include/espeak-ng/speak_lib.h` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/speak_lib.h>):

- Initialisation (note: the parameter is named `buflength`, **not** `outbuf`):
  ```c
  ESPEAK_API int espeak_Initialize(espeak_AUDIO_OUTPUT output, int buflength, const char *path, int options);
  ```
  Comment block: "Must be called before any synthesis functions are called. output: the audio data can either be played by eSpeak or passed back by the SynthCallback function. **buflength: The length in mS of sound buffers passed to the SynthCallback function. Value=0 gives a default of 60mS. This parameter is only used for AUDIO_OUTPUT_RETRIEVAL and AUDIO_OUTPUT_SYNCHRONOUS modes.** path: The directory which contains the espeak-ng-data directory, or NULL for the default location. options: bit 0: 1=allow espeakEVENT_PHONEME events … bit 15: 1=don't exit if espeak_data is not found (used for --help). **Returns: sample rate in Hz, or -1 (EE_INTERNAL_ERROR).**"
- Output modes, verbatim:
  ```c
  typedef enum {
      /* PLAYBACK mode: plays the audio data, supplies events to the calling program*/
      AUDIO_OUTPUT_PLAYBACK,
      /* RETRIEVAL mode: supplies audio data and events to the calling program */
      AUDIO_OUTPUT_RETRIEVAL,
      /* SYNCHRONOUS mode: as RETRIEVAL but doesn't return until synthesis is completed */
      AUDIO_OUTPUT_SYNCHRONOUS,
      /* Synchronous playback */
      AUDIO_OUTPUT_SYNCH_PLAYBACK
  } espeak_AUDIO_OUTPUT;
  ```
- Callback type and setter (the name is **`espeak_SetSynthCallback`**):
  ```c
  typedef int (t_espeak_callback)(short*, int, espeak_EVENT*);

  ESPEAK_API void espeak_SetSynthCallback(t_espeak_callback* SynthCallback);
  ```
  Comment: "…The callback function is of the form: `int SynthCallback(short *wav, int numsamples, espeak_EVENT *events);` wav: is the speech sound data which has been produced. **NULL indicates that the synthesis has been completed.** numsamples: is the number of entries in wav. This number may vary, may be less than the value implied by the buflength parameter given in espeak_Initialize, and **may sometimes be zero (which does NOT indicate end of synthesis)**. events: an array of espeak_EVENT items … The list of events is terminated by an event of type = 0. Callback returns: 0=continue synthesis, 1=abort synthesis."
- **There is no `espeak_ng_SetSynthCallback`.** A full-text search of both public headers (`speak_lib.h`, `espeak_ng.h`) shows the only callback setters are `espeak_SetSynthCallback`, `espeak_SetUriCallback`, `espeak_SetPhonemeTrace`, plus — new in the NG API — `ESPEAK_NG_API espeak_ng_STATUS espeak_ng_SetOutputHooks(espeak_ng_OUTPUT_HOOKS* hooks);` where `espeak_ng_OUTPUT_HOOKS` is `{ void (*outputPhoSymbol)(char* pho_code,int pho_type); void (*outputSilence)(short echo_tail); void (*outputVoiced)(short sample); void (*outputUnvoiced)(short sample); }` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/espeak_ng.h>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/speak_lib.h>).
- Synthesis call, verbatim:
  ```c
  ESPEAK_API espeak_ERROR espeak_Synth(const void *text,
      size_t size,
      unsigned int position,
      espeak_POSITION_TYPE position_type,
      unsigned int end_position,
      unsigned int flags,
      unsigned int* unique_identifier,
      void* user_data);
  ```
  Comment: "Synthesize speech for the specified text. The speech sound data is passed to the calling program in buffers by means of the callback function specified by espeak_SetSynthCallback(). **The command is asynchronous: it is internally buffered and returns as soon as possible.** … flags: … espeakCHARS_UTF8 … espeakSSML … espeakPHONEMES … espeakENDPAUSE … Return: EE_OK … EE_BUFFER_FULL … EE_INTERNAL_ERROR." Also relevant: `espeak_Cancel`, `espeak_Synchronize` ("This function returns when all data have been spoken"), `espeak_Terminate`.
- The reference Android JNI bridge demonstrates the exact working pattern in-tree: `espeak_SetSynthCallback(SynthCallback)` immediately before `espeak_Synth(c_text, strlen(c_text), 0, POS_CHARACTER, 0, isSsml ? espeakCHARS_UTF8 | espeakSSML : espeakCHARS_UTF8, &unique_identifier, object)` then `espeak_Synchronize()`, with `espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, BUFFER_SIZE_IN_MILLISECONDS /* 300 */, c_path, 0)`; the callback copies `numSamples * 2` bytes of **S16LE mono** into a Java byte array. The file also comments that "espeak_ng_Cancel() cannot interrupt a synthesis in progress -- its body is entirely `#if USE_ASYNC`, which this build disables -- so returning SYNTH_ABORT from the callback is the only way to end one early", and that "espeak marks the end of the request with a NULL buffer, not with a zero sample count" (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/jni/jni/eSpeakService.c>).
- The minimal library example in `docs/integration.md`: `espeak_Initialize(output, buflength, path, options); espeak_SetVoiceByName(voicename); espeak_Synth(text, buflength, position, position_type, end_position, flags, identifier, user_data);` compiled with `gcc test-espeak.c -lespeak-ng -o test-espeak`; the header to include is `<espeak-ng/speak_lib.h>` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/integration.md>).

**Output sample rate**

- `src/libespeak-ng/speech.c` contains both the runtime default and the initialisation default:
  - `static int voice_samplerate = 22050;`
  - in `espeak_ng_Initialize`: `int srate = 22050; // default sample rate 22050 Hz`, then `LoadPhData(&srate, context)` and `WavegenInit(srate, 0)` — i.e. the phoneme data can override the initial 22050.
  - `espeak_ng_GetSampleRate(void)` returns the module-level `samplerate`.
  - `dispatch_audio()` handles an `espeakEVENT_SAMPLERATE` event with `voice_samplerate = event->id.number;`, reopening the audio device when the rate changes — the rate is per-voice and can change at runtime.
  (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/libespeak-ng/speech.c>)
- `espeak_Initialize` returns "sample rate in Hz, or -1 (EE_INTERNAL_ERROR)" (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/speak_lib.h>). The NG accessor is `ESPEAK_NG_API int espeak_ng_GetSampleRate(void);` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/espeak_ng.h>).
- `espeak_GetCurrentVoice` is documented as `ESPEAK_API espeak_VOICE *espeak_GetCurrentVoice(void);` — "Returns the espeak_VOICE data for the currently selected voice. This is not affected by temporary voice changes caused by SSML elements such as `<voice>` and `<s>`" (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/speak_lib.h>). NG equivalents: `espeak_ng_SetVoiceByName`, `espeak_ng_SetVoiceByFile`, `espeak_ng_SetVoiceByProperties` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/espeak_ng.h>).
- `espeakEVENT_SAMPLERATE = 8` is defined in the event-type enum (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/speak_lib.h>).
- Default rate constants: `#define espeakRATE_MINIMUM 80`, `#define espeakRATE_MAXIMUM 450`, `#define espeakRATE_NORMAL 175`; default buffer floor `static const int min_buffer_length = 60; // minimum buffer length in ms`, and the 1.49.1 release notes mention "Reduce the default buffer length to 60mS to improve latency" (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/speak_lib.h>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/libespeak-ng/speech.c>, <https://api.github.com/repos/espeak-ng/espeak-ng/releases>).

**Synthesis method and what the docs say about voice quality**

- Method is **formant synthesis**, not diphone concatenation for the native voices. README, verbatim: "eSpeak NG uses a 'formant synthesis' method. This allows many languages to be provided in a small size. **The speech is clear, and can be used at high speeds, but is not as natural or smooth as larger synthesizers which are based on human speech recordings.** It also supports Klatt formant synthesis, and the ability to use MBROLA as backend speech synthesizer." (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md>)
- Diphone synthesis is available only via the optional MBROLA backend, described as a front-end role: "Can be used as a front-end to MBROLA diphone voices. eSpeak NG converts text to phonemes with pitch and length information." / "Can translate text into phoneme codes, so it could be adapted as a front end for another speech synthesis engine." (same URL)
- Klatt support is a build option, on by default: `-DUSE_KLATT` "Enable Klatt formant synthesis. Default ON", `-DUSE_SPEECHPLAYER` "Enable the speechPlayer Klatt implementation. Default ON", `-DUSE_MBROLA` "Enable MBROLA voice support. Default ON (if available)" — these select `klatt.c` and `sPlayer.c` sources in CMake (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/building.md>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/libespeak-ng/CMakeLists.txt>).
- **No Mandarin-specific quality statement is published.** `docs/languages.md` gives only the family/language mapping for `cmn` with no quality note; `docs/index.md` lists no Mandarin guide; the README's quality sentence is language-agnostic and its only named-language caveat about incompleteness is the general "Potential for other languages. Several are included in varying stages of progress." (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages.md>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/index.md>, <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md>)

**Explicit gaps for eSpeak-NG (report as "not published")**

- `espeak-ng-data` size in the project's own documentation — not published (measured figures only via the Debian package).
- Specific Android/NDK GitHub issue or PR numbers — not retrieved (API rate limit).
- Mandarin-specific voice-quality statement — not published.
- Prebuilt Android `.so`/`.aar` release assets — none published.
- A CMake target literally named `libespeak-ng` — does not exist; the target is `espeak-ng` and it produces `libespeak-ng`.

---

## 7. Other credible pure-C/C++ offline Mandarin TTS usable on Android arm64

### 7.1 sherpa-onnx (k2-fsa/sherpa-onnx) — OPEN SOURCE, Apache-2.0 code

| Item | Finding |
| --- | --- |
| Runtime dependency | ONNX Runtime (C/C++). No Python at runtime. |
| Prebuilt Android arm64 artifact | Yes — `.aar` and NDK tarballs with exact published byte sizes |
| Chinese models | VITS (several), Matcha-TTS, Kokoro, Piper zh_CN, ZipVoice |
| Code licence | Apache-2.0 |
| Weights licence | **Per model**; `matcha-icefall-zh-baker` is explicitly non-commercial-by-dataset |
| Python at runtime | No for the shipped models (Python only if you re-export) |

- Language is C++; description: "…using next-gen Kaldi with onnxruntime without Internet connection. Support embedded systems, Android, iOS, HarmonyOS, Raspberry Pi…"; topics include `android`, `cpp`, `onnx`, `vits` (<https://api.github.com/repos/k2-fsa/sherpa-onnx>).
- Code licence **Apache-2.0** (<https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/master/LICENSE>; API field `"spdx_id":"Apache-2.0"`).
- **C API usable from Qt with no Java**: header `sherpa-onnx/c-api/c-api.h`, implementation `sherpa-onnx/c-api/c-api.cc` (<https://k2-fsa.github.io/sherpa/onnx/c-api/index.html>). Documented build flags `-DSHERPA_ONNX_ENABLE_C_API=ON`, shared or static; the static build ships `libsherpa-onnx-c-api.a`, `libsherpa-onnx-cxx-api.a`, `libonnxruntime.a`, `libespeak-ng.a`, `libpiper_phonemize.a` (same URL).
- C++ API mirror: `#include "sherpa-onnx/c-api/cxx-api.h"`, `OfflineTts::Create`, `tts.Generate`, `WriteWave` (<https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-xiao_ya-medium.html>).

**Prebuilt Android arm64 artifacts with published sizes** (release `v1.13.7`, published 2026-09-01) — <https://github.com/k2-fsa/sherpa-onnx/releases/tag/v1.13.7>

| Asset | Exact size (bytes) |
| --- | --- |
| `sherpa-onnx-1.13.7.aar` | 49,113,869 |
| `sherpa-onnx-static-link-onnxruntime-1.13.7.aar` | 37,809,521 |
| `sherpa-onnx-1.13.7-rknn.aar` | 26,121,272 |
| `sherpa-onnx-v1.13.7-android.tar.bz2` | 45,287,000 |
| `sherpa-onnx-v1.13.7-android-static-link-onnxruntime.tar.bz2` | 34,346,901 |
| `sherpa-onnx-v1.13.7-android-aarch64-termux-shared.tar.bz2` | 16,171,946 |

- The AAR bundles the JNI libs, but JNI is only needed for the Java/Kotlin API; the `.aar` and NDK tarballs also expose the C/C++ API surface (<https://k2-fsa.github.io/sherpa/onnx/android/index.html>; sub-pages <https://k2-fsa.github.io/sherpa/onnx/android/build-sherpa-onnx.html>, <https://k2-fsa.github.io/sherpa/onnx/android/prebuilt-apk.html>).
- Per-model **Android TTS Engine APKs** are published for arm64-v8a, armeabi-v7a, x86_64 and x86 at v1.13.7, e.g. `sherpa-onnx-1.13.7-arm64-v8a-zho-tts-engine-vits-piper-zh_CN-xiao_ya-medium.apk` (<https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-xiao_ya-medium.html>); source at <https://github.com/k2-fsa/sherpa-onnx/tree/master/android/SherpaOnnxTtsEngine>.

**Chinese model catalogue — published sizes**

From the vits table (`Model filesize (MB)`, `Sample rate (Hz)`) — <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html>:

| Model | Language | Speakers | Published model size | Sample rate |
| --- | --- | --- | --- | --- |
| `vits-melo-tts-zh_en` | Chinese + English | 1 | 163 MB | 44100 Hz |
| `sherpa-onnx-vits-zh-ll` | Chinese | 5 | 115 MB | 16000 Hz |
| `vits-zh-hf-fanchen-C` | Chinese | 187 | 116 MB | 16000 Hz |
| `vits-zh-hf-fanchen-wnj` | Chinese | 1 (male) | 116 MB | 16000 Hz |
| `vits-zh-hf-theresa` | Chinese | 804 | 117 MB | 22050 Hz |
| `vits-zh-hf-eula` | Chinese | 804 | 117 MB | 22050 Hz |
| `vits-model-aishell3` | Chinese | 174 | 116 MB *(see discrepancy below)* | 8000 Hz |

- **Documentation discrepancy, reported as found**: the vits summary table lists `aishell3` at **116 MB**, while the same page's detail section for `vits-icefall-zh-aishell3` shows `model.onnx` at **29M** and the RTF table lists it at **30 MB**. The `tts-models` release asset `vits-icefall-zh-aishell3.tar.bz2` is **31,559,701 bytes**. Upstream page: <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html> and <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html> and <https://github.com/k2-fsa/sherpa-onnx/releases/tag/tts-models>.
- `tts-models` release tarball sizes (bytes, from the release asset API, <https://api.github.com/repos/k2-fsa/sherpa-onnx/releases/tags/tts-models>): `vits-icefall-zh-aishell3.tar.bz2` 31,559,701; `vits-zh-aishell3.tar.bz2` 146,922,607; `vits-zh-hf-fanchen-C.tar.bz2` 119,326,431; `vits-zh-hf-eula.tar.bz2` 120,562,119; `vits-zh-hf-theresa.tar.bz2` 120,596,617; `vits-cantonese-hf-xiaomaiiwn.tar.bz2` 107,995,442 (Cantonese, not Mandarin); `espeak-ng-data.tar.bz2` **7,252,012** (shared by all piper-derived models). Tarball sizes for `vits-melo-tts-zh_en`, `matcha-icefall-zh-baker`, `kokoro-multi-lang-v1_0`/`v1_1`, `kokoro-int8-multi-lang-v1_1`: **not published on the pages retrieved** (the release asset listing was truncated); the inner file sizes are published instead.
- `matcha-icefall-zh-baker` (Chinese, 1 female speaker, from `icefall` egs `baker_zh`): `model-steps-3.onnx` **72M**, `lexicon.txt` 1.3M, `tokens.txt` 19K, `date.fst` 58K, `number.fst` 63K, `phone.fst` 87K, plus `dict/`; **requires a separate vocoder** `vocos-22khz-univ.onnx` **51M** (alternatives `hifigan_v1/v2/v3.onnx`, sizes not published); output 22050 Hz (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html>).
- **Matcha caveat**: "Models are from icefall. **We don't support models from https://github.com/shivammehta25/Matcha-TTS.**" (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html>)
- Kokoro models in sherpa-onnx: `kokoro-multi-lang-v1_0` `model.onnx` **310M** + `voices.bin` **26M**, sample rate fixed 24000 Hz (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html>); `kokoro-multi-lang-v1_1` at **311 MB** in the RTF table, with an `int8` archive `kokoro-int8-multi-lang-v1_1.tar.bz2` also published (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html>).
- Piper Chinese in sherpa-onnx: `vits-piper-zh_CN-chaowen-medium` and `vits-piper-zh_CN-xiao_ya-medium`, converted from `rhasspy/piper-voices` (`zh/zh_CN/xiao_ya/medium`); `vits-piper-zh_CN-xiao_ya-medium` is 1 speaker, 22050 Hz; per-file ONNX size **not published on that page** (<https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/index.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-xiao_ya-medium.html>).
- **ZipVoice** (zero-shot voice cloning, zh+en): `sherpa-onnx-zipvoice-distill-int8-zh-en-emilia.tar.bz2` with `encoder.int8.onnx` + `decoder.int8.onnx`, plus required vocoder `vocos_24khz.onnx`; **requires both `--reference-audio` and `--reference-text`** (reference text must exactly match the reference audio); `--num-steps` trades quality vs. speed. File sizes **not published on that page** (<https://k2-fsa.github.io/sherpa/onnx/tts/zipvoice.html>).
- Quantisation summary: `int8` is confirmed published for `kokoro-int8-multi-lang-v1_1`, for ZipVoice encoder/decoder, and for English VCTK (`vits-vctk.int8.onnx` 37M vs `vits-vctk.onnx` 116M). **fp16 variants, and int8 variants for the Chinese VITS models, are not published** (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/zipvoice.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html>).

**Python export step — per model, precise**

- `vits-melo-tts-zh_en`, `matcha-icefall-zh-baker`, the `vits-zh-hf-*` family, `aishell3`, `vits-icefall-zh-aishell3`, `kokoro-*`: **no export needed — shipped pre-converted**; the docs show no "please use the following commands to export" block on those pages (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html>).
- The MeloTTS-Chinese conversion is documented as: the model "is converted from https://huggingface.co/myshell-ai/MeloTTS-Chinese"; "The converting script is available at https://github.com/k2-fsa/sherpa-onnx/tree/master/scripts/melo-tts"; "You can convert more models from https://github.com/myshell-ai/MeloTTS by yourself" (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html>).
- Kokoro self-conversion (optional): <https://github.com/k2-fsa/sherpa-onnx/blob/master/scripts/kokoro/v1.0/run.sh> (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html>).
- Piper: the docs describe a Python conversion procedure (pin `onnx==1.17.0`, `onnxruntime==1.17.1`; add `metadata_props`; emit `tokens.txt`) but state "you **only need to download the converted models**" — the Python step is **optional, not required** (<https://k2-fsa.github.io/sherpa/onnx/tts/piper.html>).

**Chinese text front-end**

- Per-model normalisation uses FST rule files passed via `--tts-rule-fsts` / `rule_fsts`: `phone.fst` (polyphone/phoneme selection), `date.fst`, `number.fst`. Documented example: `--tts-rule-fsts=./matcha-icefall-zh-baker/phone.fst,./matcha-icefall-zh-baker/date.fst,./matcha-icefall-zh-baker/number.fst` (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html>); Kokoro uses `date-zh.fst`, `number-zh.fst`, `phone-zh.fst` (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html>).
- Lexicon/token inputs per family: Chinese VITS `--vits-lexicon=lexicon.txt`; Matcha `--matcha-lexicon=lexicon.txt`; Kokoro `--kokoro-lexicon=lexicon-us-en.txt,lexicon-zh.txt` (same pages).
- **espeak-ng dependency per model**: models whose commands pass a `*-data-dir` require `espeak-ng-data` — `vits-piper-en_US-glados`, `vits-piper-en_US-libritts_r-medium`, `matcha-icefall-en_US-ljspeech`, `kokoro-multi-lang-v1_0`, ZipVoice. By contrast, the documented commands for **`vits-melo-tts-zh_en`, `matcha-icefall-zh-baker` and `vits-piper-zh_CN-xiao_ya-medium` pass lexicon + tokens (+ FSTs) and no data-dir** (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-xiao_ya-medium.html>). The Piper page states `espeak-ng-data.tar.bz2` "is shared by all models from piper, no matter which language" (<https://k2-fsa.github.io/sherpa/onnx/tts/piper.html>).
- **jieba**: jieba appears on the Chinese homophone path — the official debug output shows `homophone-replacer.cc:Apply:165 After jieba: 下面_是_一个_测试_悬界_芯片…` (<https://k2-fsa.github.io/sherpa/onnx/homophone-replacer/index.html>). However PR **#2664 "Remove cppjieba"** (closed, merged 2025-10-10, merge commit `8568fc4e5d92be1562f1946c2af90d96355ec117`) removed the third-party segmentation dependency and the `dict-dir` wiring, so "TTS and homophone replacement now use a character-based lexicon" (<https://github.com/k2-fsa/sherpa-onnx/pull/2664>, <https://api.github.com/repos/k2-fsa/sherpa-onnx/pulls/2664>). Model tarballs referenced by the docs still contain a `dict/` directory.
- **Polyphone control that exists in-tree**: the 拼音词组匹配替换 (homophone replacer) maps pinyin-with-tone rules (e.g. `xuan2 jie4 xin1 pian4` → `玄戒芯片`, `fu2nan2ren2` → `湖南人`) to character strings via a `replace.fst` generated with `pynini`; runtime flags `--hr-lexicon=lexicon.txt` and `--hr-rule-fsts=replace.fst` (`HomophoneReplacerConfig.hr` in the C/C++ API); it **only replaces Chinese characters** and requires the rule file to be pre-generated (<https://k2-fsa.github.io/sherpa/onnx/homophone-replacer/index.html>). Lexicon download: <https://github.com/k2-fsa/sherpa-onnx/releases/tag/hr-files>.
- OOV handling is documented as "add them to `lexicon.txt`" (<https://k2-fsa.github.io/sherpa/onnx/tts/faq.html>).

**Published RTF — hardware explicitly named: Raspberry Pi 4 Model B Rev 1.5** (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html>; threads 1/2/3/4, with the table's size column)

| Model | 1 | 2 | 3 | 4 | Size |
| --- | --- | --- | --- | --- | --- |
| `aishell3` (Chinese) | **0.365** | 0.220 | 0.171 | 0.156 | 30 MB |
| `matcha-icefall-zh-baker` (Chinese) | **0.892** | 0.536 | 0.432 | 0.391 | 73 MB |
| `vits-melo-tts-zh_en` | 6.727 | 3.877 | 2.914 | 2.518 | 163 MB |
| `sherpa-onnx-vits-zh-ll` | 4.275 | 2.494 | 1.840 | 1.593 | 116 MB |
| `vits-zh-hf-fanchen-C` | 4.306 | 2.451 | 1.846 | 1.600 | 116 MB |
| `vits-zh-hf-fanchen-wnj` | 4.276 | 2.505 | 1.827 | 1.608 | 116 MB |
| `vits-zh-hf-theresa` | 6.032 | 3.448 | 2.566 | 2.210 | 117 MB |
| `vits-zh-hf-eula` | 6.011 | 3.473 | 2.537 | 2.231 | 117 MB |
| `kokoro-multi-lang-v1_1` | 7.635 | 4.470 | 3.430 | 3.191 | 311 MB |
| `kokoro-en-v0_19` (English only) | 6.629 | 3.870 | 2.999 | 2.774 | 330 MB |

- **No published arm64 Android RTF** was found in the docs — only Raspberry Pi 4 numbers. Android-specific RTF: **not published**.

**Output format / sample rate**

- API output is `GeneratedAudio` with `float* samples` + `sample_rate`; helper `SherpaOnnxWriteWave` emits 16-bit signed PCM WAV, mono (`Channels: 1`, `Sample Encoding: 16-bit Signed Integer PCM`) (<https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-xiao_ya-medium.html>).
- Sample rates: melo-zh_en 44100; matcha-icefall-zh-baker 22050; kokoro-* 24000; vits-zh-fanchen-C / vits-zh-ll 16000; vits-zh-hf-theresa/eula 22050; aishell3 8000; piper zh_CN-xiao_ya-medium 22050 (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-xiao_ya-medium.html>).

**Reported Chinese pronunciation / text problems (issues)** — searched via <https://api.github.com/search/issues?q=repo%3Ak2-fsa%2Fsherpa-onnx+%E5%A4%9A%E9%9F%B3%E5%AD%97> (returned `"total_count":37`)

- **#2726** — "kokoro-multi-lang-v1_1 模型会把中文句号读成 dot" (reads the Chinese full stop as the English word "dot") — <https://github.com/k2-fsa/sherpa-onnx/issues/2726>
- **#2904** — "matcha-icefall-zh-en 在合成语音时会概率性出错的问题补充" — reported on HUAWEI Kunpeng 920 aarch64/Ubuntu 22.04 and Intel Xeon W-2133/Ubuntu 20.04 with sherpa-onnx 1.12.18, threads 1/4/16; examples include a dropped 矩 character and 方案 rendering as 方方 — <https://github.com/k2-fsa/sherpa-onnx/issues/2904> (predecessor <https://github.com/k2-fsa/sherpa-onnx/issues/2902>)
- **#2961** — "拼音词组替换能力偶尔出现丢字的问题" — reporter suspects word segmentation: "我怀疑是分词导致的问题，因为我把 lexicon.txt 中大于一个字的词组和成语都删除掉之后…就不会出现丢字的问题了" — <https://github.com/k2-fsa/sherpa-onnx/issues/2961>
- **#1552** — "vits-melo-tts-zh_en 模型疑问" — "英文发音并不好，很多单词不会发音，看起来和 cum 字典有关" — <https://github.com/k2-fsa/sherpa-onnx/issues/1552>
- **#2004** — "Incorrect phoneme handling (Kokoro-TTS)" — <https://github.com/k2-fsa/sherpa-onnx/issues/2004>
- **#2325** — with `matcha-icefall-zh-baker` the played ALSA stream stops ~2 sentences early while the saved WAV is complete — <https://github.com/k2-fsa/sherpa-onnx/issues/2325>
- **#468** — Chinese models misbehaving (encoding) on Windows, referenced from the official TTS FAQ — <https://github.com/k2-fsa/sherpa-onnx/issues/468>, <https://k2-fsa.github.io/sherpa/onnx/tts/faq.html>
- No issue dedicated to Mandarin **tone (声调)** errors specifically was found in the searches run → **not found**.

### 7.2 CosyVoice (FunAudioLLM/CosyVoice) — OPEN SOURCE code (Apache-2.0), PyTorch, autoregressive, **no ONNX/C++/Android path**

- LLM-based and PyTorch: "Fun-CosyVoice 3.0 is an advanced text-to-speech (TTS) system based on large language models (LLM)"; install is conda + `pip install -r requirements.txt` with `python=3.10`; documented runtime paths are vLLM (≥0.11.x / 0.9.0), Docker with the NVIDIA runtime, and NVIDIA TensorRT-LLM (`runtime/triton_trtllm`, "4x acceleration comparing with huggingface transformers implementation") (<https://raw.githubusercontent.com/FunAudioLLM/CosyVoice/main/README.md>).
- Code licence: **Apache-2.0** (<https://raw.githubusercontent.com/FunAudioLLM/CosyVoice/main/LICENSE>). Separate weights terms: the README Disclaimer says "The content provided above is for academic purposes only and is intended to demonstrate technical capabilities."; the weights licence text itself was **not retrieved** → not published (<https://raw.githubusercontent.com/FunAudioLLM/CosyVoice/main/README.md>).
- Model sizes (parameter counts published; on-disk byte sizes **not published** on the pages retrieved): `Fun-CosyVoice3-0.5B-2512` 0.5B, `CosyVoice2-0.5B` 0.5B, `CosyVoice-300M` / `-300M-SFT` / `-300M-Instruct` (<https://huggingface.co/FunAudioLLM/Fun-CosyVoice3-0.5B-2512>, <https://huggingface.co/FunAudioLLM/CosyVoice2-0.5B>, <https://huggingface.co/FunAudioLLM/CosyVoice-300M>, <https://www.modelscope.cn/models/iic/CosyVoice-300M>).
- Published latency: "Bi-Streaming … achieves latency as low as 150ms" (**hardware not stated on that line**); CER table in the same README (test-zh CER 1.45 % for CosyVoice2, 1.21 % for Fun-CosyVoice3-0.5B-2512); hardware for those numbers: **not published** (<https://raw.githubusercontent.com/FunAudioLLM/CosyVoice/main/README.md>).
- ONNX / C++ / Android: the README documents **no ONNX export, no C++ runtime, no Android/iOS path** — deployment options are Python, gRPC/FastAPI Docker, vLLM, TensorRT-LLM (same URL).
- **sherpa-onnx does NOT support CosyVoice**: the official TTS index lists All-in-one, KittenTTS, PocketTTS, SupertonicTTS, ZipVoice, Pre-trained models (Matcha/Kokoro/KittenTTS/vits), WebAssembly, Piper, MMS, FAQ — no CosyVoice page or mention (<https://k2-fsa.github.io/sherpa/onnx/tts/index.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/index.html>).

### 7.3 Matcha-TTS and Vocos

- **Matcha-TTS** (<https://github.com/shivammehta25/Matcha-TTS>): code licence **MIT** (<https://raw.githubusercontent.com/shivammehta25/Matcha-TTS/main/LICENSE>). **Non-autoregressive**, verbatim from the paper abstract: "The method is probabilistic, **non-autoregressive**, and learns to speak from scratch without external alignments."; "capable of high output quality in fewer synthesis steps"; "has the smallest memory footprint, rivals the speed of the fastest models on long utterances" (<https://arxiv.org/abs/2309.03199>, ICASSP 2024). Specific RTF numbers/hardware: **not published in the abstract**. Upstream checkpoint (weights) licence: **not published** on the retrieved pages. Chinese support upstream: **not published** in the sources retrieved.
- The Android-relevant Matcha path is sherpa-onnx's `matcha-icefall-zh-baker`, and it **explicitly excludes the upstream repo's models** (quote in §7.1). Weights caveat: "The dataset is for `non-commercial` use only." (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html>)
- **Vocos** (<https://github.com/charactr-platform/vocos>): code licence **MIT** (<https://raw.githubusercontent.com/charactr-platform/vocos/main/LICENSE>). Paper: "presenting Vocos, a new model that directly generates Fourier spectral coefficients… substantially improves computational efficiency, achieving an **order of magnitude increase in speed** compared to prevailing time-domain neural vocoding approaches" (<https://arxiv.org/abs/2306.00814>). Weights licence for the released checkpoints: **not published** in the retrieved sources (Hugging Face unreachable in that session).
- sherpa-onnx ships Vocos as an ONNX vocoder: `vocos-22khz-univ.onnx` **51M** at <https://github.com/k2-fsa/sherpa-onnx/releases/download/vocoder-models/vocos-22khz-univ.onnx>, alternatives `hifigan_v1/v2/v3.onnx`; also `vocos_24khz.onnx` for ZipVoice at <https://github.com/k2-fsa/sherpa-onnx/releases/download/vocoder-models/vocos_24khz.onnx> (<https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html>, <https://k2-fsa.github.io/sherpa/onnx/tts/zipvoice.html>).

### 7.4 Piper and "piper-plus"

- **rhasspy/piper**: code licence **MIT**, "Copyright (c) 2022 Michael Hansen" (<https://raw.githubusercontent.com/rhasspy/piper/master/LICENSE.md>). The fork **OHF-Voice/piper1-gpl is GPL-3.0** — its `COPYING` file is the GNU GPL v3 text (<https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/COPYING>). Both `LICENSE` and `LICENSE.md` in that repo 404 (<https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/LICENSE>, <https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/LICENSE.md>), so `COPYING` is the licence file.
- Third-party statement of the transition (piper-plus README, Japanese): "オリジナルの rhasspy/piper は 2025年10月にアーカイブ済み。OHF-Voice/piper1-gpl は GPL-3.0 に移行。piper-plus は espeak-ng に依存しない唯一の MIT 互換フォークです" ("the original rhasspy/piper was archived in October 2025; OHF-Voice/piper1-gpl moved to GPL-3.0; piper-plus is the only MIT-compatible fork that does not depend on espeak-ng") (<https://raw.githubusercontent.com/ayutaz/piper-plus/dev/README.md>).
- **Piper has an ONNX runtime** — sherpa-onnx consumes `rhasspy/piper-voices` ONNX models, adding metadata and generating `tokens.txt` (<https://k2-fsa.github.io/sherpa/onnx/tts/piper.html>).
- **Chinese Piper voices exist**: upstream path `https://huggingface.co/rhasspy/piper-voices/tree/main/zh/zh_CN/xiao_ya/medium`, and a `chaowen` voice added in commit `10eb5c7` (<https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-xiao_ya-medium.html>, <https://huggingface.co/rhasspy/piper-voices/commit/10eb5c756ae21b759c8344d54aef86f9399ae92d>).
- Licence of the **voice weights**: piper-voices voices ship per-voice `MODEL_CARD` files (e.g. <https://huggingface.co/rhasspy/piper-voices/blame/main/zh/zh_CN/chaowen/medium/MODEL_CARD>); the licence text inside those cards **was not retrievable** in the research session (Hugging Face fetch failed repeatedly) → per-voice weights licence **not published / not retrieved**.
- **piper-plus** (`ayutaz/piper-plus`) — MIT, espeak-ng-free, includes Chinese: "8言語対応 — 日本語・英語・中国語・スペイン語・フランス語・ポルトガル語・スウェーデン語・韓国語 (ja=0, en=1, **zh=2**, …) ※学習済みモデルは6言語 (JA/EN/ZH/ES/FR/PT)"; badge "License: MIT"; C++ CLI plus a **C API shared library** `libpiper_plus.so/.dylib/.dll` with FFI and streaming; Kotlin/Android G2P published on Maven Central as `io.github.ayutaz:piper-plus-g2p-android:1.0.0`; a prebuilt C++ **Linux arm64** binary exists (`piper-plus-cpp-linux-arm64.tar.gz`); and it states "piper-plus は独自の G2P・音素体系を使用しているため、upstream Piper (rhasspy/piper-voices) のモデルとは互換性がありません" (its own phone set, **incompatible with upstream piper voices**) (<https://raw.githubusercontent.com/ayutaz/piper-plus/dev/README.md>).
- piper-plus benchmark, hardware named: **Intel Xeon E5-2650 v4 @ 2.20 GHz, 48 cores, Linux x86_64, Python 3.12, ONNX Runtime 1.24**; English test sentence "Hello, how are you doing today?", 25 phonemes, 5 warmups / 30 runs: piper-plus MB-iSTFT RTF **0.078**, latency P50 27 ms, 38 MB, 208 MB RAM, 19.6 M params, MIT; Piper upstream `en_US-lessac-medium` RTF **0.066** / P50 35 ms / 60 MB; sherpa-onnx `vits-piper-en_US-amy-low` RTF **0.075** / P50 53 ms / 60 MB. **English-only test; no Chinese RTF published on that page** (<https://raw.githubusercontent.com/ayutaz/piper-plus/dev/README.md>).
- **Android arm64 prebuilt for piper-plus**: the documented release list contains `piper-plus-cpp-windows-x64.zip`, `piper-plus-cpp-macos-arm64.tar.gz`, `piper-plus-cpp-linux-x64.tar.gz`, `piper-plus-cpp-linux-arm64.tar.gz` — **no Android `.so`/AAR asset is documented** → Android arm64 prebuilt artifact **not published** on the retrieved page; the documented Android story is the Kotlin G2P library on Maven Central plus a Unity plugin (<https://raw.githubusercontent.com/ayutaz/piper-plus/dev/README.md>).

### 7.5 Other engines checked (AR status and licence/coverage marks)

- **F5-TTS** (`SWivid/F5-TTS`) — OPEN SOURCE, **MIT** ("Copyright (c) 2024 Yushen CHEN") (<https://raw.githubusercontent.com/SWivid/F5-TTS/main/LICENSE>). Listed in CosyVoice's own comparison table as open-source, 0.3B params, test-zh CER 1.52 % (<https://raw.githubusercontent.com/FunAudioLLM/CosyVoice/main/README.md>). Flow-matching generation; **AR/non-AR status not stated** on the retrieved pages. ONNX/Android port: **not published**.
- **IndexTTS / IndexTTS2** (`index-tts/index-tts`) — open weights but **NOT a standard OSS licence**: the "bilibili Model Use License Agreement" grants a "worldwide, non-exclusive, non-transferable, royalty-free limited license", but §2.2 requires a separate written licence if you or affiliates had **>100 million monthly active users** in the prior calendar month, or **>RMB 1 billion** annual revenue in the prior year; §4.2 prohibits high-risk deployments without independent compliance; §6 governs under PRC law with the Shanghai Arbitration Commission (<https://raw.githubusercontent.com/index-tts/index-tts/main/LICENSE>). CosyVoice's table lists Index-TTS2 as open-source, 1.5B params (no AR label) (<https://raw.githubusercontent.com/FunAudioLLM/CosyVoice/main/README.md>). A licence-clarification issue exists: **#228** "Clarification Needed on License: Apache 2.0 vs. Commercial Use Restriction" (<https://github.com/index-tts/index-tts/issues/228>).
- **Muyan-TTS** (`MYZY-AI/Muyan-TTS`) — **ENGLISH ONLY, therefore not a Mandarin candidate**: "Note: Muyan-TTS only supports English input since the training data is heavily skewed toward English." Architecture: "Left is an LLM that models the parallel corpus of text … and audio tokens. Right is a SoVITS model that decodes the generated audio tokens" → **autoregressive, LLM-based**; built on Llama-3.2-3B; requires `ref_wav` + `prompt_text`; inference on a single NVIDIA A100 (40 GB, PCIe) with r = 0.33 s of compute per second of audio; no ONNX/C++/Android path documented. Licence: **not published** on the README retrieved (<https://raw.githubusercontent.com/MYZY-AI/Muyan-TTS/main/README.md>).
- **"Chinese-TTS-ONNX"**: no credible primary-source project of that exact name was found in the searches run → **not published / not verified**.

### 7.6 iFlytek offline TTS (讯飞离线语音合成) — **COMMERCIAL CLOSED SDK**

- Official page: AIkit 离线语音合成（轻量级）Android SDK — <https://www.xfyun.cn/doc/tts/AIkit_offline_tts/Android-SDK%28Lightweight%29.html>. Capability: "普通品质aisound合成能力，支持中英发音人".
- **Licensing / authorisation terms, quoted verbatim (section 2. 授权说明)**: "授权方式支持【设备授权】和【应用授权】2种。设备授权： 按照设备数和有效期授权，激活设备数达到授权量上限后，新设备将无法继续激活使用。SDK采集多个设备标识按照权重算法生成设备指纹精准标识设备，计量准确。支持所有平台。应用授权： 对指定应用授权，仅可在授权的应用上使用，无数量限制，可限制有效期。需提供应用唯一标识，授权能力后，应用级授权支持Android、iOS平台应用。在能力首次使用时，需要先激活后方可使用。激活时会获取授权license缓存到设备内部存储中。**在线激活**: 在首次使用时，需要将设备联网，SDK初始化时获取授权license激活。设备激活后，即可在无网环境下使用…" — i.e. per-device or per-app authorisation; **first use requires network connectivity to activate**; clearing app storage/app data invalidates the licence until the device is online again (same URL).
- Integration is **Java-only Android Studio / Gradle**: "复制 AIKit.aar 到项目的 libs 目录下，然后在项目的 build.gradle 文件中，增加如下配置… `implementation files('libs/AIKit.aar')`"; initialisation needs `appId`, `apiKey`, `apiSecret`, `workDir` (same URL).
- Permissions requested include `INTERNET`, `READ_PHONE_STATE` (IMEI for precise authorisation), `READ_PRIVILEGED_PHONE_STATE`, `READ_PHONE_NUMBERS`, `MANAGE_EXTERNAL_STORAGE` (Android 11+), `WRITE/READ_EXTERNAL_STORAGE`, `MOUNT_UNMOUNT_FILESYSTEMS` (same URL).
- Compatibility: Android 5.0 – Android 13; HarmonyOS not validated (same URL).
- Output format: audio frames with `encoding` ∈ {lame, speex, opus, speex-wb} (default `speex-wb`), `sample_rate` ∈ {16000, 8000} (default 16000), `channels` ∈ {1,2}, `bit_depth` ∈ {16,8} (same URL).
- Voices (`vcn`) documented on that page include `xiaoyan`, `xiaofeng`, `xiaomeng`, `xiaoqiang`, `xiaolin`, `xiaorong` (Sichuan), `xiaoqian` (Northeast), `nannan` (child), `xiaomei` (Cantonese), plus English `catherine` / `john` (same URL).
- Related official pages: full-weight Android SDK <https://www.xfyun.cn/doc/tts/AIkit_offline_tts/Android-SDK.html>; Windows <https://www.xfyun.cn/doc/tts/AIkit_offline_tts/Windows-SDK.html>; privacy policy <https://www.xfyun.cn/doc/total_sdk_privacy/aikit_offline_tts_privacy.html>.
- Pricing / royalty amounts: **not published** on these documentation pages.

### 7.7 Open-source vs. commercial, and language-coverage summary for candidate 7

- **Open source, permissive, Mandarin-capable with a C/C++ API**: sherpa-onnx (Apache-2.0 code; per-model weight licences vary — `matcha-icefall-zh-baker` is explicitly non-commercial-by-dataset); MeloTTS (MIT; Mandarin via the sherpa-onnx conversion); piper-plus (MIT, includes `zh`, C API, espeak-ng-free); Piper upstream (MIT code, archived October 2025 per the piper-plus README).
- **Open source but copyleft**: OHF-Voice/piper1-gpl (GPL-3.0) — relevant when linking into a Qt app.
- **Open source code with separately governed weights**: CosyVoice (Apache-2.0 code; README declares the demonstrated content academic-purposes-only); IndexTTS (custom bilibili agreement with MAU/revenue thresholds); Matcha-TTS and Vocos (MIT code; weights licence not published in the retrieved sources).
- **Non-Chinese (English only), excluded as Mandarin candidates**: Kokoro `kokoro-en-v0_19`, Muyan-TTS (explicit), Piper/KittenTTS English voices.
- **Commercial closed SDK**: iFlytek 讯飞离线语音合成 (AIkit) — device/app authorisation, first-use online activation, Java `AIKit.aar` + Gradle, 16 kHz / 8 kHz output.

---

## Consolidated list of all URLs used

**Candidate 1 — MeloTTS**
- https://github.com/myshell-ai/MeloTTS
- https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/README.md
- https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/LICENSE
- https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/requirements.txt
- https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/docs/install.md
- https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/melo/configs/config.json
- https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/melo/text/chinese.py
- https://raw.githubusercontent.com/myshell-ai/MeloTTS/main/melo/text/chinese_bert.py
- https://api.github.com/repos/myshell-ai/MeloTTS
- https://api.github.com/repos/myshell-ai/MeloTTS/contents/
- https://api.github.com/repos/myshell-ai/MeloTTS/search/issues?q=repo%3Amyshell-ai%2FMeloTTS+pronunciation
- https://huggingface.co/myshell-ai/MeloTTS-Chinese
- https://huggingface.co/api/models/myshell-ai/MeloTTS-Chinese/tree/main
- https://huggingface.co/hfl/chinese-roberta-wwm-ext-large
- https://huggingface.co/api/models/hfl/chinese-roberta-wwm-ext-large/tree/main
- https://github.com/myshell-ai/MeloTTS/issues/98
- https://github.com/myshell-ai/MeloTTS/issues/206
- https://github.com/myshell-ai/MeloTTS/issues/241
- https://api.github.com/search/issues?q=repo%3Amyshell-ai%2FMeloTTS+%E5%A4%9A%E9%9F%B3%E5%AD%97
- https://arxiv.org/abs/2106.06103
- sherpa-onnx conversion: https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html · https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html · https://api.github.com/repos/k2-fsa/sherpa-onnx/contents/scripts/melo-tts · https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/master/scripts/melo-tts/README.md · https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/master/scripts/melo-tts/export-onnx.py · https://github.com/k2-fsa/sherpa-onnx/tree/master/scripts/melo-tts · https://github.com/k2-fsa/sherpa-onnx/releases/download/tts-models/vits-melo-tts-zh_en.tar.bz2 · https://github.com/k2-fsa/sherpa-onnx/pull/1209

**Candidate 2 — ChatTTS**
- https://github.com/2noise/ChatTTS
- https://raw.githubusercontent.com/2noise/ChatTTS/main/README.md
- https://api.github.com/repos/2noise/ChatTTS
- https://api.github.com/repos/2noise/ChatTTS.cpp
- https://api.github.com/search/repositories?q=ChatTTS+cpp
- https://api.github.com/search/repositories?q=ChatTTS
- https://huggingface.co/2Noise/ChatTTS
- https://huggingface.co/api/models/2Noise/ChatTTS/tree/main
- https://huggingface.co/api/models/2Noise/ChatTTS/tree/main/asset
- https://github.com/2noise/ChatTTS/pull/350
- https://github.com/2noise/ChatTTS/pull/178
- https://github.com/2noise/ChatTTS/issues/659
- https://github.com/2noise/ChatTTS/issues/906
- https://api.github.com/search/issues?q=repo%3A2noise%2FChatTTS+Chinese+pronunciation

**Candidate 3 — Kokoro**
- https://github.com/hexgrad/kokoro
- https://raw.githubusercontent.com/hexgrad/kokoro/main/README.md
- https://api.github.com/repos/hexgrad/kokoro
- https://huggingface.co/hexgrad/Kokoro-82M
- https://huggingface.co/hexgrad/Kokoro-82M/blob/main/VOICES.md
- https://huggingface.co/api/models/hexgrad/Kokoro-82M/tree/main
- https://huggingface.co/api/models/hexgrad/Kokoro-82M/tree/main/voices
- https://huggingface.co/hexgrad/Kokoro-82M-v1.1-zh
- https://github.com/hexgrad/misaki
- https://raw.githubusercontent.com/hexgrad/misaki/main/README.md
- https://raw.githubusercontent.com/hexgrad/misaki/main/pyproject.toml
- https://github.com/hexgrad/kokoro/issues/238
- https://github.com/hexgrad/kokoro/pull/313
- https://api.github.com/search/issues?q=repo%3Ahexgrad%2Fkokoro+Chinese+zh+pronunciation
- https://github.com/thewh1teagle/kokoro-onnx
- https://raw.githubusercontent.com/thewh1teagle/kokoro-onnx/main/README.md
- https://github.com/thewh1teagle/kokoro-onnx/releases/tag/model-files-v1.0
- https://github.com/thewh1teagle/kokoro-onnx/releases/tag/model-files-v1.1
- https://api.github.com/repos/thewh1teagle/kokoro-onnx/releases/tags/model-files-v1.0
- https://api.github.com/repos/thewh1teagle/kokoro-onnx/releases/tags/model-files-v1.1
- https://github.com/taylorchu/kokoro-onnx/releases/tag/v0.2.0
- https://huggingface.co/nvidia/kokoro-82M-onnx-opt
- https://arxiv.org/abs/2306.07691 · https://arxiv.org/abs/2203.02395
- https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html
- https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html
- https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese-English/kokoro-multi-lang-v1_0.html
- https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese-English/kokoro-multi-lang-v1_1.html
- Android ports: https://api.github.com/repos/puff-dayo/Kokoro-82M-Android · https://raw.githubusercontent.com/puff-dayo/Kokoro-82M-Android/latest/README.md · https://api.github.com/repos/siva-sub/NekoSpeak · https://api.github.com/repos/Mobile-Artificial-Intelligence/maise · https://api.github.com/repos/biaji/kokoro-tts

**Candidate 4 — PaddleSpeech**
- https://github.com/PaddlePaddle/PaddleSpeech
- https://api.github.com/repos/PaddlePaddle/PaddleSpeech
- https://api.github.com/repos/PaddlePaddle/PaddleSpeech/git/trees/develop?recursive=1
- https://api.github.com/repos/PaddlePaddle/PaddleSpeech/contents/paddlespeech/t2s/frontend?ref=develop
- https://api.github.com/repos/PaddlePaddle/PaddleSpeech/contents/paddlespeech/t2s/exps?ref=develop
- https://api.github.com/repos/PaddlePaddle/PaddleSpeech/contents/paddlespeech/t2s/exps/fastspeech2?ref=develop
- https://api.github.com/repos/PaddlePaddle/PaddleSpeech/contents/examples/csmsc/tts3?ref=develop
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/README.md
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/LICENSE
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/released_model.md
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/tts/PPTTS.md
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/tts/models_introduction.md
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/docs/source/tts/zh_text_frontend.md
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/resource/pretrained_models.py
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/cli/tts/infer.py
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/exps/syn_utils.py
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/frontend/zh_frontend.py
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/t2s/frontend/g2pw/onnx_api.py
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/paddlespeech/server/engine/tts/online/python/tts_engine.py
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/examples/csmsc/tts3/run_cnndecoder.sh
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/examples/csmsc/tts3/local/paddle2onnx.sh
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/examples/csmsc/tts3/local/ort_predict.sh
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/examples/csmsc/voc5/conf/default.yaml
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/demos/TTSAndroid/README.md
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/demos/TTSArmLinux/README.md
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/demos/streaming_tts_server/README.md
- https://raw.githubusercontent.com/PaddlePaddle/PaddleSpeech/develop/runtime/README.md
- https://raw.githubusercontent.com/wiki/PaddlePaddle/PaddleSpeech/TTS-Benchmark.md
- https://raw.githubusercontent.com/wiki/PaddlePaddle/PaddleSpeech/PaddleSpeech-Server-WebSocket-API.md
- https://github.com/PaddlePaddle/PaddleSpeech/tree/develop/paddlespeech/t2s/exps
- https://github.com/PaddlePaddle/PaddleSpeech/tree/develop/demos/TTSCppFrontend
- https://www.paddlepaddle.org.cn/lite/v2.11/source_compile/compile_env.html
- https://www.paddlepaddle.org.cn/lite/v2.11/api_reference/java_api_doc.html
- https://arxiv.org/abs/2006.04558 · https://arxiv.org/abs/2010.05646
- https://paddlespeech.cdn.bcebos.com/Parakeet/released_models/fastspeech2/fastspeech2_csmsc_onnx_0.2.0.zip
- https://paddlespeech.cdn.bcebos.com/Parakeet/released_models/fastspeech2/fastspeech2_aishell3_onnx_1.1.0.zip
- https://paddlespeech.cdn.bcebos.com/Parakeet/released_models/fastspeech2/fastspeech2_canton_onnx_1.4.0.zip
- https://paddlespeech.cdn.bcebos.com/Parakeet/released_models/g2p/new/G2PWModel_1.1.zip
- https://paddlespeech.cdn.bcebos.com/demos/TTSAndroid/2022-11-29-app-release.apk
- https://github.com/PaddlePaddle/PaddleSpeech/issues/3737 · /3297 · /3311 · /3664 · /3210 · /4171 · /1283 · /3422 · /678 · https://github.com/PaddlePaddle/PaddleSpeech/discussions/2538

**Candidate 5 — ekho**
- https://github.com/hgneng/ekho
- https://api.github.com/repos/hgneng/ekho
- https://api.github.com/repos/hgneng/ekho/contents/
- https://api.github.com/repos/hgneng/ekho/contents/ekho-data
- https://api.github.com/repos/hgneng/ekho/contents/src
- https://api.github.com/repos/hgneng/ekho/git/trees/master?recursive=1
- https://raw.githubusercontent.com/hgneng/ekho/master/README.md
- https://raw.githubusercontent.com/hgneng/ekho/master/README
- https://raw.githubusercontent.com/hgneng/ekho/master/COPYING
- https://raw.githubusercontent.com/hgneng/ekho/master/LICENSE
- https://raw.githubusercontent.com/hgneng/ekho/master/INSTALL
- https://raw.githubusercontent.com/hgneng/ekho/master/ChangeLog
- https://raw.githubusercontent.com/hgneng/ekho/master/configure.ac
- https://raw.githubusercontent.com/hgneng/ekho/master/Makefile.am
- https://raw.githubusercontent.com/hgneng/ekho/master/deploy.sh
- https://raw.githubusercontent.com/hgneng/ekho/master/scripts/download_voice.pl
- https://raw.githubusercontent.com/hgneng/ekho/master/src/README
- https://raw.githubusercontent.com/hgneng/ekho/master/src/audio.h
- https://raw.githubusercontent.com/hgneng/ekho/master/src/audio.cpp
- https://raw.githubusercontent.com/hgneng/ekho/master/src/libekho_impl.cpp
- https://raw.githubusercontent.com/hgneng/ekho/master/ekho-data/pinyin/README
- https://raw.githubusercontent.com/hgneng/ekho/master/ekho-data/jyutping/COPYING-cantonese-wong
- https://www.eguidedog.net/ekho.php
- https://www.eguidedog.net/doc/doc_use_ekho_android_cn.php
- https://sourceforge.net/projects/e-guidedog/files/Ekho-Voice-Data/0.2/
- deprecated ekho-android: https://api.github.com/repos/hgneng/ekho-android (404) · https://github.com/hgneng/ekho-android (404) · https://api.github.com/users/hgneng/repos?per_page=100&sort=updated · https://api.github.com/search/repositories?q=ekho-android · https://api.github.com/repos/hgneng/ekho-android-cantonese · https://raw.githubusercontent.com/hgneng/ekho-android-cantonese/main/jni/Android.mk · https://raw.githubusercontent.com/hgneng/ekho-android-cantonese/main/build.gradle · https://raw.githubusercontent.com/hgneng/ekho-android-cantonese/main/README.md (404)

**Candidate 6 — eSpeak-NG**
- https://github.com/espeak-ng/espeak-ng
- https://api.github.com/repos/espeak-ng/espeak-ng
- https://api.github.com/repos/espeak-ng/espeak-ng/contents/android
- https://api.github.com/repos/espeak-ng/espeak-ng/contents/android/jni
- https://api.github.com/repos/espeak-ng/espeak-ng/contents/dictsource
- https://api.github.com/repos/espeak-ng/espeak-ng/releases
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/COPYING
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/CMakeLists.txt
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/CMakeLists.txt
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/libespeak-ng/CMakeLists.txt
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/libespeak-ng/speech.c
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/speak_lib.h
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/src/include/espeak-ng/espeak_ng.h
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/building.md
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/integration.md
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/index.md
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages.md
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/lang/sit/cmn
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/lang/zh/cmn (404)
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_rules
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_list
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/extra/cmn_listx
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/jni/CMakeLists.txt
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/jni/jni/eSpeakService.c
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/build.gradle
- https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/android/CLAUDE.md
- https://packages.debian.org/bookworm/espeak-ng-data

**Candidate 7 — other engines**
- sherpa-onnx: https://github.com/k2-fsa/sherpa-onnx · https://api.github.com/repos/k2-fsa/sherpa-onnx · https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/master/LICENSE · https://api.github.com/repos/k2-fsa/sherpa-onnx/releases?per_page=100&page=1 · https://api.github.com/repos/k2-fsa/sherpa-onnx/releases/tags/tts-models · https://api.github.com/repos/k2-fsa/sherpa-onnx/pulls/2664 · https://api.github.com/search/issues?q=repo%3Ak2-fsa%2Fsherpa-onnx+%E5%A4%9A%E9%9F%B3%E5%AD%97 · https://github.com/k2-fsa/sherpa-onnx/releases/tag/v1.13.7 · https://github.com/k2-fsa/sherpa-onnx/releases/tag/tts-models · https://github.com/k2-fsa/sherpa-onnx/releases/tag/hr-files · https://github.com/k2-fsa/sherpa-onnx/pull/2664 · https://github.com/k2-fsa/sherpa-onnx/blob/master/sherpa-onnx/c-api/c-api.h · https://github.com/k2-fsa/sherpa-onnx/blob/master/sherpa-onnx/c-api/c-api.cc · https://github.com/k2-fsa/sherpa-onnx/tree/master/android/SherpaOnnxTtsEngine · https://github.com/k2-fsa/sherpa-onnx/releases/download/vocoder-models/vocos-22khz-univ.onnx · https://github.com/k2-fsa/sherpa-onnx/releases/download/vocoder-models/vocos_24khz.onnx · https://github.com/k2-fsa/sherpa-onnx/issues/2726 · /2904 · /2902 · /2961 · /1552 · /2004 · /2325 · /468 · /3813 · https://k2-fsa.github.io/sherpa/onnx/tts/index.html · https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/index.html · https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/vits.html · https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/matcha.html · https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/kokoro.html · https://k2-fsa.github.io/sherpa/onnx/tts/pretrained_models/rtf.html · https://k2-fsa.github.io/sherpa/onnx/tts/zipvoice.html · https://k2-fsa.github.io/sherpa/onnx/tts/piper.html · https://k2-fsa.github.io/sherpa/onnx/tts/faq.html · https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/index.html · https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese/vits-piper-zh_CN-xiao_ya-medium.html · https://k2-fsa.github.io/sherpa/onnx/tts/all/Chinese-English/matcha-icefall-zh-en.html · https://k2-fsa.github.io/sherpa/onnx/c-api/index.html · https://k2-fsa.github.io/sherpa/onnx/android/index.html · https://k2-fsa.github.io/sherpa/onnx/android/build-sherpa-onnx.html · https://k2-fsa.github.io/sherpa/onnx/android/prebuilt-apk.html · https://k2-fsa.github.io/sherpa/onnx/homophone-replacer/index.html
- CosyVoice: https://github.com/FunAudioLLM/CosyVoice · https://raw.githubusercontent.com/FunAudioLLM/CosyVoice/main/README.md · https://raw.githubusercontent.com/FunAudioLLM/CosyVoice/main/LICENSE · https://huggingface.co/FunAudioLLM/CosyVoice-300M · https://huggingface.co/FunAudioLLM/CosyVoice2-0.5B · https://huggingface.co/FunAudioLLM/Fun-CosyVoice3-0.5B-2512 · https://www.modelscope.cn/models/iic/CosyVoice-300M
- Matcha-TTS / Vocos: https://github.com/shivammehta25/Matcha-TTS · https://raw.githubusercontent.com/shivammehta25/Matcha-TTS/main/LICENSE · https://arxiv.org/abs/2309.03199 · https://github.com/charactr-platform/vocos · https://raw.githubusercontent.com/charactr-platform/vocos/main/LICENSE · https://arxiv.org/abs/2306.00814
- Piper / piper-plus: https://github.com/rhasspy/piper · https://raw.githubusercontent.com/rhasspy/piper/master/LICENSE.md · https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/COPYING · https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/LICENSE (404) · https://raw.githubusercontent.com/OHF-Voice/piper1-gpl/main/LICENSE.md (404) · https://raw.githubusercontent.com/ayutaz/piper-plus/dev/README.md · https://huggingface.co/rhasspy/piper-voices/commit/10eb5c756ae21b759c8344d54aef86f9399ae92d · https://huggingface.co/rhasspy/piper-voices/blame/main/zh/zh_CN/chaowen/medium/MODEL_CARD
- F5-TTS / IndexTTS / Muyan-TTS: https://github.com/SWivid/F5-TTS · https://raw.githubusercontent.com/SWivid/F5-TTS/main/LICENSE · https://github.com/index-tts/index-tts · https://raw.githubusercontent.com/index-tts/index-tts/main/LICENSE · https://github.com/index-tts/index-tts/issues/228 · https://github.com/MYZY-AI/Muyan-TTS · https://raw.githubusercontent.com/MYZY-AI/Muyan-TTS/main/README.md
- iFlytek offline SDK: https://www.xfyun.cn/doc/tts/AIkit_offline_tts/Android-SDK%28Lightweight%29.html · https://www.xfyun.cn/doc/tts/AIkit_offline_tts/Android-SDK.html · https://www.xfyun.cn/doc/tts/AIkit_offline_tts/Windows-SDK.html · https://www.xfyun.cn/doc/total_sdk_privacy/aikit_offline_tts_privacy.html

