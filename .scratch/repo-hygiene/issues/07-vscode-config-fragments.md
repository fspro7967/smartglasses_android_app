# 07 — .vscode 配置残缺

Status: wontfix
Type: task

## 现状

- `.vscode/task.json` 是一段不成文件的片段(单个任务对象,没有外层的 `version` / `tasks`),
  VS Code 不会读它;
- `.vscode/launch.json` 的 `preLaunchTask: "InstallDebugApp"` 在 `.vscode/tasks.json` 里不存在;
- `task.json`、`launch.json`、`c_cpp_properties.json` 里硬编码了本机绝对路径和构建套件目录名。

2026-11-11 通读代码时发现。

## 决定

**所有者 2026-11-11 决定不处理。** 要重新打开时把 Status 改回 `needs-triage`。
