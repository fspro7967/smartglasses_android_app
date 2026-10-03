# 02 — OpenSSL 预编译库没入库也没有拉取脚本,fresh clone 链接必失败

Status: ready-for-agent
Type: bug

## 事实

`CMakeLists.txt:109-111` 在 Android 构建里包含并调用:

```cmake
include(third_party/android_openssl/android_openssl.cmake)
add_android_openssl_libraries(smartglasses_android_app)
```

该 cmake 会为 Release 构建构造这些 IMPORTED 路径
(`third_party/android_openssl/android_openssl.cmake:4-22`):

```
third_party/android_openssl/ssl_3/arm64-v8a/libssl_3.so
third_party/android_openssl/ssl_3/arm64-v8a/libcrypto_3.so
```

但版本库里**只有头文件**:

```
$ git ls-files third_party/android_openssl/ssl_3/arm64-v8a/
third_party/android_openssl/ssl_3/arm64-v8a/include
$ git ls-files third_party/android_openssl | grep -cE '\.(a|so)$'
0
```

当前机器上这些 `.so` / `.a` 是存在的(未跟踪),所以本机能构建。
**换一台机器 clone 下来就没有**,而 `.gitignore` 的 `*.so` / `*.a` 决定了
它们永远不会被提交。项目里也**没有**任何拉取脚本 —— sherpa-onnx 有对标物
(`third_party/sherpa-onnx/download-libs.sh` + `sherpa-onnx.cmake` 里的
`FATAL_ERROR` 提示),OpenSSL 这边什么都没有。结果是 `cmake --build` 在链接期
报「找不到 libssl_3.so」,对新人来说这是个没头绪的错误。

`.scratch/offline-tts/handoff.md` 第 5.4 节把这条列为既有的 P1 问题。

## 建议做法(照抄 sherpa 的既有模式,不要新造)

1. 新增 `third_party/android_openssl/download-libs.sh`:
   - 从上游 KDAB/android_openssl 的 release/仓库拉取,或
   - 用 `build_ssl.sh`(已在目录里)在本机构建,并固定版本;
   - **校验 sha256** 并把期望值写进脚本,与 `sherpa-onnx/download-libs.sh` 一致。
2. 在 `android_openssl.cmake` 顶部加存在性检查:缺失时 `message(FATAL_ERROR ...)`
   并打印「先跑 download-libs.sh」,而不是让链接器报错。
3. 在 `third_party/android_openssl/README.md`(上游文档)之外,补一段本仓库的
   构建前置说明;或在根 `README.md` 的「How to build APK」里加一行
   —— 现在那里只说了 Qt 与 Android SDK。
4. 顺带核对 `.gitattributes`:脚本要保证 LF,否则 `bash script.sh` 在 Linux 上会因
   CRLF 报 `\r: command not found`(sherpa 那边已有先例)。

## 完成判据

- [ ] 在一个干净的容器/目录里 `git clone` → 跑前置脚本 → `cmake --build` 的
      Android Release 目标能链接通过
- [ ] 缺库时 configure/build 给出的是「请先跑 xxx.sh」,不是链接器错误
- [ ] 根 README 的构建步骤包含这一步

## 相关

- 对标实现:`third_party/sherpa-onnx/download-libs.sh`、`sherpa-onnx.cmake:24-32`
- 交接文档:`.scratch/offline-tts/handoff.md` 第 5.4 节
