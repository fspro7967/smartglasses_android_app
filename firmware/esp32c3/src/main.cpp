// =============================================================================
// ESP32-C3 BLE 音频收发固件（智能眼镜 APP 适配）
//
// 面向手机端 APP 的 BoardProtocol（includes/devicehandler.h）：
//   * 控制特征  bebe…26a8  接收 "play" / "stop" / "volume:N" / "rate:N" / "status" /
//                          "echo" / "mic:auto|force|off"，并以 notify 回执
//   * 上行音频  6666…6666  INMP441 采集的 16 kHz 单声道 int16 PCM，notify 给手机
//   * 下行播放  8888…8888  手机回传的单声道 int16 PCM（默认 16 kHz，可用 rate:N
//                          指定 8 kHz 等整数约数，板上插值到 16 kHz），写进 MAX98357
//
// 相对旧固件修掉的关键问题：
//   1. BLEDevice::setMTU(517)：旧固件本地 MTU 停在 23，手机端分片只有 20 B，
//      下行被 1 包/连接间隔的 ACK 往返锁死。抬高 MTU 后 APP 每包 ~509 B。
//   2. 播放特征加 PROPERTY_WRITE_NR：APP 的 setWriteTarget() 会优先选无响应写，
//      去掉每条分片等待 ACK 的往返延迟。
//   3. 播放环形缓冲从 512 样本（32 ms）扩到 4096 样本（256 ms），抗抖动。
//   4. 连接后请求 7.5–15 ms 的高优先级连接间隔。
//   5. 环形缓冲读写跨任务（BLE 回调 vs loop）加临界区，修数据竞争。
//   6. 麦克风检测可关闭：编译开关 MIC_DETECTION_ENABLED，或运行时 mic:force。
//   7. 下行采样率可配置（控制指令 rate:N）：手机侧 Qt Android 把每次写入串行化，
//      实测只能持续约 24 KB/s，低于 16 kHz int16 所需的 32 KB/s。让手机直接发 TTS
//      原生 8 kHz（16 KB/s），由板子线性插值到 I2S 的 16 kHz，带宽需求减半。
//
// 引脚（按实机接线确认）：
//   INMP441 : SCK→GPIO0  WS→GPIO2  SD→GPIO1  L/R→GND  VDD→3.3V
//   MAX98357: BCLK→GPIO0 LRC→GPIO2 DIN→GPIO3 VIN→5V   SD→VIN  GAIN→悬空
// =============================================================================

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <HardwareSerial.h>
#include <driver/i2s.h>
#include <math.h>

// ===== BLE UUID（必须与 APP 的 BoardProtocol 一致）=====
#define SERVICE_UUID          "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CONTROL_CHAR_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define AUDIO_CHAR_UUID       "66666666-6666-6666-6666-666666666666"
#define PLAYBACK_CHAR_UUID    "88888888-8888-8888-8888-888888888888"
#define DEVICE_NAME           "ESP32_Audio"

// ===== 音频格式（APP 固定按 16 kHz 收发）=====
#define SAMPLE_RATE           16000
#define SAMPLES_PER_READ      64          // 每次 i2s_read 的样本数（4 ms @16k）

// ===== 上行（麦克风→手机）流量控制 =====
#define BLE_PREFERRED_MTU     517         // 期望协商的 ATT MTU（APP 请求 512）
#define AUDIO_CHUNK_SAMPLES   240         // 目标单包样本数（480 B），实际受 MTU 限制
#define AUDIO_ACCUM_SAMPLES   (AUDIO_CHUNK_SAMPLES * 2)
#define AUDIO_BACKOFF_MS      30          // 检测到 ATT 通道拥塞后的退避毫秒数
#define AUDIO_MTU_MIN_WARN    64          // MTU 低于此值无法承载音频，仅提示一次

// ===== 下行（手机→功放）播放配置 =====
#define PLAYBACK_BUFFER_SIZE  4096        // 环形缓冲样本数（256 ms @16k，抗抖动）
#define PLAYBACK_CHUNK        128         // 每次写入 I2S 的样本数上限
// 下行 PCM 的默认输入采样率；可被控制指令 rate:N 改写。必须能整除 SAMPLE_RATE
// （I2S 固定 16 kHz，板上只做整数倍线性插值，不做分数倍重采样）。
#define PLAYBACK_RATE_DEFAULT SAMPLE_RATE

// ===== I2S 引脚（ESP32-C3 只有 1 个 I2S，麦克风与功放全双工共用）=====
#define I2S_PORT              I2S_NUM_0
#define I2S_BCK_PIN           0           // INMP441 SCK / MAX98357 BCLK
#define I2S_WS_PIN            2           // INMP441 WS  / MAX98357 LRC
#define I2S_SD_PIN            1           // INMP441 SD  （数据输出）
#define I2S_DOUT_PIN          3           // MAX98357 DIN（数据输入）

// ===== 麦克风存在性检测 =====
// 若数据线悬空（未接麦克风），I2S 样本几乎恒定；真实麦克风的交流本底噪声会让
// 窗口内交流 RMS > 阈值。编译开关设为 0 可完全去掉检测，等价于始终发送上行。
#ifndef MIC_DETECTION_ENABLED
#define MIC_DETECTION_ENABLED 1
#endif
#define MIC_DETECT_WINDOW_SAMPLES 1600    // 判定窗口：100 ms @16k
#define MIC_WARMUP_SAMPLES        800     // 启动后丢弃 50 ms，等 I2S 稳定
#define MIC_AC_RMS_THRESHOLD      0.5f    // 交流 RMS 阈值（LSB）

// ===== 音量 =====
#define VOLUME_DEFAULT        180         // 0..255

// ===== 全局对象 =====
BLEServer          *pServer        = nullptr;
BLECharacteristic  *pControlChar   = nullptr;
BLECharacteristic  *pAudioChar     = nullptr;
BLECharacteristic  *pPlaybackChar  = nullptr;

volatile bool     deviceConnected = false;
volatile uint16_t negotiatedMTU  = 23;
bool              mtuWarned       = false;

// ===== 上行流量控制状态 =====
int16_t  audioAccum[AUDIO_ACCUM_SAMPLES];
int      audioAccumCount   = 0;
volatile uint32_t audioBackoffUntil = 0;
uint32_t audioPacketCount  = 0;
uint32_t audioDropCount    = 0;
uint32_t audioCongestCount = 0;

// ===== 麦克风状态 =====
enum MicState { MIC_UNKNOWN, MIC_PRESENT, MIC_ABSENT };
enum MicMode  { MIC_MODE_AUTO, MIC_MODE_FORCE, MIC_MODE_OFF };
volatile MicState micState = MIC_UNKNOWN;
volatile MicMode  micMode  = MIC_MODE_AUTO;

#if MIC_DETECTION_ENABLED
int16_t micDetectBuffer[MIC_DETECT_WINDOW_SAMPLES];
int     micDetectCount     = 0;
int     micWarmupRemaining = MIC_WARMUP_SAMPLES;
#endif

const char *micStateStr() {
    switch (micState) {
        case MIC_PRESENT: return "present";
        case MIC_ABSENT:  return "absent";
        default:          return "unknown";
    }
}

const char *micModeStr() {
    switch (micMode) {
        case MIC_MODE_FORCE: return "force";
        case MIC_MODE_OFF:   return "off";
        default:             return "auto";
    }
}

// ===== 播放状态（环形缓冲跨任务访问，用临界区保护）=====
portMUX_TYPE playbackMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool    isPlaying = false;
volatile uint8_t volume    = VOLUME_DEFAULT;
int16_t          playbackBuffer[PLAYBACK_BUFFER_SIZE];
int              playbackWriteIndex = 0;
int              playbackReadIndex  = 0;
int              playbackAvailable  = 0;
int16_t          txBuffer[PLAYBACK_CHUNK];

// 下行分片奇偶对齐：APP 每次写 mtu-3 字节，MTU=512 时为 509（奇数）。
// 把每次写入末尾的半个样本带到下一次，否则样本相位逐包错位、播放变调。
// 控制回调与播放回调都在 BLE 任务里执行，这两个变量不跨任务，无需加锁。
uint8_t playbackCarryByte    = 0;
bool    playbackCarryPending = false;

// 下行输入采样率与插值状态。playbackUpsample = SAMPLE_RATE / playbackInRate（整数倍）。
// 插值需要上一个输入样本做左端点，跨包保持，否则包边界处会出现折点。
volatile int playbackInRate   = PLAYBACK_RATE_DEFAULT;
int          playbackUpsample = 1;
int16_t      playbackPrevSample = 0;
bool         playbackHavePrev   = false;
// 缓冲满而丢弃的样本数（输出侧），通过 status 回执上报，便于真机判断是否欠流控
volatile uint32_t playbackDropCount = 0;

// ===== 前向声明 =====
int  audioChunkSamples();
void flushAudio();
void initI2S();

// ===== 依据协商 MTU 计算单包可发送的样本数 =====
int audioChunkSamples() {
    int usable = (int)negotiatedMTU - 3;   // ATT 通知负载上限 = MTU - 3
    if (usable < (int)sizeof(int16_t)) {
        usable = (int)sizeof(int16_t);
    }
    int samples = usable / (int)sizeof(int16_t);
    if (samples > AUDIO_CHUNK_SAMPLES) {
        samples = AUDIO_CHUNK_SAMPLES;
    }
    return samples;
}

// ===== 把累积样本按 MTU 允许的最大包 notify 出去 =====
void flushAudio() {
    if (millis() < audioBackoffUntil) {
        audioDropCount += audioAccumCount;
        audioAccumCount = 0;
        return;
    }

    const int chunk = audioChunkSamples();
    while (audioAccumCount >= chunk) {
        pAudioChar->setValue((uint8_t *)audioAccum, chunk * sizeof(int16_t));
        pAudioChar->notify();
        audioPacketCount++;

        const int remaining = audioAccumCount - chunk;
        if (remaining > 0) {
            memmove(audioAccum, audioAccum + chunk, remaining * sizeof(int16_t));
        }
        audioAccumCount = remaining;

        if (millis() < audioBackoffUntil) {   // 刚发就拥塞：丢弃剩余，保证实时
            audioDropCount += audioAccumCount;
            audioAccumCount = 0;
            break;
        }
    }

    static uint32_t lastReport = 0;
    if (audioPacketCount - lastReport >= 200) {
        lastReport = audioPacketCount;
        Serial.printf("[上行] 已发 %lu 包 (MTU=%u, %d 样本/包, 丢弃=%lu, 拥塞=%lu)\n",
                      (unsigned long)audioPacketCount, negotiatedMTU, chunk,
                      (unsigned long)audioDropCount, (unsigned long)audioCongestCount);
    }
}

// ===== 读取 INMP441 原始 PCM =====
int readRawAudio(int16_t *samples, int count) {
    size_t bytesRead = 0;
    if (i2s_read(I2S_PORT, samples, count * sizeof(int16_t),
                 &bytesRead, portMAX_DELAY) != ESP_OK) {
        return 0;
    }
    return bytesRead / sizeof(int16_t);
}

// ===== 一阶 DC Blocker（去掉 INMP441 的直流偏置）=====
void applyDcBlocker(int16_t *samples, int count) {
    static int16_t prevSample = 0;
    static int32_t filteredState = 0;
    const int32_t R = 32580;   // ≈ 0.994 × 32768，高通约 15 Hz @16k
    for (int i = 0; i < count; i++) {
        const int32_t in  = samples[i];
        const int32_t out = in - prevSample + ((filteredState * R) >> 15);
        samples[i] = (int16_t)out;
        filteredState = out;
        prevSample = in;
    }
}

#if MIC_DETECTION_ENABLED
// ===== 判断窗口内是否真的接入了麦克风 =====
bool analyzeMicPresence(const int16_t *samples, int count) {
    if (count <= 0) {
        return false;
    }

    int16_t vmin = samples[0];
    int16_t vmax = samples[0];
    int64_t sum = 0;
    int64_t sumSq = 0;
    for (int i = 0; i < count; i++) {
        const int32_t v = samples[i];
        if (v < vmin) vmin = v;
        if (v > vmax) vmax = v;
        sum += v;
        sumSq += (int64_t)v * (int64_t)v;
    }

    if (vmin == vmax) {   // 数据线被钳死/恒定输出：没有器件在驱动
        return false;
    }

    const double mean = (double)sum / (double)count;
    double acPower = (double)sumSq / (double)count - mean * mean;
    if (acPower < 0.0) {
        acPower = 0.0;
    }
    return sqrt(acPower) >= MIC_AC_RMS_THRESHOLD;
}

void updateMicState(bool present) {
    const MicState next = present ? MIC_PRESENT : MIC_ABSENT;
    if (next == micState) {
        return;
    }
    micState = next;
    Serial.printf("[提示] 麦克风%s，%s上行音频\n",
                  present ? "已接入" : "未检测到",
                  present ? "恢复" : "暂停");
}

// 用原始（未去直流）样本判定，避免 DC Blocker 把恒定输入抹成 0 掩盖真实电平。
void updateMicPresence(const int16_t *samples, int count) {
    if (micWarmupRemaining > 0) {
        const int skip = (count < micWarmupRemaining) ? count : micWarmupRemaining;
        samples += skip;
        count -= skip;
        micWarmupRemaining -= skip;
        if (count <= 0) {
            return;
        }
    }

    while (count > 0) {
        const int space = MIC_DETECT_WINDOW_SAMPLES - micDetectCount;
        const int take  = (count < space) ? count : space;
        memcpy(&micDetectBuffer[micDetectCount], samples, take * sizeof(int16_t));
        micDetectCount += take;
        samples += take;
        count -= take;

        if (micDetectCount >= MIC_DETECT_WINDOW_SAMPLES) {
            micDetectCount = 0;
            updateMicState(analyzeMicPresence(micDetectBuffer, MIC_DETECT_WINDOW_SAMPLES));
        }
    }
}
#endif // MIC_DETECTION_ENABLED

// ===== 当前是否应发送上行音频 =====
bool shouldSendUplink() {
    switch (micMode) {
        case MIC_MODE_OFF:   return false;
        case MIC_MODE_FORCE: return true;
        default:
#if MIC_DETECTION_ENABLED
            return micState == MIC_PRESENT;
#else
            return true;
#endif
    }
}

// ===== 把环形缓冲的样本经音量缩放后写入 I2S 功放 =====
void processPlayback() {
    if (!isPlaying) {
        return;   // TX 欠载由 tx_desc_auto_clear 自动输出静音
    }

    int count = 0;
    portENTER_CRITICAL(&playbackMux);
    while (count < PLAYBACK_CHUNK && playbackAvailable > 0) {
        txBuffer[count++] = playbackBuffer[playbackReadIndex];
        playbackReadIndex = (playbackReadIndex + 1) % PLAYBACK_BUFFER_SIZE;
        playbackAvailable--;
    }
    portEXIT_CRITICAL(&playbackMux);

    if (count == 0) {
        return;
    }

    const int32_t vol = volume;
    for (int i = 0; i < count; i++) {
        txBuffer[i] = (int16_t)((int32_t)txBuffer[i] * vol / 255);
    }

    size_t bytesWritten = 0;
    i2s_write(I2S_PORT, txBuffer, count * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
}

// ===== I2S 全双工初始化 =====
void initI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,   // 两端都选左声道
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = SAMPLES_PER_READ,
        .use_apll = false,
        .tx_desc_auto_clear = true,   // 欠载时输出静音，避免重复旧样本
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .mck_io_num   = I2S_PIN_NO_CHANGE,
        .bck_io_num   = I2S_BCK_PIN,
        .ws_io_num    = I2S_WS_PIN,
        .data_out_num = I2S_DOUT_PIN,
        .data_in_num  = I2S_SD_PIN
    };

    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("[错误] I2S 驱动安装失败: %d\n", err);
        return;
    }
    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[错误] I2S 引脚设置失败: %d\n", err);
        return;
    }
    i2s_start(I2S_PORT);

    Serial.printf("I2S 全双工就绪 @ %d Hz\n", SAMPLE_RATE);
    Serial.printf("  INMP441 : BCK=GPIO%d WS=GPIO%d SD=GPIO%d\n",
                  I2S_BCK_PIN, I2S_WS_PIN, I2S_SD_PIN);
    Serial.printf("  MAX98357: BCLK=GPIO%d LRC=GPIO%d DIN=GPIO%d\n",
                  I2S_BCK_PIN, I2S_WS_PIN, I2S_DOUT_PIN);
}

// ===== 清空播放环形缓冲（临界区）=====
void resetPlaybackBuffer() {
    portENTER_CRITICAL(&playbackMux);
    playbackWriteIndex = 0;
    playbackReadIndex  = 0;
    playbackAvailable  = 0;
    portEXIT_CRITICAL(&playbackMux);
    playbackCarryByte    = 0;     // 新会话从字节边界重新开始
    playbackCarryPending = false;
    playbackHavePrev     = false; // 新会话第一个样本前没有左端点
    playbackPrevSample   = 0;
}

// ===== BLE 服务器回调 =====
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *server) {
        deviceConnected = true;
    }

    void onConnect(BLEServer *server, esp_ble_gatts_cb_param_t *param) {
        deviceConnected = true;
        audioAccumCount = 0;
        const uint16_t mtu = server->getPeerMTU(param->connect.conn_id);
        negotiatedMTU = (mtu >= 23) ? mtu : 23;
        mtuWarned = false;
        Serial.printf("[提示] 手机已连接，MTU=%u，单包最多 %d 样本\n",
                      negotiatedMTU, audioChunkSamples());

        // 高优先级连接参数：7.5–15 ms 间隔，0 延迟，2 s 监督超时
        server->updateConnParams(param->connect.remote_bda, 6, 12, 0, 200);
    }

    void onMtuChanged(BLEServer *server, esp_ble_gatts_cb_param_t *param) {
        negotiatedMTU = param->mtu.mtu;
        Serial.printf("[提示] MTU 协商为 %u，单包最多 %d 样本\n",
                      negotiatedMTU, audioChunkSamples());
        if (negotiatedMTU < AUDIO_MTU_MIN_WARN && !mtuWarned) {
            mtuWarned = true;
            Serial.println("[警告] MTU 过小，上行音频将大量拥塞丢包");
        }
    }

    void onDisconnect(BLEServer *server) {
        deviceConnected = false;
        audioAccumCount = 0;
        Serial.println("[提示] 手机已断开，重新广播");
        BLEDevice::startAdvertising();
    }
};

// ===== 上行通知状态回调：拥塞退避 =====
class AudioNotifyCallbacks : public BLECharacteristicCallbacks {
    void onStatus(BLECharacteristic *characteristic, Status s, uint32_t code) {
        if (s == Status::ERROR_GATT) {
            audioBackoffUntil = millis() + AUDIO_BACKOFF_MS;
            audioCongestCount++;
            if (audioCongestCount <= 5 || (audioCongestCount % 500) == 0) {
                Serial.printf("[警告] BLE 发送拥塞，退避 %d ms（累计 %lu 次）\n",
                              AUDIO_BACKOFF_MS, (unsigned long)audioCongestCount);
            }
        }
    }
};

// ===== 控制特征回调：接收指令并以 notify 回执 =====
class ControlCallbacks : public BLECharacteristicCallbacks {
    void reply(BLECharacteristic *c, const String &text) {
        c->setValue(std::string(text.c_str()));
        c->notify();
    }

    void onWrite(BLECharacteristic *characteristic) {
        const std::string value = characteristic->getValue();
        if (value.empty()) {
            return;
        }
        const String cmd = String(value.c_str());
        Serial.printf("[控制] 收到: %s\n", cmd.c_str());

        if (cmd == "play") {
            resetPlaybackBuffer();
            isPlaying = true;
            reply(characteristic, "OK:playing");
        } else if (cmd == "stop") {
            isPlaying = false;
            resetPlaybackBuffer();
            reply(characteristic, "OK:stopped");
        } else if (cmd.startsWith("volume:")) {
            const int vol = cmd.substring(7).toInt();
            if (vol >= 0 && vol <= 100) {
                volume = (uint8_t)map(vol, 0, 100, 0, 255);
                Serial.printf("[控制] 音量 %d%%\n", vol);
                reply(characteristic, "OK:volume:" + String(vol));
            } else {
                reply(characteristic, "ERROR:volume 0-100");
            }
        } else if (cmd.startsWith("rate:")) {
            // 仅在未播放时允许切换，避免插值倍数在一段连续音频中途改变
            const int rate = cmd.substring(5).toInt();
            if (isPlaying) {
                reply(characteristic, "ERROR:rate only when stopped");
            } else if (rate > 0 && rate <= SAMPLE_RATE && (SAMPLE_RATE % rate) == 0) {
                playbackInRate   = rate;
                playbackUpsample = SAMPLE_RATE / rate;
                resetPlaybackBuffer();
                Serial.printf("[控制] 下行采样率 %d Hz（板上 %d 倍插值）\n", rate, playbackUpsample);
                reply(characteristic, "OK:rate:" + String(rate));
            } else {
                reply(characteristic, "ERROR:rate must divide 16000");
            }
        } else if (cmd == "status") {
            int bufferAvail;
            portENTER_CRITICAL(&playbackMux);
            bufferAvail = playbackAvailable;
            portEXIT_CRITICAL(&playbackMux);
            const int volPercent = map(volume, 0, 255, 0, 100);
            String response = "status:";
            response += isPlaying ? "playing" : "stopped";
            response += ",volume:" + String(volPercent);
            response += ",buffer:" + String(bufferAvail);
            response += ",rate:" + String((int)playbackInRate);
            response += ",drop:" + String((unsigned long)playbackDropCount);
            response += ",mic:" + String(micStateStr());
            response += ",micmode:" + String(micModeStr());
            reply(characteristic, response);
        } else if (cmd == "echo") {
            reply(characteristic, "echo:hello from ESP32");
        } else if (cmd == "mic:auto") {
            micMode = MIC_MODE_AUTO;
            reply(characteristic, "OK:micmode:auto");
        } else if (cmd == "mic:force") {
            micMode = MIC_MODE_FORCE;
            reply(characteristic, "OK:micmode:force");
        } else if (cmd == "mic:off") {
            micMode = MIC_MODE_OFF;
            reply(characteristic, "OK:micmode:off");
        } else {
            Serial.println("[控制] 未知指令");
            reply(characteristic, "ERROR:unknown command");
        }
    }
};

// ===== 播放特征回调：接收手机回传的 16 kHz int16 PCM =====
class PlaybackCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) {
        if (!isPlaying) {
            return;   // 未 play 前丢弃，避免把非音频数据当 PCM 播出去
        }

        std::string data = characteristic->getValue();
        // 先接上上一包遗留的半个样本，让整个下行音频流保持连续字节对齐
        if (playbackCarryPending) {
            data.insert(data.begin(), (char)playbackCarryByte);
            playbackCarryPending = false;
        }
        if (data.size() % sizeof(int16_t)) {   // 奇数长度：末尾半个样本留到下一包
            playbackCarryByte = (uint8_t)data.back();
            data.pop_back();
            playbackCarryPending = true;
        }

        const int n = (int)(data.size() / sizeof(int16_t));
        if (n <= 0) {
            return;
        }

        const uint8_t *p = (const uint8_t *)data.data();
        const int up = playbackUpsample;   // 1 = 不插值
        portENTER_CRITICAL(&playbackMux);
        for (int i = 0; i < n; i++) {
            const int16_t cur = (int16_t)(uint16_t)(p[2 * i] | (p[2 * i + 1] << 8));  // 小端

            // 在 prev 与 cur 之间线性插出 up 个输出样本（最后一个恰为 cur）。
            // 第一个样本没有左端点，直接当作 prev 用，避免从 0 起跳产生爆音。
            const int32_t prev = playbackHavePrev ? playbackPrevSample : cur;
            bool full = false;
            for (int k = 1; k <= up; k++) {
                const int next = (playbackWriteIndex + 1) % PLAYBACK_BUFFER_SIZE;
                if (next == playbackReadIndex) {   // 缓冲满，丢弃新样本并计数
                    playbackDropCount++;
                    full = true;
                    break;
                }
                playbackBuffer[playbackWriteIndex] =
                    (int16_t)(prev + ((int32_t)(cur - prev) * k) / up);
                playbackWriteIndex = next;
                playbackAvailable++;
            }
            playbackPrevSample = cur;
            playbackHavePrev   = true;
            if (full) {
                break;
            }
        }
        portEXIT_CRITICAL(&playbackMux);
    }
};

// ===== 主初始化 =====
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- ESP32-C3 BLE 音频收发器 ---");

    initI2S();

    BLEDevice::init(DEVICE_NAME);
    BLEDevice::setMTU(BLE_PREFERRED_MTU);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *service = pServer->createService(SERVICE_UUID);

    // 控制特征：READ / WRITE / WRITE_NR / NOTIFY
    pControlChar = service->createCharacteristic(
        CONTROL_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR |
        BLECharacteristic::PROPERTY_NOTIFY);
    pControlChar->setCallbacks(new ControlCallbacks());
    pControlChar->setValue(std::string("ESP32 Audio Ready"));
    pControlChar->addDescriptor(new BLE2902());

    // 上行音频特征：READ / NOTIFY
    pAudioChar = service->createCharacteristic(
        AUDIO_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY);
    pAudioChar->setCallbacks(new AudioNotifyCallbacks());
    pAudioChar->addDescriptor(new BLE2902());

    // 下行播放特征：READ / WRITE / WRITE_NR / NOTIFY
    // WRITE_NR 是关键：APP 会优先用无响应写，避免每包等 ACK 把带宽锁死。
    pPlaybackChar = service->createCharacteristic(
        PLAYBACK_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR |
        BLECharacteristic::PROPERTY_NOTIFY);
    pPlaybackChar->setCallbacks(new PlaybackCallbacks());
    pPlaybackChar->addDescriptor(new BLE2902());

    service->start();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(false);
    advertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();

#if !MIC_DETECTION_ENABLED
    micState = MIC_PRESENT;   // 检测被编译掉：对外总是「可发送」
#endif

    Serial.println("========================================");
    Serial.printf("蓝牙启动成功，设备名: %s\n", DEVICE_NAME);
    Serial.println("控制指令: play / stop / volume:0-100 / rate:N / status / echo");
    Serial.println("          mic:auto / mic:force / mic:off");
    Serial.printf("麦克风检测: %s", MIC_DETECTION_ENABLED ? "开启" : "关闭");
#if MIC_DETECTION_ENABLED
    Serial.printf("（窗口 %d 样本，交流 RMS 阈值 %.2f）\n",
                  MIC_DETECT_WINDOW_SAMPLES, MIC_AC_RMS_THRESHOLD);
#else
    Serial.println();
#endif
    Serial.println("========================================");
}

// ===== 主循环 =====
void loop() {
    if (deviceConnected) {
        int16_t rawSamples[SAMPLES_PER_READ];
        const int samplesRead = readRawAudio(rawSamples, SAMPLES_PER_READ);

        if (samplesRead > 0) {
#if MIC_DETECTION_ENABLED
            if (micMode == MIC_MODE_AUTO) {
                updateMicPresence(rawSamples, samplesRead);
            }
#endif
            applyDcBlocker(rawSamples, samplesRead);

            if (shouldSendUplink()) {
                for (int i = 0; i < samplesRead; i++) {
                    if (audioAccumCount < AUDIO_ACCUM_SAMPLES) {
                        audioAccum[audioAccumCount++] = rawSamples[i];
                    } else {
                        audioDropCount++;
                    }
                }
                flushAudio();
            } else {
                audioAccumCount = 0;
            }
        }
    } else {
        audioAccumCount = 0;
        delay(20);
    }

    // 播放（手机→MAX98357）：i2s_write 按 DMA 速率阻塞，自动保持 16 kHz
    processPlayback();
}
