# 03 — 未提交的 whisper 参数改动:只有 suppress_nst 真的变了,方向与注释相反

Status: needs-triage
Type: task

## 现状

工作区未提交的 diff 把 `src/whisper_manager.cpp:121-129` 的六行参数注释掉了:

```cpp
//排除空白token
//params.suppress_blank = true;
//params.suppress_nst = true;
params.audio_ctx = 278 ;
//解码与置信度阈值，防止幻觉
//params.temperature = 0.0f;
//params.temperature_inc = 0.2f;
//params.entropy_thold = 2.4f;
//params.logprob_thold = -1.0f;
```

## 关键事实:六行里只有一行改变了行为

`whisper_full_default_params()` 的默认值(vendored 代码
`third_party/whisper.cpp/src/whisper.cpp:5969-5978`):

| 参数 | 库默认值 | 被注释掉的值 | 注释后是否变化 |
|---|---|---|---|
| `suppress_blank` | `true` | `true` | 否 |
| `suppress_nst` | `false` | `true` | **是(true → false)** |
| `temperature` | `0.0f` | `0.0f` | 否 |
| `temperature_inc` | `0.2f` | `0.2f` | 否 |
| `entropy_thold` | `2.4f` | `2.4f` | 否 |
| `logprob_thold` | `-1.0f` | `-1.0f` | 否 |

也就是说:这次改动的**唯一净效果**是把 `suppress_nst` 从 true 恢复成库默认的 false,
即**重新允许非语音 token**。而紧挨着的注释写的是「排除空白token」和「防止幻觉」——
注释描述的是被注释掉的那些行的意图,现在它们不生效了。改动的方向与注释相反。

另外 `params.audio_ctx = 278` 仍然生效,它是把 Whisper 的音频上下文从默认 1500 压到 278
的速度换精度开关,不在本次 diff 里,但值得知道当前识别质量是建立在这个值上的。

## 需要所有者回答的

1. 这次改动是**有意的**(为了排查某个具体的幻觉/漏字问题),还是调试残留?
2. 如果是有意的:请把 `suppress_nst = false` 的理由写进注释(替代现在那两行失效的注释),
   然后提交。
3. 如果是调试残留:恢复 `params.suppress_nst = true`,并考虑把其余五行**删除**
   (它们本来就等于默认值,留着只会让人误以为设置了什么),而不是留成注释。

无论选哪个,不要再把「等于默认值的赋值」和「真正生效的参数」混在同一段注释里 ——
这次的坑正是从这里来的。

## 完成判据

- [ ] `whisper_manager.cpp` 里生效的参数集合是明确的
- [ ] 每条生效参数旁边有说明它为什么不是默认值
- [ ] 工作区不再残留未提交的参数改动

## 相关

- 语言不对称(刻意写死 `en`)的说明在同文件 `:108-117`,那是**另一件事**,
  已在 `.scratch/offline-tts/spec.md` 步骤 9 冻结,不要一起动。
- 采集侧问题:`.scratch/voice-pipeline/issues/01`、`issues/02`
