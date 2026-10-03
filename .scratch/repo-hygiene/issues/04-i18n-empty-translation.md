# 04 — 翻译目录是空的:所有 qsTr()/tr() 都是空操作

Status: needs-triage
Type: task

## 现状

`smartglasses_android_app_zh_CN.ts` 全文只有一个空 `<TS>` 元素(94 字节):

```xml
<TS version="2.1" language="zh_CN"></TS>
```

但代码里到处都是 `qsTr()` / `tr()`:`qml/Main.qml` 约 60 处,
`src/mainwindow.cpp` 里 `tr("读")`、`tr("写")`、`tr("通知")` 等特征属性名若干处。
`CMakeLists.txt:72-74` 还在 Release 构建里跑 `qt_create_translation()`,
`src/main.cpp:18-26` 也装了 `QTranslator`。

净效果:**这些全是空操作**。界面文案是硬编码在源码里的中文字符串,
「翻译」这套管线目前只增加构建时间和一层误导 —— 后人看到 `qsTr("智能眼镜助手")`
会以为改文案要去动 `.ts` 文件。

## 需要所有者回答的

1. 这个 app 要不要多语言?
   - **不要**:那就别留半套 i18n。可以保留 `qsTr()` 包裹(以后要加不亏),
     但应删除 `qt_create_translation()` 与 `QTranslator` 装载,或至少加注释说明
     「当前只有一个语言,`.ts` 不维护」。
   - **要**:需要把现存的文案抽进 `.ts` 并真正维护,
     同时决定默认语言(`zh_CN`)与回退语言,以及 `main.cpp:20-26` 的
     `uiLanguages()` 匹配逻辑是否够用(它按系统语言逐个找,找不到就静默用源码字符串)。
2. 顺带:工程名叫 `smartglasses_android_app`,而 `/data/SmartGlasses/` 下现在还有
   `smartglasses_board`(固件)。如果三者(手机 app、固件、未来的眼镜端)会有共享术语,
   值得在 `.scratch/` 或 `CONTEXT.md` 里统一(这次分析没有动域模型文档)。

## 完成判据

- [ ] 二选一有了明确结论
- [ ] 若不做:构建里不再有无效的翻译步骤,或有一行注释说明为何保留
- [ ] 若做:至少一条文案走通「改 `.ts` → 重新构建 → 界面变化」

## 相关

- 规格的已知风险表里已记过这条(`.scratch/offline-tts/spec.md` 第四节),
  当时作为「不要以为现有 i18n 生效」的提醒,没有变成工单
