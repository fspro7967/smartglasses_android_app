# 现状记录:仓库卫生(构建、密钥、测试、配置)

**状态:** 盘点记录,不是冻结规格。工单来自两次通读(2026-10-03、2026-11-11)
**日期:** 2026-11-11

---

## 这份文档要做什么

收纳与功能无关、但决定「别人拿到这个仓库能不能构建、敢不敢改」的问题:构建脚本、
入库的密钥与二进制、测试与 CI、编辑器配置、README。每一条都是事后发现的,没有统一的目标状态;
本文只给出目录和已经做出的决定。

## 一、已经做出的决定

| 决定 | 留档 |
|---|---|
| 仓库里的签名密钥与口令是有意公开的测试/演示密钥,不轮换、不清历史 | `docs/adr/0001-committed-test-signing-key.md`,`issues/01` |
| Release 构建启用 `-march=armv8.2-a+fp16+dotprod`,最低硬件要求随之抬到 ARMv8.2-A | `docs/adr/0004-require-armv8-2.md`,`issues/05` |
| 两个模型文件与整份 whisper.cpp 源码保持直接入库,不用 LFS 或下载脚本 | `docs/adr/0005-models-and-whisper-vendored-in-repo.md`,`issues/09` |
| API Key 存储 / 备份 / 明文流量 / 包名、`.vscode` 配置、CMake 杂项、README 排版:不处理 | `issues/06`、`07`、`08`、`10`(均 wontfix) |

## 二、工单

| # | 标题 | 状态 |
|---|---|---|
| 01 | 签名密钥与口令被跟踪进仓库 | resolved(认定为测试密钥) |
| 02 | OpenSSL 预编译库没入库也没有拉取脚本 | ready-for-agent |
| 03 | 只有少量测试,没有 CI,CMake 用 `file(GLOB)` | needs-triage(已补 2 组测试) |
| 04 | 翻译目录是空的 | needs-triage |
| 05 | CMake 的 OpenMP 开关与优化选项没有生效 | resolved |
| 06 | API Key 明文 / 可备份 / 明文流量 / 示例包名 | wontfix |
| 07 | `.vscode` 配置残缺 | wontfix |
| 08 | CMake 杂项 | wontfix |
| 09 | 模型与第三方源码直接入库 | wontfix(保持现状) |
| 10 | README 的排版与措辞 | wontfix |

状态以各工单文件的 `Status:` 行为准,这张表只是目录。

## 三、还没人决定的事

`issues/02`、`03`、`04` 三张。其中只有 `02` 会让人**构建不了**:全新克隆缺 OpenSSL 的 `.so`,
而仓库里没有任何获取它们的办法(sherpa-onnx 的 `.so` 同样不入库,但有
`third_party/sherpa-onnx/download-libs.sh`)。`03`、`04` 是「要不要做、做到哪一步」的问题。
