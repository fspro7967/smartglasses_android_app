# 08 — 提取清单只校验落盘副本自身:APK 换了模型也不会重新提取

Status: ready-for-agent
Type: bug

## 问题

`AssetExtractor::isExtracted()`(`src/assetextractor.cpp:51-100`)的校验链是:

1. 读 `.extract-manifest`,得到 `文件名 → 字节数`;
2. 清单里的文件集合必须与期望集合完全一致;
3. 逐个比对**落盘文件**的 `QFileInfo::size()` 与清单记录。

而清单里的字节数来自**上次复制的产物**(`extractFiles()` 里
`sizes.append(written)`,`src/assetextractor.cpp:180`),不是来自 APK 里的源资产。
于是清单是**自洽**的:它记录「我当时写出来的长度」,再拿落盘文件的长度去比,
只要没人动过那些文件就永远相等。

后果:装了一个带了新模型的新版本 APK 之后

- 文件集合没变(还是那 8 个名字)→ 第 2 步通过;
- 落盘的还是旧文件,长度与旧清单一致 → 第 3 步通过;
- `ensureExtracted()` 直接返回 true,**旧模型继续被使用**。

内容变了但长度恰好相同的模型更新同样识别不出来。对 TTS 来说,
表现是「换了模型/修了发音人,apk 装上去却没有任何变化」,而且没有任何日志提示。

## 影响范围

- 只有 TTS 模型走这条路(`MainWindow::prepareTtsModel()` →
  `AssetExtractor::ensureExtracted()`,`src/mainwindow.cpp:704-709`)。
  whisper 已经改成从内存加载,不再落盘,所以不受影响。
- 真机验证工单 `.scratch/offline-tts/issues/02` 测的是「首次出现 / 二次跳过 / 杀进程不留半截」,
  没有覆盖「换了源资产」这一条 —— 本工单补上。

## 建议做法(任选,推荐前两条一起)

1. **清单里记源资产长度**:`extractFiles()` 复制时顺便取
   `QFile(sourcePath).size()`(Android 的 `assets:/` 支持 `QFile::size()`),
   写进清单;`isExtracted()` 重新打开源资产取长度并比对。
   源长度不同即重新提取。
2. **清单里记一个源版本**:`QCoreApplication::applicationVersion()`
   或 APK 的 `versionCode`。清单版本号与当前不一致即重新提取。
   这条能覆盖「长度相同但内容不同」的情况(代价是每次升级都重提一次 31 MB,
   可以接受;也可只在构建时把模型 sha256 写进一个头文件)。
3. 若将来模型变大、想避免每次升级重提,可在后台线程算一次源资产 sha256 存进清单,
   校验时只比 sha256 —— 与 `.scratch/offline-tts/spec.md` 步骤 2 的
   「后台线程 + 分块」要求一致。

无论选哪个,`isExtracted()` 的语义要从「落盘文件没被动过」改成
**「落盘文件确实来自当前这份 APK」**。

## 完成判据

- [ ] 造一个模型文件长度不同的 APK,覆盖安装后应用重新提取(日志/时间戳可证)
- [ ] 源资产未变时仍不重复提取(保持 `issues/02` 的第 2 条判据)
- [ ] 清单格式变化有明确处理:旧格式清单应导致重提,而不是被当成损坏后无限重试

## 相关

- 实现:`src/assetextractor.cpp`、`includes/assetextractor.h`
- 调用方:`src/mainwindow.cpp:688-724`
- 既有真机判据:`.scratch/offline-tts/issues/02`
