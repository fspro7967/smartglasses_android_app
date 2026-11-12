# 06 — API Key 明文存储且可被备份带走;全局允许明文流量;包名仍是示例值

Status: wontfix
Type: task

## 现状

- `MsgSender::setApiConfig()` 把 API Key 明文写进 `QSettings`(`src/msgsender.cpp`)。
- `android/AndroidManifest.xml`:`android:allowBackup="true"`——应用数据(含上面的 Key)
  会进入系统备份 / `adb backup`;`android:usesCleartextTraffic="true"`——全局允许 http
  (局域网 Ollama 需要它);`package="org.qtproject.example"`——Qt 模板的示例包名。

2026-11-11 通读代码时发现。

## 当时给出的选项

- A. 关掉 `allowBackup`,明文流量保留。
- B. 在 A 的基础上用 Android Keystore 加密 Key。
- C. 再改正式包名(会变成另一个应用,需卸载重装)。

## 决定

**所有者 2026-11-11 决定不处理。** 要重新打开时把 Status 改回 `needs-triage`。

留一句供将来参考:A 是一行清单改动,且不影响任何现有功能;如果哪天这个应用要装到
不属于开发者本人的手机上,建议至少做 A。
