# 识别固定用英文,回复与朗读用中文

语音管线两端的语言是有意不对称的:使用者说**英文**,Whisper 的识别语言写死为 `"en"`;
大模型按中文 system prompt 用**中文**回答,再由中文 TTS 念出来。之所以这样,是因为使用者
本来就说英文,而所用的小号 Whisper 模型英文识别率明显高于中文;回复语言与识别语言无关,
只由 system prompt 决定。

这个决定最早冻结在 `.scratch/offline-tts/spec.md`(2026-09,「流水线语言」一行与步骤 9),
此处是把它从规格和代码注释里提出来单独留档。

## 后果

- `WhisperManager::processBuffer()` 里的 `params.language = "en"` **不是 bug**。改成 `"zh"`
  或 `"auto"` 会让转写质量下降,而原因不会出现在任何报错里。
- 回复语言要改,改的是 `src/msgsender.cpp` 的 `kSystemPrompt`(API 与 Ollama 两条路共用);
  同时 TTS 模型只会中文,换成别的回复语言就得换模型。
- 一个副作用:朗读出来的中文如果被麦克风拾到,会被按英文去识别,得到的多半是无意义的片段
  (见 `0007-mic-stays-live-while-speaking.md`)。
