# eSpeak-NG Mandarin (cmn) Chinese support — primary-source research report

All claims below are tied to an exact URL. Where a number is not published, this report says
"not published". Items that could not be verified against a primary source are marked
"not determinable from public sources".

Environment note: GitHub HTML pages were noisy/unfetchable in this environment, so every
GitHub claim below comes from the REST API (`api.github.com`), raw file endpoints
(`raw.githubusercontent.com`), or API search results. Anonymous GitHub REST `core` calls were
rate-limited for part of the session; `search` calls remained available.

---

## 1. Official status of Mandarin in eSpeak-NG

### 1.1 Default branch

- `espeak-ng/espeak-ng` default branch is **`master`** — `"default_branch":"master"`,
  verified at <https://api.github.com/repos/espeak-ng/espeak-ng>.

### 1.2 The exact lines mentioning Chinese / Mandarin / `cmn` / `zh`

From <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages.md>
(verbatim table rows, in file order):

```
| `sit`       | `yue`             | Sino-Tibetan          | Chinese                     | Cantonese              |
| `sit`       | `hak`             | Sino-Tibetan          | Chinese                     | Hakka                  |
| `sit`       | `cmn`             | Sino-Tibetan          | Chinese                     | Mandarin               |
```

Document header line, verbatim: `Development version of eSpeak NG supports 127 languages and accents,`
above `which are listed in table below.`

The only other `zh`-adjacent text in that file is the ISO 639-5 family code column (`sit`) and the
statement that identifiers are BCP 47:

> "The languages in espeak-ng are grouped by their
> [ISO 639-5](https://en.wikipedia.org/wiki/List_of_ISO_639-5_codes) language
> family code. They are identified by their
> [BCP 47](https://en.wikipedia.org/wiki/BCP47) identifier."

- **`cmn` is documented as supported.** It appears in the supported-language table as
  Sino-Tibetan → Chinese → Mandarin.
- **The string `zh` does not appear in `docs/languages.md` at all** (only `cmn`, `yue`, `hak` for
  Sinitic languages). Verified by reading the whole file at the URL above.

### 1.3 Does the documentation say anything about QUALITY or LIMITATIONS for Mandarin?

**No.** `docs/languages.md` contains exactly seven footnote markers, applied to Arabic (`[3,7]`),
Bosnian (`[1,2]`), Cherokee (`[1,6]`), Persian `fa-latn` (`[1,5]`), Lingua Franca Nova (`[1,2]`),
Japanese (`[4]`), Macedonian (`[1,2]`), Serbian (`[1,2]`), Turkmen (`[1]`), Uyghur (`[1,3]`),
Uzbek (`[1,2]`). The footnotes are:

```
\[1\] With Latin alphabet.
\[2\] With Cyrillic alphabet.
\[3\] With Arabic alphabet.
\[4\] With Hiragana and Katakana syllabary.
\[5\] Only Farsi/Persian.
\[6\] Only Cherokee-English Dictionary fully annotated UTF-8 pronunciation.
\[7\] Only fully diacritized Arabic.
```

**No footnote is attached to `cmn`, `yue` or `hak`, and no quality, coverage, accuracy or limitation
statement about Mandarin appears anywhere in `docs/languages.md`.**
Source: <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages.md>

### 1.4 Voice/language definition for `cmn` (in-repo source data)

`espeak-ng-data/lang/sit/cmn` is committed in the repository
(<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/lang/sit/cmn>),
verbatim body:

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

- This is the only place in the shipped voice data that shows `zh` as an accepted language tag for
  the Mandarin voice. The rest of the file is commented-out dialect-variant `replace` experiments
  (e.g. `//[en]: replace ng with n`).
- A second Mandarin voice exists as a separate variant **`cmn-latn-pinyin`**
  ("Chinese (Mandarin, latin as Pinyin)"), evidenced in a `espeak-ng --voices` dump quoted in
  <https://github.com/espeak-ng/espeak-ng/issues/1580> and named in
  <https://github.com/espeak-ng/espeak-ng/pull/2524> (`cmn, cmn-Latn-pinyin` are listed among
  languages with an explicit `pitch` directive).

### 1.5 Per-language documentation

`docs/index.md` is the full documentation index
(<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/index.md>). Under
"Pronunciation Guides" it lists **only** English (`languages/gmw/en.md`) and, under
"Other Miscellaneous Information", Cherokee (`languages/iro/chr.md`). There is **no
`docs/languages/sit/cmn.md`**: the raw URL returns HTTP 404
(<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages/sit/cmn.md>).

- Therefore: **eSpeak-NG publishes no per-language quality/limitation documentation for Mandarin.**

### 1.6 General (non-Chinese-specific) quality statement in the project README

From <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md>:

> "eSpeak NG uses a "formant synthesis" method. This allows many languages to be
> provided in a small size. The speech is clear, and can be used at high speeds,
> but is not as natural or smooth as larger synthesizers which are based on human
> speech recordings."

> "Potential for other languages. Several are included in varying stages
> of progress. Help from native speakers for these or other languages is
> welcome."

Both are **generic** statements about the whole project, not about Mandarin specifically.

---

## 2. Does eSpeak-NG handle Chinese tones? What is the actual Han → pinyin mechanism?

### 2.1 Chinese-related files that exist in `dictsource/` today

Directory listing fetched from <https://api.github.com/repos/espeak-ng/espeak-ng/contents/dictsource>
(Chinese-related entries, with the `size` in bytes returned by that API):

| file | size (bytes) |
|---|---|
| `dictsource/cmn_emoji` | 91353 |
| `dictsource/cmn_list` | 36237 |
| `dictsource/cmn_rules` | 3997 |
| `dictsource/hak_list` | 349 |
| `dictsource/hak_rules` | 5366 |

`dictsource/yue_list` also exists (HTTP 200,
<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/yue_list>), and
`yue`/`hak` both appear in the build's dictionary list (see §2.6).

### 2.2 The files named in the research brief do NOT exist (all HTTP 404 on `master`)

| requested path | result |
|---|---|
| `dictsource/zh_listx` | 404 — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/zh_listx> |
| `dictsource/zh_rules` | 404 — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/zh_rules> |
| `dictsource/zh_pinyin` | 404 — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/zh_pinyin> |
| `dictsource/cmn_listx` | 404 — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_listx> |
| `dictsource/extra/zh_listx` | 404 — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/extra/zh_listx> |
| `docs/languages/sit/cmn.md` | 404 — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages/sit/cmn.md> |

The `zh`/`zhy` → `cmn`/`yue` rename is documented by the merged PR "Rename zh and zhy to cmn and yue",
<https://github.com/espeak-ng/espeak-ng/pull/940> (state: closed, merged 2021-05-16; author
`jaacoppi`, association COLLABORATOR). Its body states:
> "See discussion in #933. There are no tests for Cantonese, I only tried a few words from the
> extended dictionary. Checks for Mandarin pass."

### 2.3 `dictsource/cmn_list` — the built-in character→pinyin dictionary

URL: <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_list> (36237 bytes).

Structure (verbatim excerpts):

The first line is a phoneme-table declaration, then number handling:

```
_?? @:11   // unrecognized character

//numbers
_dpt t'iE35n_|
_0 liN35
_1 ji55
_2 @r51
_3 san55
_4 si[51
_5 wu214
_6 liou51
_7 tS;hi55
_8 pA55
_9 tS;iou214
```

Then a commented-out block of Latin letter names (`//a ei51` … `//z zi51`) with the reason given in-file:

```
// Latin letters with Chinese accent
// This will make letter within English sentense translated not correctly. i.e. "ma is a horse". "a" will be translated as ei51.
```

Then bopomofo (Zhuyin) letter mappings:

```
//bopomofo letters
ㄅ po55
ㄆ pho55
ㄇ mo55
ㄈ fo55
```

Then the `$textmode` switch and the Han→pinyin list, with the dictionary's own header comment:

```
$textmode

// Most frequent pronunciations of the 3799 most common characters (from Unihan database ftp://ftp.unicode.org/Public/UNIDATA/Unihan.zip, kHanyuPinlu field with some corrections)
涉	she4
礦	kuang4
河	he2
反	fan3
遠	yuan3
应	ying1
```

Further quoted entries (verbatim, tab-separated):

```
上	shang4
五	wu3
好	hao3
的	de5
個	ge5
們	men5
```

**Format:** `<one Han character><TAB><pinyin syllable><tone digit>`. Tone is marked as a **trailing
ASCII digit 1–5** where `5` is the neutral tone (e.g. `的 de5`, `個 ge5`, `么 me5`, `們 men5`,
`吧 ba5`). There is **no tone-mark (diacritic) notation in the dictionary**; diacritics are accepted
only on pinyin *input* (see §2.4 `.replace`).

**Dictionary size:** the file's own header states **3799 characters**. An independent count of the
number of lines was not obtained (see §5, limitation). At least the file's own stated figure is a
primary-source number; no other published count was found.

**This dictionary gives exactly ONE reading per character** — chosen as the most frequent reading
from the Unihan `kHanyuPinlu` field. That is stated in the file's own comment quoted above.

### 2.4 `dictsource/cmn_rules` — pinyin → phonemes, and where tones become real

URL: <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_rules> (3997 bytes).

Header (verbatim):

```
// This file is UTF8 encoded

// Default is to handle latin characters as pinyin
// ?1:	speak latin characters as English words
```

Pinyin tone-mark normalisation (verbatim, first lines of the `.replace` block):

```
.replace
//replace tone mark with tone number
ˉ 1
ˊ 2
ˇ 3
ˋ 4
ā a1
á a2
ǎ a3
à a4
// TODO: àn is not handled
ō o1
ó o2
ŏ o3
ǒ o3
ò o4
```

Note the in-file `// TODO: àn is not handled` — an admitted gap.

Tone digits are mapped to **tone-contour phonemes** at the end of the file (verbatim):

```
// tone number
.group
       1	55
       2	35
       3	214
       4	51
       5	11

	|	_|
```

So: **1→55 (high level), 2→35 (rising), 3→214 (dipping), 4→51 (falling), 5→11 (neutral/low).**

Latin letters fall back to English via the `_^_EN` switch embedded in every letter group, e.g.

```
.group b
       b        _^_EN
       b (L03L02       p
```

`docs/dictionary.md` documents `_^_<language code>` as "Translate using a different language."
(<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/dictionary.md>).

**Conclusion: yes, eSpeak-NG handles Chinese tones.** Tones are carried as a digit through the
dictionary and converted to tonal phonemes by `cmn_rules`.

### 2.5 多音字 (polyphonic characters): what the mechanism actually is

#### (a) The base dictionary is single-reading only

`cmn_list` has one reading per character ("Most frequent pronunciations of the 3799 most common
characters … kHanyuPinlu field"), so by construction it cannot disambiguate 多音字 by context.
Source: <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_list>.

#### (b) There IS an extended dictionary with compound (multi-character) exceptions

`dictsource/extra/cmn_listx` **exists** (HTTP 200):
<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/extra/cmn_listx>

Its first four lines, verbatim:

```
//From Unihan database ftp://ftp.unicode.org/Public/UNIDATA/Unihan.zip kMandarin entries (except the ones that have kHanyuPinlu, which are in zh_list)
//with compounds from CC-CEDICT http://www.mdbg.net/chindict/chindict.php?page=cedict and some corrections
//21611 single characters plus 36500 compound exceptions (includes 320 added 'yi' and 10721 added 'bu' exceptions, and 9700 extra 2-syllable words for 3rd-tone sandhi blocking)
$textmode
```

Directly relevant facts stated by the file itself:
- **21,611 single characters** (the ones whose Unihan entry has no `kHanyuPinlu`) plus
- **36,500 compound exceptions**, of which
  - 320 added `yi` exceptions, 10,721 added `bu` exceptions (the 一/不 tone-sandhi cases), and
  - 9,700 extra 2-syllable words added specifically **"for 3rd-tone sandhi blocking"**.
- The comment still says "which are in `zh_list`" — a stale reference to the pre-rename filename
  (<https://github.com/espeak-ng/espeak-ng/pull/940>).

**File size in bytes: not published / not retrieved.** The `contents` API listing of `dictsource`
was truncated before the `extra/` directory, and a directory listing of `dictsource/extra` could not
be fetched while the anonymous API was rate-limited. The raw file was partially retrieved
(14,115 leading entries seen, all single Han characters in Unicode order starting `〇 ling2`,
`㐀 qiu1`, `㐁 tian3`), i.e. the raw fetch is served from the start of the file and the compound
section lies beyond the retrieved portion.

**Consequence for the exact compound line format:** the literal lines of the 36,500-entry compound
section were **not retrieved**, so their exact syntax is **not determinable from the portion of
public sources fetched here**. What *is* documented for multi-character dictionary entries in this
project is:
- `docs/dictionary.md` "Pronunciation Dictionary List → Multiple Words": "A pronunciation may also
  be specified for a group of words, when these appear together. Up to four words may be given,
  enclosed in brackets." Example given: `(de jure)    deI||dZ'U@rI2   // note || used as a word break
  in the phoneme string` and `(of a)       @v@`.
- A real Chinese example of that bracketed form, quoted by a reporter in
  <https://github.com/espeak-ng/espeak-ng/issues/606>: "If dictionary defines word rules, such as
  `(地 面) di4mian4`, the characters are matched multiple times".
- `$textmode` (documented in `docs/dictionary.md` as "Translating a Word to Another Word") is the
  mode `cmn_listx` itself turns on with the bare `$textmode` line, so compound entries in that file
  are word→replacement-text entries of the same shape as `cmn_list`'s character→pinyin entries.

#### (c) Is there word segmentation (jieba / pypinyin style)? — No, not in eSpeak-NG

The segmentation eSpeak-NG performs is *character-level*, not lexical/statistical:

- Merged PR by maintainer `alex19EP` (association MEMBER), "translate: segment $textmode replacement
  text like normal input", closed/merged 2026-07-22 —
  <https://github.com/espeak-ng/espeak-ng/pull/2455>. Verbatim from the PR body:
  > "**Chinese/Cantonese**: each hanzi must be a separate word so that words can match their
  > multi-word `*_list` entries, which map them to pinyin/jyutping via a second `$textmode`
  > replacement. The single-word re-translation of e.g. Mandarin 彩虹 matched nothing…"
  >
  > "Fixed by mirroring the tokenizer's word-splitting rules over the replacement text in
  > `TranslateWordWithBounds` (new `SegmentReplacement`): **ideographs become separate words**, and
  > non-ASCII non-alpha marks terminate a word. ASCII characters are always kept in-word…"

  i.e. the clause tokenizer splits each Han character into its own token; multi-character
  disambiguation can then only happen through dictionary lookup of multi-word entries
  (the "compound exceptions" of `cmn_listx`).
- A sibling PR on the same repository,
  <https://github.com/espeak-ng/espeak-ng/pull/2520> (state: open), describes the same engine path
  as a bug fix: "**Han characters weren't split when reached via `alt_alphabet`** (Arabic →
  Mandarin, #2511). The test was `langopts.ideographs`, which the *base* translator doesn't set, so
  a Han run arrived as one token and missed every dictionary entry. Gated on a new `AL_IDEOGRAPHS`
  flag."

There is **no jieba, no pypinyin, no statistical segmenter** anywhere in the eSpeak-NG build. The
build's dictionary list is a plain enumeration of `<lang>_list`/`<lang>_rules` names — see
<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/cmake/data.cmake> (verbatim excerpt):

```
list(APPEND _dict_compile_list
  ab af am an ar as az
  ...
  ca chr cmn crh cs cv cy
  ...
  yue
)
```

Adding a jieba-style segmenter was proposed *outside* eSpeak-NG, for Piper, and was closed
"not planned": <https://github.com/rhasspy/piper/issues/164> — title "Better Chinese phonemization"
(state: closed, `"state_reason":"not_planned"`, closed 2024-09-14), body verbatim:
> "I'm working on applying some other word segmentation module like jieba to piper-phonemize,
> because **the Chinese dict in espeak-ng is far too small and rigid.** I want to know if espeak-ng
> can keep my splits (seems espeak-ng may remove all the space in chinese text before dicting
> Pinyin). Or should I write a new character to phoneme module?"

#### (d) The multi-word dictionary path is itself reported broken and is still open

<https://github.com/espeak-ng/espeak-ng/issues/606> — "Chinese dictionary multiple match"
(state: **open**, opened 2019-04-09 by `rongcuid`, 3 comments, last updated 2020-07-16). Verbatim
from the body:
> "If dictionary defines word rules, such as `(地 面) di4mian4`, the characters are matched multiple
> times:"
>
> "I already tried my best to isolate the problem, and I think that the problem comes from
> `dictionary.c:LookupDict2`, which sets the global variable `dictionary_skipwords`. In a GDB
> session, I notice that `dictionary_skipwords` is set to 1, instead of an expected 2, which means
> that each character would be looked up and translated."
>
> "Note: I worked on my fork, which has a minor change to load `listx` after `list` … That is done
> so that zh_listx is actually loaded. In this repo, **zh_listx is not loaded and has no effect**."

(The last sentence describes the repository state in 2019, i.e. before the `zh`→`cmn` rename and
before the current CMake data rules of §2.6.)

### 2.6 Is `espeak-ng-data/cmn_dict` in the repo, or generated at build time?

**Generated at build time. It is not committed.**

- `https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/cmn_dict` returns
  **HTTP 404** (probed directly).
- `docs/dictionary.md` documents the compilation step:
  > "These files are compiled into the file `<language>_dict` in the espeak-ng-data
  > directory (e.g. `espeak-ng-data/en_dict`)."
  Source: <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/dictionary.md>
- The build rules confirm it. From
  <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/cmake/data.cmake> (verbatim):
  > `set(DATA_DIST_DIR ${DATA_DIST_ROOT}/espeak-ng-data)`
  > `set(DICT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dictsource)`
  > `set(_dict_target "${DATA_DIST_DIR}/${_dict_name}_dict")`
  > `COMMAND ${ESPEAK_RUN_CMD} --compile=${_dict_name}`

  and, critically for the extended Mandarin dictionary:
  ```
  if(EXISTS "${DICT_SRC_DIR}/extra/${_dict_name}_listx")
      option(EXTRA_${_dict_name} "Compile extra ${_dict_name} dictionary" ON)
      if(EXTRA_${_dict_name})
        list(APPEND _dict_deps "${DICT_SRC_DIR}/extra/${_dict_name}_listx")
  ```
  Because `dictsource/extra/cmn_listx` exists, CMake defines option **`EXTRA_cmn`, defaulting ON**.
  So a default CMake build **does** compile the 21,611-character + 36,500-compound extended
  Mandarin dictionary into `${build}/espeak-ng-data/cmn_dict`.
- Only the *source* voice data is committed: `espeak-ng-data/lang` (and `espeak-ng-data/voices/!v`)
  are copied into the distribution directory by the same file
  (`file(COPY "${DATA_SRC_DIR}/lang" DESTINATION "${DATA_DIST_DIR}")`).

Additional context on the extended dictionary being optional in older autotools builds: a user's
configure line in <https://github.com/espeak-ng/espeak-ng/issues/1665> reads
`--without-extdict-ru --without-extdict-cmn --without-extdict-yue`.

### 2.7 Independent evidence that tones are actually realized (user-observable traces)

- <https://github.com/espeak-ng/espeak-ng/issues/364> — "How to make tone sandhi works like Mandarin
  Chinese." (state: **closed**; labels `bug` + `resolved/fixed`; milestone `1.49.3`). The reporter
  (`Yoxem`, association CONTRIBUTOR) quotes an actual `espeak -v zh "雨傘" -x -X` trace and writes:
  > "However, the tone sandhi of Mandarin (the tone of tone-214 syllable in front of another
  > tone-214 tone is changed to tone-35) works. eg 雨傘 (yu214 san214 -> yu35 san214)"
  >
  > "Even though the tone sandhi rule is not defined in zh_list or zh_rules, the tone-sandhi rule
  > works."

  The quoted trace ends `;'y35_| s'a214n_|` with the inline comment `#'y214 is changed to 'y35 automatically`.
- <https://github.com/espeak-ng/espeak-ng/issues/815> — "Wrong tone sandhi" (state: **open**,
  0 comments, opened 2020-08-26 by `ferrumcccp`). Body verbatim:
  > ```
  > $ espeak -q -vzh -x "展览馆，好旅馆" # Which means "exhibition hall, good hotel"
  > ts.'a35n_| l'a35n_| kw'a214n_|
  > X'Au35_| l'y35_| kw'a214n_|
  > ```
  > "The correct pronunciation should be: … X'Au214_| … Because "好旅馆" is two words and tone
  > sandhi should be applied separately."
- <https://github.com/espeak-ng/espeak-ng/issues/1275> — "Conversion from text to IPA phonemes seems
  incorrect when phonemes include tone changes" (state: **open**, 2 comments, opened 2022-08-10).
  Body verbatim: "The IPA transcription result seems wrong for the languages with tone changes
  (e.g., Chinese mandarin)." … "the tone changes are represented as `5, none, 2, 5` for each
  character respectively." … "Here, the system correctly identified the same vowel [A] for all
  characters and accurately distinguished tone changes. So, I think the problem is the conversion
  script for IPA transcription."
- <https://github.com/rhasspy/piper/issues/305> — "When espeak-ng translates Chinese (cmn), IPA tone
  symbols are not output correctly" (state: **open**, 3 comments, opened 2023-12-11 by `yzznw`).
  Body verbatim:
  > "In "dictionary.c", in function "WritePhMnemonic" code segment: `if (!first && IsDigit09(c)) continue;` it's cause Q1 problem. If I remove this, output seems OK."
  >
  > "In chinese there are 5 tones by number: 1 to 5. mapped to IPA in espeak-ng is : 1(55), 2(35), 3(214), 4(51), 5(11). But in huayan model, tone 1 sometimes sounds like tone 2, and tone 4 sometimes sounds like tone 1."

---

## 3. Concrete GitHub issues about Chinese/Mandarin pronunciation (exact numbers, titles, state)

Method: GitHub search API, e.g.
<https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+chinese+in:title&sort=created&order=desc&per_page=50>
(19 results), `…/search/issues?q=repo:espeak-ng/espeak-ng+mandarin` (50 results),
`…/search/issues?q=repo:espeak-ng/espeak-ng+cmn` (48 results),
`…/search/issues?q=repo:espeak-ng/espeak-ng+pinyin` (15 results),
`…/search/issues?q=repo:espeak-ng/espeak-ng+tone+in:title` (16 results),
`…/search/issues?q=repo:espeak-ng/espeak-ng+多音字` (1 result),
`…/search/issues?q=repo:espeak-ng/espeak-ng+polyphone` (1 result),
`…/search/issues?q=repo:rhasspy/piper+chinese` (19), `…/search/issues?q=repo:OHF-Voice/piper1-gpl+chinese` (16),
`…/search/issues?q=repo:OHF-Voice/piper1-gpl+g2pw` (8), `…/search/issues?q=repo:OHF-Voice/piper1-gpl+polyphone` (2).
All states below are the `state` field returned by the API at fetch time.

### 3.1 eSpeak-NG repository

| # | Full title | State | URL | Notes / maintainer signal |
|---|---|---|---|---|
| 606 | Chinese dictionary multiple match | **open** | <https://github.com/espeak-ng/espeak-ng/issues/606> | Reports multi-word entry `(地 面) di4mian4` consumes 1 word not 2 (`dictionary_skipwords`); "zh_listx is not loaded and has no effect". 3 comments; no fix. |
| 815 | Wrong tone sandhi | **open** | <https://github.com/espeak-ng/espeak-ng/issues/815> | 好旅馆 → `X'Au35` instead of `X'Au214`. 0 comments, no response. |
| 257 | Mistake in pronounced of 9 in Mandarin Chinese | **open** | <https://github.com/espeak-ng/espeak-ng/issues/257> | Labels `bug` + `languages/pronunciation`; 12 comments; last updated 2019-09-27. Not fixed. |
| 1805 | Mandarin Pinyin issue | **open** | <https://github.com/espeak-ng/espeak-ng/issues/1805> | 2 comments. (This is the only espeak-ng issue the API returns for the queries `多音字` and `polyphone`, i.e. those terms appear in its comments — comment text not retrieved, see §5.) |
| 2151 | Does it support mixed Chinese and English streaming mode? | **open** | <https://github.com/espeak-ng/espeak-ng/issues/2151> | Body empty; 0 comments. |
| 1851 | it says "Chinese Letter" | **open** | <https://github.com/espeak-ng/espeak-ng/issues/1851> | 14 comments; last updated 2025-11-19. Han text read as "Chinese letter". |
| 1831 | Cantonese did not return as international phonetic alphabet (IPA), but jyutping instead. (with --ipa) | **open** | <https://github.com/espeak-ng/espeak-ng/issues/1831> | — |
| 1275 | Conversion from text to IPA phonemes seems incorrect when phonemes include tone changes | **open** | <https://github.com/espeak-ng/espeak-ng/issues/1275> | Tone numerals mangled in IPA output. |
| 2160 | Using mb-cn1 does not work well !!! | **open** | <https://github.com/espeak-ng/espeak-ng/issues/2160> | — |
| 338 | eSpeak-ng 1.49.2: Chinese Mandarin dictionary (zhy_dict) problem | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/338> | Labels `bug` + `resolved/fixed`, milestone 1.49.3. Fixed by PR #340. |
| 340 | Fix for issue #338: Chinese Cantonese dictionary (zhy_dict) problem | **closed (merged 2017-11-26)** | <https://github.com/espeak-ng/espeak-ng/pull/340> | Maintainer `valdisvi` (MEMBER). |
| 346 | Big bug in Mandarin Chinese | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/346> | Label `resolved/duplicate`. |
| 347 | Chinese Mandarin: English words pronunciation problem | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/347> | Closed 2022-09-24. |
| 348 | Bug around Chinese Mandarin in Espeak has been discovered | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/348> | Label `resolved/not-an-espeakng-bug`. |
| 361 | can't split the syllables accurately & show the tone number | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/361> | Closed same day (2017-12-29). |
| 364 | How to make tone sandhi works like Mandarin Chinese. | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/364> | Labels `bug` + `resolved/fixed`; milestone 1.49.3. |
| 499 | The Mandarin Chinese language cannot read the Chinese characters | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/499> | — |
| 664 | the  Chinese  pronunciation  is wrong | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/664> | — |
| 685 | Voice Chinese (Mandarin): some characters are reported two times | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/685> | — |
| 738 | Improves on some Chinese pronunciation | **closed (not merged)** | <https://github.com/espeak-ng/espeak-ng/pull/738> | Body: "I improved part of the the Chinese dictionaries and their priorities, fixing some common unclear/wrong pronunciations. These are not comprehensive, just some I discovered when using espeak-ng for my project." Not merged. |
| 788 | cmn: use voiceless uvular fricative as pronounciation of pinyin h | **closed (merged 2020-07-05)** | <https://github.com/espeak-ng/espeak-ng/pull/788> | — |
| 933 | Unable to compile zhy dictionay on Windows | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/933> | 15 comments; led to PR #940. |
| 940 | Rename zh and zhy to cmn and yue | **closed (merged 2021-05-16)** | <https://github.com/espeak-ng/espeak-ng/pull/940> | "There are no tests for Cantonese… Checks for Mandarin pass." |
| 1028 | The pronunciation of Mandarin Chinese using ESpeak NJ in NVDA is not normal | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/1028> | Tones read aloud as English numbers ("今One 天 One 的 Five…"). 10 comments. |
| 1044 | questions about mandarin data packet | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/1044> | Reports `Full dictionary is not installed for 'zh'` / `Error processing file 'zh_rules': No such file or directory` while `cmn_rules`/`cmn_list` exist. 19 comments. |
| 1163 | Please default ESpeak NG's voice role to "Chinese (Mandarin, latin as Pinyin)" for Chinese to fix #1028 | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/1163> | "#1028 makes ESpeak completely unusable for Chinese users." |
| 1236 | Recognition of Chinese characters Error | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/1236> | `-v cmn "好"` sounds wrong. |
| 1370 | Cmn voice not correctly translated | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/1370> | — |
| 1404 | fix #1370 Cmn voice not correctly translated | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/1404> | — |
| 1669 | Add _dpt pronounce for cmn_list(chinese), But didn't work | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/1669> | "Decimal points are not supported in dictsource/cmn_list (Chinese)"; `_dpt dian3` had no effect. |
| 1670 | feat: supporting speak decimal representation in Mandarin | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/1670> | Follow-up to #1669. |
| 1793 | dictrules for cmn is broken | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/1793> | — |
| 2511 | fix(ar): fallback Chinese characters to Mandarin (zh) pronunciation | **closed (merged 2026-09-01)** | <https://github.com/espeak-ng/espeak-ng/pull/2511> | Hanzi in Arabic text previously read as "Chinese letter". |
| 2455 | translate: segment $textmode replacement text like normal input | **closed (merged 2026-07-22)** | <https://github.com/espeak-ng/espeak-ng/pull/2455> | Maintainer-authored (MEMBER). Confirms hanzi→separate-word + nested `$textmode` hanzi→pinyin path. |
| 2520 | mn: rebuild the Mongolian voice against a measured speech corpus | **open** | <https://github.com/espeak-ng/espeak-ng/pull/2520> | Describes the Han-not-split `alt_alphabet` bug. |
| 2524 | Fix stale pitch=82 default in formant_factor calculation | **open** | <https://github.com/espeak-ng/espeak-ng/pull/2524> | Lists `cmn`, `cmn-Latn-pinyin` among voices with explicit `pitch`. |
| 1665 | Error trying to build latest stable release (espeak-ng 1.51) | **closed** | <https://github.com/espeak-ng/espeak-ng/issues/1665> | Contains `--without-extdict-cmn` configure flag evidence. |

### 3.2 `rhasspy/piper`

| # | Full title | State | URL | Notes |
|---|---|---|---|---|
| 164 | Better Chinese phonemization | **closed** | <https://github.com/rhasspy/piper/issues/164> | `"state_reason":"not_planned"`, closed 2024-09-14. "the Chinese dict in espeak-ng is far too small and rigid". 8 comments. |
| 305 | When espeak-ng translates Chinese (cmn), IPA tone symbols are not output correctly | **open** | <https://github.com/rhasspy/piper/issues/305> | 3 comments. Detailed tone-numeral report (213→2, 51→5) and points at `dictionary.c:WritePhMnemonic`. |
| 278 | More natural Chinese voice, Please | **open** | <https://github.com/rhasspy/piper/issues/278> | "Chinese has an English accent and seems unnatural. The segmentation of sentence pauses feels a bit mechanical." |
| 243 | incorrect output for simple Chinese phrase? | **open** | <https://github.com/rhasspy/piper/issues/243> | `一点儿` renders incorrectly. 1 comment. |
| 835 | Error pause for Chinese | **open** (reopened) | <https://github.com/rhasspy/piper/issues/835> | Pause placement wrong for Chinese text. |
| 652 | Training a new model, hoping to receive assistance | **closed** | <https://github.com/rhasspy/piper/issues/652> | `"state_reason":"not_planned"`. "the pronunciation tone of the Piper project's Chinese model is incorrect". |
| 505 | Is the Chinese language code currently using cmn or zh_cN? | (state not retrieved — see §5) | <https://github.com/rhasspy/piper/issues/505> | — |
| 9 | How to convert the vits model to onnx? | **closed** | <https://github.com/rhasspy/piper/issues/9> | Body: "or Chinese support thx". |

### 3.3 `OHF-Voice/piper1-gpl` (current Piper)

| # | Full title | State | URL | Notes |
|---|---|---|---|---|
| 271 | Add Chinese pinyin support – Phase 1 (monophonic dict fallback, honest scoping) | **closed (merged 2026-08-22)** | <https://github.com/OHF-Voice/piper1-gpl/pull/271> | Adds a **non-espeak** Chinese pinyin path (g2pW). Verbatim: "Polyphonic chars (`重/行/长/好` etc.) treated as unsupported → empty, preventing silent mis-assignment of `重庆/银行/长江`." Approved by `yangfan-yf-yf`. |
| 289 | fix: Phase 1 relaxed poly fallback for mobile | **closed (merged 2026-08-29)** | <https://github.com/OHF-Voice/piper1-gpl/pull/289> | Verbatim: "Change phonemize() to use first reading for poly chars instead of failing … 95% coverage, tradeoff: 重庆 may pick first sense until Phase 2, acceptable vs silence". |
| 269 | Run g2pW under onnxruntime without torch | **closed (merged 2026-08-13)** | <https://github.com/OHF-Voice/piper1-gpl/pull/269> | Verbatim: "Separately, output was compared against upstream `g2pw.G2PWConverter` on the same `g2pw.onnx`: identical results across 19 sentences (polyphone disambiguation, mixed ASCII/digits, …)". |
| 153 | Add Chinese phonemization using g2pW | **closed (not merged)** | <https://github.com/OHF-Voice/piper1-gpl/pull/153> | Predecessor of #271. |
| 158 | Integrate goruut/pygoruut phonemizer | **open** | <https://github.com/OHF-Voice/piper1-gpl/issues/158> | Publishes a per-language table including `chinese/mandarin \| zh \| 9% \| 83% \| 8% \| 83%` (word/char success rates). Third-party measurement, not an eSpeak-NG measurement. |
| 128 | Some prosody-relevant punctuation chars are dropped by espeakbridge | **open** | <https://github.com/OHF-Voice/piper1-gpl/issues/128> | Verbatim: "…and _all_ Chinese punctuation is dropped, so Chinese paragraphs just become a single run-on sentence." |
| 134 | Submission Process for a New zh_CN TTS Model | **open** | <https://github.com/OHF-Voice/piper1-gpl/issues/134> | 16 comments. |
| 29 | how to solve the error pause for Chinese | **open** | <https://github.com/OHF-Voice/piper1-gpl/issues/29> | Same complaint as rhasspy/piper#835. |
| 205 | Use pypinyin as a fallback for g2pw | **open** | <https://github.com/OHF-Voice/piper1-gpl/pull/205> | Assignee `synesthesiam` (Piper maintainer). |
| 272 | (Phase 2: full contextual polyphone disambiguation via g2pw BERT ONNX) | referenced, title/state not retrieved — see §5 | <https://github.com/OHF-Voice/piper1-gpl/pull/272> | Referenced verbatim from #271: "Full contextual polyphone disambiguation (g2pw BERT ONNX `g2pw.onnx`) deferred to #272." |

**Maintainer responses captured:** the strongest verbatim maintainer statements are in
<https://github.com/espeak-ng/espeak-ng/pull/2455> (eSpeak-NG maintainer `alex19EP`, MEMBER) and
<https://github.com/OHF-Voice/piper1-gpl/pull/271> / <https://github.com/OHF-Voice/piper1-gpl/pull/289>
(Piper maintainer team). Maintainer comment threads on
<https://github.com/espeak-ng/espeak-ng/issues/606>, `/815`, `/1805` and
<https://github.com/rhasspy/piper/issues/164> were **not retrieved** — see §5.

---

## 4. Published comparisons of eSpeak-NG Mandarin quality vs pinyin-dictionary front-ends

- **eSpeak-NG's own docs:** no comparison, and no quality statement about Mandarin at all.
  `docs/languages.md` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages.md>)
  and `docs/index.md` (<https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/index.md>)
  contain none; there is no `docs/languages/sit/cmn.md` (404).
- **eSpeak-NG `ChangeLog.md`:** the file is referenced by the README
  ("See the [ChangeLog](ChangeLog.md) for a description of the changes in the various releases")
  — <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md> — and a root
  `CHANGELOG.md` does not exist (HTTP 404 at
  <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/CHANGELOG.md>). The actual
  `ChangeLog.md` could not be downloaded in this environment (fetch failed / timed out), so **any
  Chinese-quality statement inside `ChangeLog.md` is not determinable from the sources retrieved here.**
- **A credible, quotable comparative statement does exist, but it is in the downstream Piper
  project's bug tracker, not in an eSpeak-NG document:**
  - <https://github.com/rhasspy/piper/issues/164> (closed, `not_planned`): "**the Chinese dict in
    espeak-ng is far too small and rigid**" and the reporter's plan to substitute a jieba
    word-segmentation module.
  - <https://github.com/OHF-Voice/piper1-gpl/pull/271> (merged): Piper replaced espeak-ng for
    Chinese with a g2pW/BERT polyphone-disambiguation front-end, describing the espeak-ng path's
    failure mode as "silent mis-assignment of `重庆/银行/长江`".
  - <https://github.com/OHF-Voice/piper1-gpl/pull/289> (merged): "Phase 1 strict mono returned empty
    for any poly char (虹/称/绛/简/重/行/长), causing long sentences like "彩虹，又称天弓…" to be
    empty -> PIPER_ERR_GENERIC -> silent in piper-app iOS/macOS." and the accepted trade-off
    "重庆 may pick first sense until Phase 2".
  - <https://github.com/OHF-Voice/piper1-gpl/issues/158> publishes a third-party success-rate table
    row for `chinese/mandarin | zh | 9% | 83%` (word / char success), but this is a comparison of
    pygoruut against *Piper models*, **not** an eSpeak-NG Mandarin quality measurement.

**Answer for Q4:** No primary eSpeak-NG document compares espeak-ng Mandarin quality to
pinyin-dictionary front-ends. The available comparative statements are downstream (Piper) and are
attributed above. No published, first-party numeric quality metric for eSpeak-NG Mandarin was found
— **not published**.

---

## 5. Explicit gaps / limitations of this research

1. **Compound section of `dictsource/extra/cmn_listx`** (the 36,500 compound exceptions): the raw
   file is served from byte 0 and exceeded the fetch display limit; 14,115 leading single-character
   entries were retrieved, all in Unicode order. The literal compound lines and the file's exact
   byte size were **not** retrieved. (A directory listing of `dictsource/extra` would give the size;
   it was blocked by the anonymous GitHub REST `core` rate limit, which resets at
   2026-09-10T14:39:31Z.)
2. **Exact line counts** for `cmn_list` (header states 3799 characters) and `cmn_listx` (header
   states 21611 + 36500) were not independently counted.
3. **Comment bodies** (i.e. maintainer replies) for issues
   <https://github.com/espeak-ng/espeak-ng/issues/606>,
   <https://github.com/espeak-ng/espeak-ng/issues/815>,
   <https://github.com/espeak-ng/espeak-ng/issues/1805>,
   <https://github.com/rhasspy/piper/issues/164> and the state of
   <https://github.com/rhasspy/piper/issues/505> and
   <https://github.com/OHF-Voice/piper1-gpl/pull/272> were not retrieved: the required REST
   endpoints are on the rate-limited `core` bucket, and GitHub HTML pages did not fetch in this
   environment.
4. **`ChangeLog.md`** could not be downloaded (see §4).
5. Items 1–4 are marked in-line above; everything else in this report was read directly from the
   raw file, API response, or PR/issue body whose URL is given.

---

## 6. Complete list of URLs used

Repository / API metadata
- <https://api.github.com/repos/espeak-ng/espeak-ng>
- <https://api.github.com/repos/espeak-ng/espeak-ng/contents/dictsource>
- <https://api.github.com/rate_limit>

eSpeak-NG documentation and source data
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages.md>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/index.md>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/dictionary.md>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/README.md>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/CMakeLists.txt>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/cmake/data.cmake>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/lang/sit/cmn>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_list>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_rules>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/extra/cmn_listx>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/yue_list>
- 404 probes: <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/zh_listx>,
  <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/zh_rules>,
  <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/zh_pinyin>,
  <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/cmn_listx>,
  <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/dictsource/extra/zh_listx>,
  <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/espeak-ng-data/cmn_dict>,
  <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/docs/languages/sit/cmn.md>,
  <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/CHANGELOG.md>
- <https://raw.githubusercontent.com/espeak-ng/espeak-ng/master/ChangeLog.md> (referenced by README; fetch failed here)

GitHub search API queries used
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+chinese+in:title&sort=created&order=desc&per_page=50>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+mandarin&sort=created&order=desc&per_page=50>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+cmn&sort=created&order=desc&per_page=30>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+pinyin&sort=created&order=desc&per_page=30>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+tone+in:title&sort=created&order=desc&per_page=30>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+%E5%A4%9A%E9%9F%B3%E5%AD%97&per_page=20>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+polyphone&per_page=20>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+%22Chinese+dictionary+multiple+match%22&per_page=3>
- <https://api.github.com/search/issues?q=repo:espeak-ng/espeak-ng+%22Wrong+tone+sandhi%22&per_page=3>
- <https://api.github.com/search/issues?q=repo:rhasspy/piper+chinese&sort=created&order=desc&per_page=30>
- <https://api.github.com/search/issues?q=repo:OHF-Voice/piper1-gpl+chinese&sort=created&order=desc&per_page=30>
- <https://api.github.com/search/issues?q=repo:OHF-Voice/piper1-gpl+g2pw&per_page=20>
- <https://api.github.com/search/issues?q=repo:OHF-Voice/piper1-gpl+polyphone&per_page=20>

Individual issues / pull requests
- <https://github.com/espeak-ng/espeak-ng/issues/257>, /338, /346, /347, /348, /361, /364, /499,
  /606, /664, /685, /815, /933, /1028, /1044, /1163, /1236, /1275, /1370, /1404, /1580, /1665,
  /1669, /1670, /1793, /1805, /1831, /1851, /2151, /2160
- <https://github.com/espeak-ng/espeak-ng/pull/340>, /738, /788, /940, /2455, /2511, /2520, /2524
- <https://github.com/rhasspy/piper/issues/9>, /164, /243, /278, /305, /505, /652, /835
- <https://github.com/OHF-Voice/piper1-gpl/issues/29>, /128, /134, /158
- <https://github.com/OHF-Voice/piper1-gpl/pull/153>, /205, /269, /271, /272 (referenced), /289
