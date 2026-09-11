# Offline Mandarin TTS on Android: `android.speech.tts` vs Qt TextToSpeech — primary-source research

Scope: Qt 6.11.1 / C++17 / CMake / arm64-v8a / Android 9+ (API 28), currently no Java/JNI/Gradle code.
Facts only, every bullet carries a URL. Where a number or fact is not published, this report says
"not published". Where a source could not be reached from this environment, it says
"not determinable from public sources" and names the host that was blocked.

## 0. Verification environment (read this before trusting any "not determinable")

- `developer.android.com`, `source.android.com`, `support.google.com`, `android.googlesource.com`,
  `play.google.com` and `web.archive.org` were **unreachable** from this sandbox (`web_fetch`
  returned `fetch failed`; the shell has no outbound network). Consequences:
  - The per-member "Added in API level N" annotations on developer.android.com could **not** be read.
    API levels below are therefore established by **diffing AOSP source tags** (presence at tag A,
    absence at tag B) instead of by quoting the doc annotation.
  - Google-hosted pages about installing voice data, and Play Store listings, could not be read.
- Hosts that **were** reachable: `doc.qt.io`, `code.qt.io`, `wiki.qt.io`, `www.qt.io`,
  `github.com`, `cdn.jsdelivr.net` (used to read AOSP sources from the `aosp-mirror` GitHub
  organisation), and `developer.android.google.cn` (the official Google-run Chinese mirror of
  developer.android.com — the reference pages there return HTTP 200 but the tool could not extract
  their body text, so they yielded no usable content).
- AOSP code below is cited at the mirror URL actually fetched, with the upstream repository and
  git tag named. Canonical browser URL form:
  `https://android.googlesource.com/platform/frameworks/base/+/refs/heads/master/core/java/android/speech/tts/<File>.java`
  — [AOSP framework base tree](https://android.googlesource.com/platform/frameworks/base/) (host unreachable here).

---

## Topic A — Android system TextToSpeech (`android.speech.tts.TextToSpeech`)

### A1. Does it work fully offline? What do the official APIs say about network requirements?

- The `TextToSpeech` class documentation states only what the class does — "Synthesizes speech from
  text for immediate playback or to create a sound file" — and gives **no** blanket offline
  guarantee; it requires an engine to be installed and initialized:
  [TextToSpeech.java, AOSP master](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
  (also mirrored at `developer.android.com/reference/android/speech/tts/TextToSpeech`, unreachable here).
- Offline synthesis is expressed as an **engine-declared feature**, not a framework guarantee:
  `TextToSpeech.Engine.KEY_FEATURE_EMBEDDED_SYNTHESIS` = `"embeddedTts"` — "If set and supported by
  the engine as per `TextToSpeech#getFeatures(Locale)`, the engine must synthesize text on-device
  (without making network requests)." It is marked `@Deprecated` since API level 21:
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- The network counterpart: `KEY_FEATURE_NETWORK_SYNTHESIS` = `"networkTts"` — "If set (and supported
  by the engine …), the engine must use network based synthesis"; also `@Deprecated` since API level 21:
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- The deprecation text gives the replacement API explicitly: "Starting from API level 21, to select
  network synthesis, call `getVoices()`, find a suitable network voice
  (`Voice#isNetworkConnectionRequired()`) and pass it to `setVoice(Voice)`":
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- The `Voice` class answers the offline question per voice:
  `public boolean isNetworkConnectionRequired()` — "Does the Voice require a network connection to work."
  Backed by a `mRequiresNetworkConnection` field set only via the `Voice(String, Locale, int, int,
  boolean, Set<String>)` constructor, i.e. it is reported by the engine:
  [Voice.java, AOSP master](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/Voice.java)
  (also documented at `developer.android.com/reference/android/speech/tts/Voice`, unreachable here).
- `Voice` also declares expected-latency labels that describe the network case:
  `LATENCY_HIGH` — "Network based expected synthesizer latency (~200ms)"; `LATENCY_VERY_HIGH` —
  "Very slow network based expected synthesizer latency (> 200ms)"; `LATENCY_VERY_LOW` — "< 20ms";
  `LATENCY_LOW` — "~20ms"; `LATENCY_NORMAL` — "~50ms":
  [Voice.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/Voice.java)
- Framework error codes confirm the network path exists and can fail:
  `ERROR_NETWORK = -6` ("Denotes a failure caused by a network connectivity problems"),
  `ERROR_NETWORK_TIMEOUT = -7`:
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- `TextToSpeech.getFeatures(Locale)` returns "A set of strings containing the features of the voice
  in the given locale. The features will be a subset of the features returned by `Voice#getFeatures()`,
  or null on error":
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- AOSP's own Settings code decides "is this engine going to need the network?" with exactly that
  feature pair — network-required means "has `networkTts` **and not** `embeddedTts`":
  `return features.contains(TextToSpeech.Engine.KEY_FEATURE_NETWORK_SYNTHESIS) && !features.contains(TextToSpeech.Engine.KEY_FEATURE_EMBEDDED_SYNTHESIS);`
  and shows the `tts_engine_network_required` alert, logging "Network required for sample synthesis
  for requested language":
  [TextToSpeechSettings.java, android-9.0.0_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-9.0.0_r1/src/com/android/settings/tts/TextToSpeechSettings.java)
  (same logic in [android-5.1.1_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-5.1.1_r1/src/com/android/settings/tts/TextToSpeechSettings.java))
- Net statement supported by sources: the framework can drive a **fully offline** engine, but only if
  the installed engine exposes a voice with `isNetworkConnectionRequired() == false` (historically:
  `embeddedTts` without `networkTts`). No AOSP source read for this report asserts that any
  particular shipping engine is offline.
- Manifest consequence for API 30+ (relevant since the app targets Android 9+, but affects builds
  targeting 30+): "Apps targeting Android 11 that use text-to-speech should declare
  `TextToSpeech.Engine#INTENT_ACTION_TTS_SERVICE` in the `queries` elements of their manifest" —
  `<queries><intent><action android:name="android.intent.action.TTS_SERVICE" /></intent></queries>`:
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)

### A2. Is voice/engine data required to be pre-downloaded by the user?

- The framework defines an explicit install-voice-data intent:
  `Engine.ACTION_INSTALL_TTS_DATA` = `"android.speech.tts.engine.INSTALL_TTS_DATA"` — "Triggers the
  platform TextToSpeech engine to start the activity that installs the resource files on the device
  that are **required for TTS to be operational**. Since the installation of the data can be
  interrupted or declined by the user, the application shouldn't expect successful installation upon
  return from that intent, and if need be, should check installation status with
  `ACTION_CHECK_TTS_DATA`":
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- The check intent returns availability lists: `ACTION_CHECK_TTS_DATA` =
  `"android.speech.tts.engine.CHECK_TTS_DATA"`, with result codes `CHECK_VOICE_DATA_PASS = 1` /
  `CHECK_VOICE_DATA_FAIL = 0`, extras `EXTRA_AVAILABLE_VOICES` and `EXTRA_UNAVAILABLE_VOICES`
  ("all the unavailable voices (**ones that user can install**)"), and a broadcast
  `ACTION_TTS_DATA_INSTALLED` = `"android.speech.tts.engine.TTS_DATA_INSTALLED"`:
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- Runtime signal that language data is absent: `LANG_MISSING_DATA = -1` ("Denotes the language data
  is missing"), alongside `LANG_NOT_SUPPORTED = -2`, returned by `isLanguageAvailable(Locale)` /
  `setLanguage(Locale)`:
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- Per-voice download state is a declared engine feature:
  `KEY_FEATURE_NOT_INSTALLED` = `"notInstalled"` — "Feature key that indicates that the voice may
  need to download additional data to be fully functional. **The download will be triggered by
  calling `setVoice(Voice)` or `setLanguage(Locale)`.** Until download is complete, each synthesis
  request will either report `ERROR_NOT_INSTALLED_YET` error, or use a different voice to synthesize
  the request. This feature should NOT be used as a key of a request parameter." Corresponding error
  code `ERROR_NOT_INSTALLED_YET = -9` ("Denotes a failure caused by an unfinished download of the
  voice data"):
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- History of the user-facing "install voice data" affordance in AOSP Settings: in Android 4.4 the
  per-engine settings screen has `KEY_INSTALL_DATA = "tts_install_data"`, enables that preference
  only when `EXTRA_UNAVAILABLE_VOICES` is non-empty ("if (unavailable != null && unavailable.size() > 0)
  mInstallVoicesPreference.setEnabled(true)"), and its handler documents itself as "Ask the current
  default engine to launch the matching INSTALL_TTS_DATA activity so the required TTS files are
  properly installed", starting `TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA`:
  [TtsEngineSettingsFragment.java, android-4.4_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-4.4_r1/src/com/android/settings/tts/TtsEngineSettingsFragment.java)
- In the Android 9 (API 28) AOSP Settings source this install-data preference is **gone**; the
  screen checks voice data with `ACTION_CHECK_TTS_DATA` and reaches engine-owned UI through
  `TtsEngines.getSettingsIntent(engine)` (the "gear" click):
  [TextToSpeechSettings.java, android-9.0.0_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-9.0.0_r1/src/com/android/settings/tts/TextToSpeechSettings.java)
  and [TtsEnginePreference.java, android-9.0.0_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-9.0.0_r1/src/com/android/settings/tts/TtsEnginePreference.java)
  (contrast the 4.4 version, which contains the install-data path:
  [TtsEnginePreference.java, android-4.4_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-4.4_r1/src/com/android/settings/tts/TtsEnginePreference.java))
  — i.e. on API 28 the per-engine voice download is delegated to whatever UI the engine app ships.
- Device with **no** engine installed: engines are discovered by resolving a service intent, so with
  none installed `getEngines()` returns an empty list and the "highest ranked system engine" is
  `null`; `getDefaultEngine()` returns the value of `Settings.Secure.TTS_DEFAULT_SYNTH` only if
  `isEngineInstalled(engine)`, otherwise `getHighestRankedEngineName()`, i.e. `null` when no system
  engine exists:
  [TtsEngines.java, AOSP master](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TtsEngines.java)
- AOSP Settings additionally hides the TTS settings page entirely when no engine resolves:
  `isPageSearchEnabled(Context)` returns `!ttsEngines.getEngines().isEmpty() && context.getResources().getBoolean(R.bool.config_show_tts_settings_summary)`:
  [TextToSpeechSettings.java, android-9.0.0_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-9.0.0_r1/src/com/android/settings/tts/TextToSpeechSettings.java)
- Google's own product documentation for "Speech Services by Google" / "Install voice data"
  (support.google.com, play.google.com) could **not** be read here — **not determinable from public
  sources reachable in this environment**. The claim "Google's engine downloads Mandarin voice data
  on demand" is therefore **not verified** by this report.
- Whether any specific engine (including `com.google.android.tts`) is present on a device without
  Google Play Services: **not determinable from public sources** read for this report (the engine is
  distributed outside AOSP; no AOSP source names it).

### A3. Which engines implement `android.speech.tts.TextToSpeech`?

- Registration mechanism (framework side, authoritative): an engine is any app exposing a **Service**
  that handles `TextToSpeech.Engine.INTENT_ACTION_TTS_SERVICE` (`"android.intent.action.TTS_SERVICE"`).
  Discovery is `pm.queryIntentServices(new Intent(Engine.INTENT_ACTION_TTS_SERVICE), MATCH_DEFAULT_ONLY)`;
  "the 'engine name' is the same as the package name", and "the current API allows only one engine
  per package name". Optional meta-data `TextToSpeech.Engine.SERVICE_META_DATA` (`"android.speech.tts"`)
  points at an XML resource whose tag must be `tts-engine` and which may declare `settingsActivity`:
  [TtsEngines.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TtsEngines.java),
  with the tag/attribute names also stated in
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- Engine ranking, quoted: "Engines that are a part of the system image are always lesser than those
  that are not. Within system engines / non system engines the engines are sorted in order of their
  declared priority." (higher `priority` sorts first):
  [TtsEngines.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TtsEngines.java)
- AOSP's deprecated default-engine constant names the historical reference engine:
  `DEFAULT_ENGINE = "com.svox.pico"` (`@hide`, `@Deprecated`: "No longer in use, the default engine is
  determined by the sort order defined in `TtsEngines`"):
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- Concrete third-party engines, incl. Google Speech Services (`com.google.android.tts`), Samsung TTS,
  eSpeak-based engines, and Chinese vendors (iFlytek/讯飞, Baidu): **surveyed in section A3b below.**
- The AOSP in-repo reference engine (`com.svox.pico`) is **not** available at the mirror path that
  would normally hold it — `github.com/aosp-mirror/platform_packages_apps_PicoTts` returns HTTP 404:
  [github.com/aosp-mirror/platform_packages_apps_PicoTts](https://github.com/aosp-mirror/platform_packages_apps_PicoTts),
  [jsDelivr AndroidManifest.xml](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_PicoTts@master/AndroidManifest.xml)
- Concrete engine implementation, verified from source — **eSpeak NG**: its Android manifest registers
  exactly the standard contract, `<service android:name=".TtsService" android:directBootAware="true"
  android:exported="true">` with `<action android:name="android.intent.action.TTS_SERVICE" />` +
  `DEFAULT` category and `<meta-data android:name="android.speech.tts" android:resource="@xml/tts_engine" />`,
  plus activities `.DownloadVoiceData` (`INSTALL_TTS_DATA`), `.CheckVoiceData` (`CHECK_TTS_DATA`),
  `.GetSampleText` (`GET_SAMPLE_TEXT`) and `.TtsSettingsActivity` (`CONFIGURE_ENGINE`);
  declared `versionCode="23"`, `versionName="1.53.0"`, application class
  `com.reecedunn.espeak.EspeakApp`, Wear standalone meta-data `com.google.android.wearable.standalone="true"`:
  [espeak-ng android/AndroidManifest.xml](https://cdn.jsdelivr.net/gh/espeak-ng/espeak-ng@master/android/AndroidManifest.xml),
  engine service file path [android/src/com/reecedunn/espeak/TtsService.java](https://github.com/espeak-ng/espeak-ng/blob/7dcfa23f/android/src/com/reecedunn/espeak/TtsService.java)
  (file content not retrieved). Installed `applicationId` / Play listing: **not published** in the file
  retrieved; `play.google.com` unreachable → **not determinable from public sources**.
  An engine can also be told about an install when data packs change: the framework broadcasts
  `ACTION_TTS_DATA_INSTALLED` and AOSP Settings re-runs `ACTION_CHECK_TTS_DATA` on receipt:
  [TtsEngineSettingsFragment.java, android-4.4_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-4.4_r1/src/com/android/settings/tts/TtsEngineSettingsFragment.java)
- Structural conclusion supported by the framework source: *any* app implementing the
  `android.intent.action.TTS_SERVICE` service (plus the `tts-engine` meta-data if it wants a settings
  screen) is a first-class TTS engine, and a client app can additionally name a specific engine in
  the constructor `TextToSpeech(Context, OnInitListener, String engine)`:
  [TtsEngines.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TtsEngines.java)
  (constructor used by Qt's own backend, see B3).

### A3b. Vendor engine survey — what could and could not be verified

Every claim below is limited to what was retrieved. "Not determinable" here always means: the
vendor's own page could not be retrieved in this environment (see section 0).

**Google Speech Services / "Speech Services by Google" (`com.google.android.tts`)**
- No Google-hosted page could be retrieved that documents `com.google.android.tts` as an Android TTS
  engine: `developer.android.google.cn` returned HTTP 200 but only site chrome/body-truncated for
  [TextToSpeech](https://developer.android.google.cn/reference/android/speech/tts/TextToSpeech),
  [TextToSpeech.Engine](https://developer.android.google.cn/reference/android/speech/tts/TextToSpeech.Engine),
  [TextToSpeechService](https://developer.android.google.cn/reference/android/speech/tts/TextToSpeechService) and
  [an accessibility guide page](https://developer.android.google.cn/guide/topics/ui/accessibility/apps);
  `support.google.com`, `play.google.com`, `source.android.com`, `android.googlesource.com` are
  network-blocked; `android-developers.googleblog.com` and `developers.googleblog.com` fail to fetch.
  → the claim that `com.google.android.tts` is the Google engine is **not determinable from public
  sources** retrievable here.
- Google blog URLs on Google's TTS engine exist in the search index but could not be fetched, so
  nothing from their content is reported:
  [Wear OS TTS engine announcement](https://android-developers.googleblog.com/2024/03/introducing-new-text-to-speech-engine-wear-os.html),
  [64-bit TTS upgrades post](https://android-developers.googleblog.com/2022/09/listen-to-our-major-text-to-speech-upgrades-for-64-bit-devices.html)
- What *is* verifiable for any Google engine: it must be a service registered for
  `android.intent.action.TTS_SERVICE` with meta-data `android.speech.tts`, and must implement
  `INSTALL_TTS_DATA` / `CHECK_TTS_DATA` / `GET_SAMPLE_TEXT` for the platform flows:
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)

**Samsung Text-to-speech Engine (`com.samsung.SMT` family)**
- `developer.samsung.com` could not be reached (`fetch failed`) — [developer.samsung.com](https://developer.samsung.com/);
  the consumer support page [Text-to-speech on your Samsung smartwatch](https://www.samsung.com/ca/support/mobile-devices/samsung-smartwatch-text-to-speech/)
  returned HTTP 200 but only site navigation, so no engine/package statement is verifiable from it.
- Samsung Knox admin docs are reachable ([Knox Platform for Enterprise](https://docs.samsungknox.com/admin/knox-platform-for-enterprise/)),
  but the per-device preinstalled-package tables surfaced by search are PDFs rejected by the fetch
  tool as `application/pdf` — [G891A_O.pdf](https://docs.samsungknox.com/admin/knox-platform-for-enterprise/assets/G891A_O.pdf).
- A Galaxy Store page for a `com.samsung.SMT.*` package appears in the search index
  ([galaxystore.samsung.com/detail/com.samsung.SMT.lang_zh_cn_f00](https://galaxystore.samsung.com/detail/com.samsung.SMT.lang_zh_cn_f00))
  but fetching it redirects cross-origin to `apps.galaxyappstore.com`, which the fetch tool refused —
  package name **not verified**.
- [opensource.samsung.com](https://opensource.samsung.com/) returned HTTP 403 (Cloudflare challenge).
- → No Samsung-authored statement was retrieved that a `com.samsung.SMT` engine registers for
  `android.intent.action.TTS_SERVICE`: **not determinable from public sources**.

**iFlytek / 讯飞**
- iFlytek's own Android TTS docs document a **proprietary SDK**, not the standard engine contract:
  init via `SpeechUtility.createUtility(context, SpeechConstant.APPID + "=…")`, `msc.jar`/`libmsc.so`,
  permissions `INTERNET`/`RECORD_AUDIO`, parameters `ENGINE_TYPE=TYPE_CLOUD`, `voice_name`, `speed`,
  `volume`, `pitch`, `sample_rate`:
  [xfyun.cn 在线语音合成 Android SDK](https://www.xfyun.cn/doc/tts/online_tts/Android-SDK.html)
- MSC API reference documents `com.iflytek.cloud.SpeechSynthesizer` with
  `createSynthesizer(Context, InitListener)`, `startSpeaking(String, SynthesizerListener)`,
  `synthesizeToUri(...)`, `setParameter(...)`:
  [xfyun.cn MSC Android 语音合成](https://www.xfyun.cn/doc/mscapi/Android/androidsynthesizer.html).
  Neither page mentions `android.intent.action.TTS_SERVICE`, meta-data `android.speech.tts`,
  `CHECK_TTS_DATA` or `INSTALL_TTS_DATA`.
- Newer AIkit offline TTS is likewise an AAR-based proprietary SDK (`AIKit.aar`,
  `AiHelper.getInst().initEntry(...)`, `AiRequest`/`AiListener`, license activation) with no mention
  of the standard engine contract:
  [xfyun.cn AIkit 离线语音合成（轻量级）Android SDK](https://www.xfyun.cn/doc/tts/AIkit_offline_tts/Android-SDK%28Lightweight%29.html)
- Whether any iFlytek *app* (e.g. `com.iflytek.speechcloud`) registers as a system TTS engine is not
  documented on the retrieved pages — **not determinable from public sources**
  ([xfyun.cn](https://www.xfyun.cn/) itself is reachable).

**Baidu / 百度语音**
- Official TTS Android SDK doc pages exist and return HTTP 200 —
  [ai.baidu.com/ai-doc/SPEECH/cltwwjwqm](https://ai.baidu.com/ai-doc/SPEECH/cltwwjwqm) and
  [cloud.baidu.com/doc/SPEECH/s/cltwwjwqm](https://cloud.baidu.com/doc/SPEECH/s/cltwwjwqm) — but both
  are client-side rendered and only site chrome was retrieved, so no content could be quoted.
- Whether Baidu exposes the standard `android.speech.tts` engine API, and the package names
  `com.baidu.tts` / `com.baidu.duersdk`, could not be verified from any retrieved Baidu primary
  source — **not determinable from public sources** ([ai.baidu.com](https://ai.baidu.com/) is reachable).

**Other engines**
- Beyond eSpeak NG (quoted in A3) and the framework abstract base class `TextToSpeechService`
  ([TextToSpeechService.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeechService.java)),
  no other engine with a retrievable primary-source registration for
  `android.intent.action.TTS_SERVICE` was identified. Google Play listings (the usual venue for such
  evidence) are unreachable — **not determinable from public sources**.

### A4. How does a Qt C++ app call Android TextToSpeech?

- **Qt does ship a wrapper** — the QtTextToSpeech module, whose Android engine calls
  `android.speech.tts.TextToSpeech` (see Topic B). That is the supported path.
- **Qt's own Android backend requires a Java class compiled into the app's jar** — this is the
  decisive fact for a "no Java/Gradle" project:
  - The C++ backend declares the Java type it talks to:
    `Q_DECLARE_JNI_CLASS(QtTextToSpeech, "org/qtproject/qt/android/speech/QtTextToSpeech")`:
    [qtexttospeech_android.h](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.h)
  - That Java class is built by Gradle into a jar named `Qt6AndroidTextToSpeech`
    (`qt_internal_add_jar(Qt${QtSpeech_VERSION_MAJOR}AndroidTextToSpeech SOURCES … OUTPUT_DIR
    "${QT_BUILD_DIR}/jar")`, then `install_jar(… COMPONENT Devel)`):
    [jar/CMakeLists.txt](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/CMakeLists.txt)
  - Gradle config for that jar: `plugins { alias(libs.plugins.android.library) }`,
    `namespace = 'org.qtproject.qt.android.speech'`, `compileSdk` default 36, `minSdk` default 28:
    [jar/build.gradle](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/build.gradle)
  - The C++ plugin depends on that jar: `add_dependencies(QTextToSpeechEngineAndroidPlugin Qt${QtSpeech_VERSION_MAJOR}AndroidTextToSpeech)`:
    [src/CMakeLists.txt](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/CMakeLists.txt)
  - The Java side is a plain `class QtTextToSpeech` in package `org.qtproject.qt.android.speech` that
    wraps `android.speech.tts.TextToSpeech`, implements `OnInitListener` and
    `UtteranceProgressListener`, and calls back into C++ through `native void notify*(…)`:
    [QtTextToSpeech.java](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech/QtTextToSpeech.java)
- **Can `QJniObject::callMethod` on `"android/speech/tts/TextToSpeech"` work from C++ with no Java
  source at all?** Partially documented, and the documented limits matter here:
  - Calling framework classes directly **is** documented, with framework examples using fully
    qualified class names and no app Java code — e.g. `QJniObject myJavaString("java/lang/String");`,
    `QJniObject::callStaticObjectMethod("java/lang/Thread", "currentThread", "()Ljava/lang/Thread;")`,
    `QJniObject::callStaticMethod<jint>("java/lang/Math", "max", "(II)I", a, b)`:
    [QJniObject class reference (Qt 6.11)](https://doc.qt.io/qt-6/qjniobject.html)
  - `QJniObject` is documented as Android-only: "This API has been designed and tested for use with
    Android. It has not been tested for other platforms":
    [QJniObject class reference](https://doc.qt.io/qt-6/qjniobject.html)
  - Qt's own Android TTS backend addresses a **framework** class this way, entirely in C++, with no
    app Java code: `Q_DECLARE_JNI_CLASS(Locale, "java/util/Locale")` and
    `Q_DECLARE_JNI_NATIVE_METHOD(...)` are declared in the C++ source:
    [qtexttospeech_android.cpp](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.cpp).
    So the "declare a JNI class in C++" mechanism is demonstrated by Qt itself for platform types —
    including the `android/provider/Settings$Secure` example in the QJniObject docs
    (`Q_DECLARE_JNI_CLASS(SettingsSecure, "android/provider/Settings$Secure")`):
    [QJniObject class reference](https://doc.qt.io/qt-6/qjniobject.html)
  - Java-to-C++ callbacks are documented as requiring registration of native methods
    (`QJniEnvironment::registerNativeMethods()`), which in turn requires a Java declaration
    (`private static native void …`):
    [QJniObject class reference](https://doc.qt.io/qt-6/qjniobject.html)
  - `android.speech.tts.TextToSpeech` cannot be used from pure C++ method calls without implementing
    a Java interface: its constructor is
    `TextToSpeech(Context context, TextToSpeech.OnInitListener listener[, String engine])` and the
    engine-to-client callbacks arrive through `UtteranceProgressListener`, an abstract Java class:
    [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java),
    [UtteranceProgressListener.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/UtteranceProgressListener.java)
  - Qt's documented answer for exactly this case is generated Java glue via a Gradle/Kotlin project:
    "For an application that invokes methods but does not need to implement any callbacks, a single
    code-generation project is enough. For an application that uses both method invocations and
    interface callbacks, two generation projects are needed: one for the method proxies and one for
    the callback hierarchy." Qt Jenny "translates Java APIs to Qt C++ wrappers", and its installation
    page says "You will need to configure a Gradle project that has a dependency to
    `qtjenny-annotation` and `qtjenny-compiler`":
    [Qt Jenny: Java Code Generator](https://doc.qt.io/qt-6/qtjenny.html),
    [Qt Jenny installation](https://doc.qt.io/qt-6/qtjenny-installation.html)
  - No Qt example, forum thread or bug report was found in the sources reachable here that documents
    driving `android.speech.tts.TextToSpeech` from `QJniObject` **without** any Java class. Whether
    such a no-Java approach is workable in practice (e.g. by hand-rolling a `java.lang.reflect.Proxy`
    or by polling state instead of using listeners) is **not determinable from public sources read
    here**, and is **not documented** by Qt.
- Qt for Android builds already run Gradle: the Qt 6.11 supported configuration lists "Gradle 9.3.1
  and AGP 9.0.0", JDK 21, Clang 17.0.2 / NDK r27c:
  [Qt for Android](https://doc.qt.io/qt-6/android.html),
  [Supported Platforms](https://doc.qt.io/qt-6/supported-platforms.html)

### A5. Sample rate, PCM access, latency, and exact API names / levels

- **`TextToSpeech.getSampleRate()` does not exist.** The complete list of public methods in
  `android.speech.tts.TextToSpeech` (AOSP master) contains no `getSampleRate`; the public surface is
  `addSpeech` (×5), `addEarcon` (×4), `speak` (×2), `playEarcon` (×2), `playSilentUtterance`,
  `playSilence`, `getFeatures(Locale)`, `isSpeaking`, `stop`, `setSpeechRate`, `setPitch`,
  `setAudioAttributes`, `getCurrentEngine`, `getDefaultLanguage`, `setLanguage`, `getLanguage`,
  `setVoice`, `getVoice`, `getDefaultVoice`, `isLanguageAvailable`, `synthesizeToFile` (×3),
  `setOnUtteranceCompletedListener`, `setOnUtteranceProgressListener`, `setEngineByPackageName`,
  `getDefaultEngine`, `areDefaultsEnforced`, plus `getEngines()`:
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java).
  (Note: this environment extracted the file from the `aosp-mirror/platform_frameworks_base@master`
  tree; that tree's `TextToSpeech.java` is the file searched.)
- **The sample rate is chosen by the engine**, and the API that sets it is on the engine side:
  `SynthesisCallback.start(int sampleRateInHz, @SupportedAudioFormat int audioFormat, @IntRange(from=1,to=2) int channelCount)`
  — javadoc: "The service should call this when it starts to synthesize audio for this request. …
  @param sampleRateInHz Sample rate in HZ of the generated audio. @param audioFormat … Must be one of
  `AudioFormat#ENCODING_PCM_8BIT` or `AudioFormat#ENCODING_PCM_16BIT`. Can also be
  `AudioFormat#ENCODING_PCM_FLOAT` when targetting Android N and above. @param channelCount The
  number of channels. Must be `1` or `2`.":
  [SynthesisCallback.java, AOSP master](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/SynthesisCallback.java)
- **The client learns the format from a callback**: 
  `UtteranceProgressListener.onBeginSynthesis(String utteranceId, int sampleRateInHz, int audioFormat, int channelCount)`
  — "Called when the TTS engine begins to synthesize the audio for a request. It provides information
  about the format of the byte array for subsequent `onAudioAvailable` calls. … @param sampleRateInHz
  Sample rate in hertz of the generated audio. @param audioFormat Audio format of the generated audio.
  Should be one of `AudioFormat#ENCODING_PCM_8BIT`, `AudioFormat#ENCODING_PCM_16BIT` or
  `AudioFormat#ENCODING_PCM_FLOAT`. @param channelCount The number of channels.":
  [UtteranceProgressListener.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/UtteranceProgressListener.java)
- **Streaming raw PCM to the client**:
  `UtteranceProgressListener.onAudioAvailable(String utteranceId, byte[] audio)` — "This is called
  when a chunk of audio is ready for consumption. The audio parameter is a copy of what will be
  synthesized to the speakers (when synthesis was initiated with a `speak` call) or written to the
  file system (for `synthesizeToFile`). The audio bytes are delivered in one or more chunks; if
  `onDone` or `onError` is called all chunks have been received.":
  [UtteranceProgressListener.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/UtteranceProgressListener.java)
- **API levels — established by AOSP tag diff, because the doc annotations were unreachable:**
  - `onBeginSynthesis` and `onAudioAvailable` are **absent** in
    `platform_frameworks_base` tag `android-6.0.1_r1` (Android 6.0, API 23):
    [UtteranceProgressListener.java @ android-6.0.1_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@android-6.0.1_r1/core/java/android/speech/tts/UtteranceProgressListener.java)
    and **present** in tag `android-7.0.0_r1` (Android 7.0, API 24):
    [UtteranceProgressListener.java @ android-7.0.0_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@android-7.0.0_r1/core/java/android/speech/tts/UtteranceProgressListener.java)
    → these callbacks enter the platform at **API level 24**, **not** API 26. (Corroborated by Qt's
    backend, which gates capabilities on the API-24 "Nougat" threshold — see B3.)
  - `onRangeStart` is **absent** in `android-7.0.0_r1` (API 24) and **present** in
    `android-7.1.1_r1` (API 25):
    [UtteranceProgressListener.java @ android-7.1.1_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@android-7.1.1_r1/core/java/android/speech/tts/UtteranceProgressListener.java)
    → enters the platform at **API level 25**.
  - The official "Added in API level N" annotations themselves could **not** be verified
    (developer.android.com unreachable) — **not determinable from public sources reachable here**;
    the tag-diff evidence above is source-level evidence that those methods are present in the
    API 24 and API 25 platform source trees respectively.
- **File synthesis** (the documented way to obtain a whole utterance rather than chunks):
  `synthesizeToFile(CharSequence text, Bundle params, File file, String utteranceId)` and
  `synthesizeToFile(CharSequence text, Bundle params, ParcelFileDescriptor fileDescriptor, String utteranceId)`
  — "Synthesizes the given text to a file using the specified parameters. This method is
  asynchronous, i.e. the method just adds the request to the queue of TTS requests and then returns."
  The `HashMap<String,String>`/`String filename` overload is `@Deprecated` "As of API level 21,
  replaced by `synthesizeToFile(CharSequence, Bundle, File, String)`" and its javadoc example names a
  `.wav` path ("something like `/sdcard/myappsounds/mysound.wav`"), but the container format itself is
  produced by the engine, not specified by the framework:
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java)
- `SynthesisRequest` (engine-side request object) exposes `getText`, `getCharSequenceText`,
  `getVoiceName`, `getLanguage`, `getCountry`, `getVariant`, `getSpeechRate`, `getPitch`, `getParams`,
  `getCallerUid` — and **no** sample-rate getter:
  [SynthesisRequest.java, AOSP master](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/SynthesisRequest.java)
- Concrete numeric sample rate used by any real engine (e.g. Google Speech Services with Mandarin
  voices): **not published** in AOSP; it is only discoverable at runtime via `onBeginSynthesis`.
  No documented fixed value exists — **not published**.
- Measured latency of any real engine: **not published**. The only latency figures in the framework
  are engine-declared labels (`Voice.getLatency()` / `LATENCY_*`, quoted in A1), which are
  declarations, not measurements:
  [Voice.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/Voice.java)
- Speech-rate API for rate scaling: `TextToSpeech.setSpeechRate(float)` with "1.0 is the normal
  speech rate" (per Qt's own rate mapping, see B3) and engine default
  `Engine.DEFAULT_RATE = 100` (per-mille-ish scale) — both in
  [TextToSpeech.java](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java);
  the AOSP Settings UI bounds rate/pitch sliders at 10–600 and 25–400 respectively:
  [TextToSpeechSettings.java, android-9.0.0_r1](https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-9.0.0_r1/src/com/android/settings/tts/TextToSpeechSettings.java)

---

## Topic B — Qt TextToSpeech module (Qt 6)

### B1. Does the module exist in Qt 6.11.1? Module name, CMake target, introduction version

- The module exists and is documented for the Qt 6.11 series; the live manual page is "Qt TextToSpeech | **Qt 6.11.2**":
  [Qt TextToSpeech](https://doc.qt.io/qt-6/qttexttospeech-index.html)
- Module name: **QtTextToSpeech** (QML import `QtTextToSpeech`); C++ classes `QTextToSpeech`, `QVoice`:
  [Qt TextToSpeech](https://doc.qt.io/qt-6/qttexttospeech-index.html)
- CMake target: `find_package(Qt6 REQUIRED COMPONENTS TextToSpeech)` +
  `target_link_libraries(mytarget PRIVATE Qt6::TextToSpeech)`; qmake: `QT += texttospeech`:
  [Qt TextToSpeech](https://doc.qt.io/qt-6/qttexttospeech-index.html)
- Version evidence for 6.11.1 specifically: the Qt repository browser lists branches `6.11`, `6.11.0`,
  **`6.11.1`**, `6.11.2`, `6.11.3`, `6.12` for `qt/qtspeech.git`:
  [qt/qtspeech.git ref list (top of page)](https://code.qt.io/cgit/qt/qtspeech.git/tree/src/plugins/tts/android)
  (the header of any tree page shows the branch list; e.g.
  [src/plugins/tts/android tree @ dev](https://code.qt.io/cgit/qt/qtspeech.git/tree/src/plugins/tts/android)).
- Introduction version: the module entered Qt as **Qt 5.8, as a Technology Preview**, named "Qt
  Speech": "Technology Preview Modules — Qt Speech - A module to make text to speech and speech
  recognition easy. For Qt 5.8 … only the text to speech part is released. **It has backends for
  several speech synthesizers on macOS, Android, Windows and Linux** currently.":
  [New Features in Qt 5.8 (Qt Wiki)](https://wiki.qt.io/New_Features_in_Qt_5.8)
  (Qt 5.8 final release date in the release plan: 23 January 2017 —
  [Qt 5.8 Release (Qt Wiki)](https://wiki.qt.io/Qt_5.8_Release))
- Confirmed still named "Qt Speech" in the Qt 5.9 archives, already with `#include <QTextToSpeech>`
  and `QT += texttospeech`:
  [Qt Speech 5.9 (archives)](https://doc.qt.io/archives/qt-5.9/qtspeech-index.html)
- The Qt 6.11 documentation page does **not** state the version in which the module was introduced;
  the exact wording "introduced in Qt X" for the Qt 6 series is **not published** on the pages read
  here (the only explicit "introduced in" statements on those pages concern individual members, e.g.
  `Capability` "introduced in Qt 6.6", `engine`/`setEngine` "introduced in Qt 6.4"):
  [QTextToSpeech class reference](https://doc.qt.io/qt-6/qtexttospeech.html)

### B2. Is Android a supported platform? Which backends exist?

- Android is supported: "The 'android' engine is **the only engine available on the Android
  platform**. It uses the `TextToSpeech` package, which in turn supports multiple engine backends."
  Caveat quoted: "**Note:** The 'android' engine does not have the `PauseResume` capability." Engine
  parameter: `androidEngine` (QString) — "There is no API in Qt to get the list of installed engines.":
  [Qt TextToSpeech Engines](https://doc.qt.io/qt-6/qttexttospeech-engines.html)
- Complete backend list documented on that page: **WinRT** (`"winrt"`, uses
  `Windows.Media.SpeechSynthesis`, plays PCM via `QAudioSink` from Qt Multimedia; parameter
  `audioDevice`), **SAPI** (`"sapi"`, SAPI 5.3; no engine-specific parameters), **Darwin**
  (`"darwin"`, AVFoundation; iOS and macOS), **Android** (`"android"`), **Flite** (`"flite"`,
  requires Flite ≥ 2.2, renders PCM via `QAudioSink`; parameter `audioDevice`), and
  **speech-dispatcher** (`"speechd"`, requires libspeechd ≥ 0.9; lacks `WordByWordProgress` and
  `Synthesize`):
  [Qt TextToSpeech Engines](https://doc.qt.io/qt-6/qttexttospeech-engines.html)
- A "mock" engine is also returned by `availableEngines()` and "should not be deployed to target
  systems":
  [Qt TextToSpeech Engines](https://doc.qt.io/qt-6/qttexttospeech-engines.html)
- Qt's global platform-support page lists the supported Android configuration (Android 9 / API 28 to
  Android 16 / API 36; `arm64-v8a`, `x86_64`, `x86`, `armeabi-v7a`; Clang 17.0.2 / NDK r27c;
  JDK 21; Gradle 9.3.1, AGP 9.0.0) but does **not** carry a per-module supported-platform table:
  [Supported Platforms](https://doc.qt.io/qt-6/supported-platforms.html),
  [Qt for Android](https://doc.qt.io/qt-6/android.html)
- QtTextToSpeech is classified as a **Qt Add-On** module ("Provides support for synthesizing speech
  from text and playing it as audio output"), not a Qt Essential:
  [All Modules](https://doc.qt.io/qt-6/qtmodules.html)

### B3. What does the Android backend wrap? Which JNI/Java files does it need?

- It wraps `android.speech.tts.TextToSpeech` through JNI, via a **Java helper class**:
  - C++ side declares `Q_DECLARE_JNI_CLASS(QtTextToSpeech, "org/qtproject/qt/android/speech/QtTextToSpeech")`
    and holds a `QJniObject m_speech`:
    [qtexttospeech_android.h](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.h)
  - `JNI_OnLoad` looks the class up and registers seven C++ native methods — `notifyError`,
    `notifyReady`, `notifySpeaking`, `notifyRangeStart`, `notifyBeginSynthesis`,
    `notifyAudioAvailable`, `notifyEndSynthesis` — via `jniEnv.registerNativeMethods(clazz, {...})`;
    the engine object is created with
    `QJniObject::construct<QtJniTypes::QtTextToSpeech>(QNativeInterface::QAndroidApplication::context(), id, …)`:
    [qtexttospeech_android.cpp](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.cpp)
  - Java file needed: `src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech/QtTextToSpeech.java`:
    [directory listing](https://code.qt.io/cgit/qt/qtspeech.git/tree/src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech),
    [QtTextToSpeech.java](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech/QtTextToSpeech.java)
  - Build files needed: [jar/CMakeLists.txt](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/CMakeLists.txt),
    [jar/build.gradle](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/build.gradle),
    [jar/settings.gradle](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/settings.gradle),
    [jar/gradle.properties](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/gradle.properties)
- What the Java class does with the framework API (all quoted from
  [QtTextToSpeech.java](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech/QtTextToSpeech.java)):
  - constructor: `mTts = new TextToSpeech(context, mTtsChangeListener);` or, when an engine name was
    passed from Qt, `new TextToSpeech(context, mTtsChangeListener, engine)`, then
    `mTts.setOnUtteranceProgressListener(mTtsUtteranceProgressListener)`;
  - speech: `mTts.speak(text, TextToSpeech.QUEUE_FLUSH, params, UTTERANCE_ID)` with
    `params.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, mVolume)`;
  - PCM: `result = mTts.synthesizeToFile(text, params, file, SYNTHESIZE_ID);` **where `file = new File("/dev/null")`**,
    and the PCM is picked up in `onAudioAvailable(...)` → `notifyAudioAvailable(mId, bytes)`;
    the audio format is converted in `onBeginSynthesis(...)` from `AudioFormat.ENCODING_PCM_*` to
    `QAudioFormat` enums (8BIT→1/`UInt8`, 16BIT→2/`Int16`, FLOAT→4/`Float`, else 0/`Unknown`);
  - rate/pitch: `mTts.setSpeechRate(rate)` / `mTts.setPitch(pitch)`; string rate also read from
    `Settings.Secure.TTS_DEFAULT_RATE` (÷100) and pitch from `Settings.Secure.TTS_DEFAULT_PITCH` (÷100);
  - locale/voice: `mTts.setLanguage(locale)`, `mTts.getVoices()`, `mTts.getVoice()`, `mTts.setVoice(voice)`,
    `mTts.getAvailableLanguages()`, all gated on `Build.VERSION.SDK_INT >= LOLLIPOP`.
- Qt-side capability declaration for the Android plugin (file contents verbatim):
  `{"Keys": ["android"], "Provider": "android", "Version": 100, "Priority": 100, "Capabilities": ["Speak", "WordByWordProgress", "Synthesize"]}` —
  i.e. **no `PauseResume`**:
  [android_plugin.json](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/android_plugin.json)
- Version gating in the C++ backend: `if (QOperatingSystemVersion::current() < QOperatingSystemVersion::AndroidNougat) return QTextToSpeech::Capability::Speak;` —
  i.e. below Android 7.0 (API 24) only `Speak` is claimed (consistent with the API-24 finding in A5):
  [qtexttospeech_android.cpp](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.cpp)
- Known behavioural limitation documented in the class reference: "**Note:** On Android, resuming
  paused speech will restart from the beginning. This is a limitation of the underlying
  text-to-speech engine.":
  [QTextToSpeech class reference](https://doc.qt.io/qt-6/qtexttospeech.html)
- The Kotlin/Gradle code-generation tool Qt ships for Java interop (relevant to A4) is documented
  separately: [Qt Jenny: Java Code Generator](https://doc.qt.io/qt-6/qtjenny.html),
  [Qt Jenny installation](https://doc.qt.io/qt-6/qtjenny-installation.html)

### B4. Licence of QtTextToSpeech

- The module's own documentation page states: "Qt TextToSpeech is available under commercial licenses
  from The Qt Company. In addition, it is available under free software licenses: The GNU Lesser
  General Public License, version 3, **or the GNU General Public License, version 2**":
  [Qt TextToSpeech](https://doc.qt.io/qt-6/qttexttospeech-index.html)
- The module's source headers state a different (broader) triple:
  `SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only`:
  [qtexttospeech_android.h](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.h),
  [qtexttospeech_android.cpp](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.cpp),
  [QtTextToSpeech.java](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech/QtTextToSpeech.java)
  (the Java file's header is `LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only`;
  the build files are `BSD-3-Clause`: [jar/CMakeLists.txt](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/CMakeLists.txt),
  [src/CMakeLists.txt](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/CMakeLists.txt))
- The Qt 6.11 licensing page describes the general scheme — commercial, or LGPLv3 "provided you can
  comply with the terms and conditions of the GNU LGPL version 3 (or GNU GPL version 3)" — and lists
  the modules that are GPLv3-only instead of LGPLv3. **Qt TextToSpeech is not in that GPLv3-only
  list**:
  [Qt Licensing (Qt 6.11)](https://doc.qt.io/qt-6/licensing.html)
- Net: three primary sources give three slightly different statements (index page: LGPLv3 or **GPLv2**;
  source headers: LGPL-3.0-only or GPL-2.0-only or GPL-3.0-only; licensing page: LGPLv3, with a
  GPLv3-only list that excludes TextToSpeech). Which exact GPL version applies to the module
  documentation prose is **not determinable** from these pages alone.

### B5. Standard Qt for Android binary install, or a separate add-on?

- QtTextToSpeech is listed under "**Qt Add-Ons**", not under "Qt Essentials". The page states:
  "Qt Add-On modules bring additional value for specific purposes. These modules may only be
  available on some development platform. … **The Qt installers include the option of downloading
  the add-ons.**"
  [All Modules](https://doc.qt.io/qt-6/qtmodules.html)
- Consequences stated in the docs: Essentials "are available on all supported development platforms
  and on the tested target platforms"; Add-Ons "may only be available on some development platform"
  and "Each add-on module specifies its compatibility promise separately":
  [All Modules](https://doc.qt.io/qt-6/qtmodules.html)
- The Qt 6 module index for Qt 6.11 places TextToSpeech in the C++ classes list as
  `QTextToSpeech`/`QVoice`, i.e. it is a first-class documented module of the 6.11 release (not an
  extension module):
  [Qt TextToSpeech](https://doc.qt.io/qt-6/qttexttospeech-index.html),
  [All Modules](https://doc.qt.io/qt-6/qtmodules.html)
- Whether the module + its `Qt6AndroidTextToSpeech` jar are present **by default** in the Qt Online
  Installer's Android binary package, or must be explicitly ticked, is **not determinable from
  public sources read here**: the Qt for Android page documents only platform configurations
  ("Supported Configurations" table), and contains no per-module list:
  [Qt for Android](https://doc.qt.io/qt-6/android.html),
  [Supported Platforms](https://doc.qt.io/qt-6/supported-platforms.html).
  The only positive statement found is the generic one that add-ons are a download option in the
  installer: [All Modules](https://doc.qt.io/qt-6/qtmodules.html)

---

## Topic C — Does `QTextToSpeech` expose raw PCM, or only play via the platform API?

- `QTextToSpeech` **does** expose raw PCM, since Qt 6.6, through `synthesize()`:
  - `[since 6.6] template <typename Functor> void QTextToSpeech::synthesize(const QString &text, Functor &&functor)`
    and `synthesize(const QString &text, const QObject *context, Functor &&functor)`:
    "**Synthesizes the text into raw audio data.** … When data is available, the functor will be
    called as `functor(QAudioFormat format, QByteArray bytes)`, with `format` describing the format
    of the data in `bytes`; or as `functor(QAudioBuffer &buffer)`. … While synthesizing, the functor
    might be called multiple times, possibly with changing values for `format`." Capability gate:
    "**Note:** This API requires that the engine has the `Synthesize` capability."
    [QTextToSpeech class reference](https://doc.qt.io/qt-6/qtexttospeech.html)
  - Class overview: "To synthesize text into PCM data for further processing, use `synthesize()`.":
    [QTextToSpeech class reference](https://doc.qt.io/qt-6/qtexttospeech.html)
  - `Capability::Synthesize` = `1 << 3` — "The engine can synthesize PCM audio data from text."
    (enum introduced in Qt 6.6); `State::Synthesizing` = `4` — "Text is being synthesized into PCM
    data. The synthesized() signal will be emitted with chunks of data." (Note: the class's public
    **signals** list contains no `synthesized()` signal — the documented signals are
    `aboutToSynthesize`, `engineChanged`, `errorOccurred`, `localeChanged`, `pitchChanged`,
    `rateChanged`, `sayingWord`, `stateChanged`, `voiceChanged`, `volumeChanged`; the `synthesized()`
    signal named in the `State` enum description belongs to the engine-side interface that is not
    public API since Qt 6):
    [QTextToSpeech class reference](https://doc.qt.io/qt-6/qtexttospeech.html)
- The other main entry point only plays audio through the platform API:
  `void QTextToSpeech::say(const QString &text)` — "Starts speaking the text. This function starts
  sythesizing the speech asynchronously, and reads the text to the default audio output device.":
  [QTextToSpeech class reference](https://doc.qt.io/qt-6/qtexttospeech.html)
- **`setAudioOutput()` does not exist** in `QTextToSpeech` as documented for Qt 6.11.2: it appears
  neither in the Public Functions list (which contains the constructors/destructor,
  `availableLocales`, `availableVoices`, `engine`, `engineCapabilities`, `errorReason`, `errorString`,
  `findVoices`, `locale`, `pitch`, `rate`, `setEngine`, `state`, `synthesize` ×2, `voice`, `volume`)
  nor in the Public Slots list (`enqueue`, `pause`, `resume`, `say`, `setLocale`, `setPitch`,
  `setRate`, `setVoice`, `setVolume`, `stop`):
  [QTextToSpeech class reference](https://doc.qt.io/qt-6/qtexttospeech.html).
  The only audio-device parameter documented anywhere in the module is the engine-level
  `audioDevice` parameter for the **winrt** and **flite** engines — **not** for the Android engine:
  [Qt TextToSpeech Engines](https://doc.qt.io/qt-6/qttexttospeech-engines.html)
- On Android specifically, the PCM path exists because the Qt Android plugin declares the
  `Synthesize` capability and the Java glue implements it through
  `mTts.synthesizeToFile(text, params, new File("/dev/null"), SYNTHESIZE_ID)` + `onAudioAvailable`:
  [android_plugin.json](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/android_plugin.json),
  [QtTextToSpeech.java](https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech/QtTextToSpeech.java)
- `QTextToSpeechEngine`/`QTextToSpeechPlugin` were removed from the public API in Qt 6 — "They still
  exist and are used by the available engine implementations, but they are not part of the documented
  and supported API":
  [Changes to Qt TextToSpeech](https://doc.qt.io/qt-6/qttexttospeech-changes-qt6.html)

---

## Flat list of all URLs used

Android / AOSP:

1. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeech.java
2. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/Voice.java
3. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TtsEngines.java
4. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/UtteranceProgressListener.java
5. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/SynthesisCallback.java
6. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/SynthesisRequest.java
7. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@master/core/java/android/speech/tts/TextToSpeechService.java
8. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@android-6.0.1_r1/core/java/android/speech/tts/UtteranceProgressListener.java
9. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@android-7.0.0_r1/core/java/android/speech/tts/UtteranceProgressListener.java
10. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_frameworks_base@android-7.1.1_r1/core/java/android/speech/tts/UtteranceProgressListener.java
11. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-9.0.0_r1/src/com/android/settings/tts/TextToSpeechSettings.java
12. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-9.0.0_r1/src/com/android/settings/tts/TtsEnginePreference.java
13. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-5.1.1_r1/src/com/android/settings/tts/TextToSpeechSettings.java
14. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-4.4_r1/src/com/android/settings/tts/TtsEngineSettingsFragment.java
15. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_settings@android-4.4_r1/src/com/android/settings/tts/TtsEnginePreference.java
16. https://developer.android.google.cn/reference/android/speech/tts/TextToSpeech (reachable but body not extractable)
17. https://developer.android.google.cn/reference/android/speech/tts/Voice (reachable but body not extractable)
18. https://developer.android.google.cn/reference/android/speech/tts/UtteranceProgressListener (reachable but body not extractable)
19. https://developer.android.com/reference/android/speech/tts/TextToSpeech (unreachable from this environment)
20. https://android.googlesource.com/platform/frameworks/base/ (unreachable from this environment)

Android / AOSP (added by the vendor-engine survey, section A3b):

21. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_Settings@master/src/com/android/settings/tts/TextToSpeechSettings.java
22. https://cdn.jsdelivr.net/gh/aosp-mirror/platform_packages_apps_PicoTts@master/AndroidManifest.xml (404)
23. https://github.com/aosp-mirror/platform_packages_apps_PicoTts (404)
24. https://cdn.jsdelivr.net/gh/espeak-ng/espeak-ng@master/android/AndroidManifest.xml
25. https://github.com/espeak-ng/espeak-ng/blob/7dcfa23f/android/src/com/reecedunn/espeak/TtsService.java
26. https://developer.android.google.cn/reference/android/speech/tts/TextToSpeech.Engine
27. https://developer.android.google.cn/reference/android/speech/tts/TextToSpeechService
28. https://developer.android.google.cn/guide/topics/ui/accessibility/apps
29. https://android-developers.googleblog.com/2024/03/introducing-new-text-to-speech-engine-wear-os.html (unreachable)
30. https://android-developers.googleblog.com/2022/09/listen-to-our-major-text-to-speech-upgrades-for-64-bit-devices.html (unreachable)
31. https://www.xfyun.cn/doc/tts/online_tts/Android-SDK.html
32. https://www.xfyun.cn/doc/mscapi/Android/androidsynthesizer.html
33. https://www.xfyun.cn/doc/tts/AIkit_offline_tts/Android-SDK%28Lightweight%29.html
34. https://www.xfyun.cn/
35. https://ai.baidu.com/ai-doc/SPEECH/cltwwjwqm
36. https://cloud.baidu.com/doc/SPEECH/s/cltwwjwqm
37. https://ai.baidu.com/
38. https://developer.samsung.com/ (unreachable)
39. https://www.samsung.com/ca/support/mobile-devices/samsung-smartwatch-text-to-speech/
40. https://docs.samsungknox.com/admin/knox-platform-for-enterprise/
41. https://docs.samsungknox.com/admin/knox-platform-for-enterprise/assets/G891A_O.pdf (PDF rejected by fetch tool)
42. https://galaxystore.samsung.com/detail/com.samsung.SMT.lang_zh_cn_f00 (cross-origin redirect refused)
43. https://opensource.samsung.com/ (HTTP 403)
44. https://developers.google.cn/
45. https://developers.google.cn/android
46. https://support.google.cn/accessibility/android/answer/6006983
47. https://issuetracker.google.com/issues/439841787
48. https://www.android.com/accessibility/

Qt:

49. https://doc.qt.io/qt-6/qttexttospeech-index.html
50. https://doc.qt.io/qt-6/qttexttospeech-engines.html
51. https://doc.qt.io/qt-6/qtexttospeech.html
52. https://doc.qt.io/qt-6/qttexttospeech-changes-qt6.html
53. https://doc.qt.io/qt-6/qtmodules.html
54. https://doc.qt.io/qt-6/licensing.html
55. https://doc.qt.io/qt-6/qjniobject.html
56. https://doc.qt.io/qt-6/android.html
57. https://doc.qt.io/qt-6/supported-platforms.html
58. https://doc.qt.io/qt-6/qtjenny.html
59. https://doc.qt.io/qt-6/qtjenny-installation.html
60. https://doc.qt.io/archives/qt-5.9/qtspeech-index.html
61. https://wiki.qt.io/New_Features_in_Qt_5.8
62. https://wiki.qt.io/Qt_5.8_Release
63. https://code.qt.io/cgit/qt/qtspeech.git/tree/src/plugins/tts/android
64. https://code.qt.io/cgit/qt/qtspeech.git/tree/src/plugins/tts/android/src
65. https://code.qt.io/cgit/qt/qtspeech.git/tree/src/plugins/tts/android/jar
66. https://code.qt.io/cgit/qt/qtspeech.git/tree/src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech
67. https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/CMakeLists.txt
68. https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/CMakeLists.txt
69. https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/android_plugin.json
70. https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.h
71. https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/src/qtexttospeech_android.cpp
72. https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/CMakeLists.txt
73. https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/build.gradle
74. https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/settings.gradle
75. https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/gradle.properties
76. https://code.qt.io/cgit/qt/qtspeech.git/plain/src/plugins/tts/android/jar/src/org/qtproject/qt/android/speech/QtTextToSpeech.java
77. https://www.qt.io/blog/2017/01/20/qt-speech-text-speech (nav only; body not extractable)

Blocked / unusable from this environment: https://developer.android.com/*,
https://source.android.com/*, https://support.google.com/*, https://android.googlesource.com/*,
https://play.google.com/*, https://web.archive.org/*, https://r.jina.ai/*.
