#include "sentencesplitter.h"

#include <QChar>

namespace {

// 句末标点与换行。除换行外都是中文全角标点。
bool isDelimiter(QChar c)
{
    return c == QChar(0x3002)      // 。
        || c == QChar(0xFF01)      // ！
        || c == QChar(0xFF1F)      // ？
        || c == QChar(0xFF1B)      // ；
        || c == QChar(0x2026)      // …
        || c == QLatin1Char('\n')  // 换行（\r 由调用处一并吞掉）
        || c == QLatin1Char('\r');
}

// 分隔符两侧都是数字时，它是数字书写的一部分（例如 “12。345”），
// 不应作为句子边界。
bool isInsideNumber(const QString &text, int index)
{
    return index > 0 && index + 1 < text.size()
        && text.at(index - 1).isDigit()
        && text.at(index + 1).isDigit();
}

} // namespace

QList<QString> splitSentences(const QString &text)
{
    // 第一遍：按分隔符切开，分隔符留在前一句末尾。
    QList<QString> raw;
    QString current;
    current.reserve(text.size());

    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (!isDelimiter(c) || isInsideNumber(text, i)) {
            current.append(c);
            continue;
        }

        current.append(c);

        // 吞掉紧随其后的连续分隔符（含 \r\n 这种成对形式），
        // 这样 “！！” 或 “\r\n” 只会产生一个边界。
        while (i + 1 < text.size()
               && isDelimiter(text.at(i + 1))
               && !isInsideNumber(text, i + 1)) {
            current.append(text.at(++i));
        }

        const QString trimmed = current.trimmed();
        if (!trimmed.isEmpty())
            raw.append(trimmed);
        current.clear();
    }

    // 末段没有分隔符收尾时同样要收进来。
    const QString tail = current.trimmed();
    if (!tail.isEmpty())
        raw.append(tail);

    // 第二遍：把过短的句子并入相邻句。
    QList<QString> merged;
    QString pending;
    for (const QString &sentence : raw) {
        pending += sentence;
        if (pending.size() >= kMinSentenceLength) {
            merged.append(pending);
            pending.clear();
        }
    }
    // 余下的短句没有后继可并，回接到上一句而不是丢弃。
    if (!pending.isEmpty()) {
        if (merged.isEmpty())
            merged.append(pending);
        else
            merged.last() += pending;
    }

    return merged;
}
