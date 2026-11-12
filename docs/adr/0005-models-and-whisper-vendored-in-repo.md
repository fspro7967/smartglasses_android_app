# 模型文件与 whisper.cpp 源码直接放在仓库里

Whisper 模型(59.7 MB)、TTS 模型(约 31 MB)和整份 whisper.cpp 源码(一千多个文件)都直接
提交在仓库里,没有用 Git LFS,也没有改成构建前下载。2026-11-11 所有者决定**保持现状**:
克隆下来这些东西就在,不依赖任何外部地址还在不在。

## 考虑过的选项

- **保持现状**(选定)。
- **Git LFS**:仓库本体变小,但每个克隆的人都要装 LFS,托管方还有流量与容量配额。
- **下载脚本**:仓库最小,但构建依赖上游地址长期有效,模型版本要靠校验值钉住。

## 后果

- `.git` 约 120 MB,且每换一次模型就再涨一个模型的大小,历史里永远带着旧模型。
- 与此相对,两组预编译的 `.so` **没有**入库:sherpa-onnx / ONNX Runtime 靠
  `third_party/sherpa-onnx/download-libs.sh` 获取,OpenSSL 的目前没有任何获取办法
  (`.scratch/repo-hygiene/issues/02`)。所以「克隆即可构建」现在并不成立,
  成立的只是「模型和 whisper 源码不用另外找」。
- `.gitattributes` 把模型目录标成 `-text`,就是为了保证这些直接入库的文件不被行尾转换改坏。

工单:`.scratch/repo-hygiene/issues/09`。
