#ifndef SENTENCESPLITTER_H
#define SENTENCESPLITTER_H

#include <QList>
#include <QString>

// 把一段回复文本切分成适合逐句朗读的句子。
//
// 这是纯自由函数：不依赖 QObject、信号槽、蓝牙或任何设备，因此可以在没有
// Android 设备的情况下直接单元测试（见 tests/tst_sentencesplitter.cpp）。
// 保持这一点很重要——一旦它沾上 QObject，本仓库就再也没有可离线验证的逻辑了。
//
// 切分规则：
//   1. 在 。！？；… 与换行处切分；分隔符**保留在前一句末尾**，朗读停顿更自然
//   2. 连续分隔符（“！！！” “。。”）只算一个边界，不产生空句
//   3. 句子短于 kMinSentenceLength 时与相邻句合并，避免孤立的短片段
//      （末句没有后继时并入前一句，绝不丢弃）
//   4. 不在数字中间切分，保护 “12。345” 这类书写
//   5. 结果已去除首尾空白，纯空白句被丢弃
//
// 注意：半角句点 `.` **不在**分隔符集合内。这是刻意的——加入它会把英文
// 缩写（Mr.）、版本号与小数（3.14）切碎，代价远大于收益。
QList<QString> splitSentences(const QString &text);

// 合并阈值（字符数）：短于此长度的句子会并入相邻句。
inline constexpr int kMinSentenceLength = 6;

#endif // SENTENCESPLITTER_H
