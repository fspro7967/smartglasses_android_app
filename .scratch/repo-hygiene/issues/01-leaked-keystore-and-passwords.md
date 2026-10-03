# 01 — 签名密钥与口令被跟踪进仓库(README 说它们不在版本控制里)

Status: ready-for-human
Type: bug

## 事实

两个文件**已被 git 跟踪**:

```
$ git ls-files .vscode/settings.json android_release.keystore
.vscode/settings.json
android_release.keystore
```

而仓库根 `README.md:8` 写的是:

> The keystore path, store/key passwords and alias are configured in `settings.json`
> (see `.vscode/settings.example.json`); **they are intentionally kept out of version control.**

`.gitignore` 也确实写了规则(`.vscode/settings.json`、`*.keystore`),但对**已经跟踪**的
文件无效。`.vscode/settings.json` 里 `STORE_PASS` / `KEY_PASS` 是明文。

`.scratch/offline-tts/handoff.md` 第 5.1 节记录:**仓库所有者已知晓此情况并选择暂不处理**
(该文档成文时仓库是 public)。因此本工单不是「新发现」,而是把它从交接文档里
搬进工单系统,免得随交接文档一起沉底。

## 为什么值得单独立一张

- Release 构建用的私钥一旦公开,用它签出来的包就不具备任何安全性;
  而换 key 会导致已安装用户 `INSTALL_FAILED_UPDATE_INCOMPATIBLE`。
  两条路都有代价,所以这是一个**必须由所有者拍板**的决策,不是技术债。
- 光删文件不够:**git 历史里还在**。任何清理方案都要明确回答「历史怎么处理」。
- 临时缓解措施(交接文档里已写)值得固化到工单里:
  - 日常验证用 **Debug 构建**,它不需要 keystore;
  - 不要把这把 key 复制到别处,也不要基于它做正式发布。

## 可选方向(需要所有者选)

1. **轮换**:生成新 keystore、新口令,`git rm --cached` 两个文件,更新 README;
   接受老安装包必须卸载重装。
2. **重写历史**:轮换 + `git filter-repo` 清掉两个文件的历史,然后强推。
   代价是所有协作者要重新 clone。
3. **接受现状**:明确记录「这把 key 只用于内部/演示,不用于对外发布」,
   并把 README 里那句「intentionally kept out of version control」改成与事实一致的说法
   —— **至少不要让文档继续撒谎**,否则下一个人会以为自己提交的是脱敏文件。

不论选哪个,顺带检查:
- `.vscode/settings.example.json` 的说明是否足够(它现在是对的:让使用者复制后自填);
- `android_release.keystore` 是否真的必须入库(交接文档说用它才能保持升级兼容)。

## 完成判据

- [ ] README 第 8 行的说法与仓库事实一致
- [ ] 所有者对一个方向(轮换 / 重写历史 / 接受并记录)有明确结论
- [ ] 结论写进 `docs/adr/` 或本工单的 `## Answer`(这类决策值得留档)

## 相关

- 交接文档原文:`.scratch/offline-tts/handoff.md` 第 5.1 节
- 另一条「只在 fresh clone 暴露」的构建问题:`issues/02`
