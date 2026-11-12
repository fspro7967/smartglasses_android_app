# 08 — CMake 杂项:自动安装默认开、build-tools 版本写死、口令走命令行

Status: wontfix
Type: task

## 现状(根 `CMakeLists.txt`)

- `AUTO_INSTALL_APK` 默认 `ON`:`apk` 目标完成后自动 `adb install`,没插手机时构建以失败收场;
- `apksigner` 只在 `${ANDROID_SDK_ROOT}/build-tools/37.0.0` 里找,换个 build-tools 版本就
  `FATAL_ERROR`;
- 签名口令以 `--ks-pass pass:…` 形式出现在命令行(会进构建日志和 `CMakeCache.txt`);
- `QT_VERSION_MAJOR` 小于 6 的分支是死代码(前面已经 `find_package(Qt6 REQUIRED …)`)。

2026-11-11 通读代码时发现。

## 决定

**所有者 2026-11-11 决定不处理。** 要重新打开时把 Status 改回 `needs-triage`。

口令那一条与 `issues/01` 的结论相容:仓库里的密钥本来就被认定为公开的测试密钥
(`docs/adr/0001-committed-test-signing-key.md`)。换用正式密钥时这一条需要重新考虑。
