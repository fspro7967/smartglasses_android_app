// splitSentences() 的单元测试。
//
// 这是本仓库里第一个不需要真实设备就能运行的测试：splitSentences 是纯自由
// 函数，不依赖 QObject、信号槽、蓝牙或 Android，因此可以在构建机上以桌面
// 目标直接跑（见 tests/CMakeLists.txt —— 该目标只在非 Android 构建中生成）。
//
// 运行方式（构建机上）：
//   cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug && cmake --build build-tests
//   ./build-tests/tests/tst_sentencesplitter
//
// 注意：多数用例的句子都刻意写得长于 kMinSentenceLength。短于阈值的句子会
// 按设计并入相邻句，若用例句子过短，期望值会被合并规则改写。

#include <QtTest>
#include <QRegularExpression>

#include "sentencesplitter.h"

class TestSentenceSplitter : public QObject
{
    Q_OBJECT

private slots:
    // ---------- 退化输入：不产生任何句子 ----------

    void emptyInputProducesNothing()
    {
        QCOMPARE(splitSentences(QString()), QList<QString>());
    }

    void whitespaceOnlyProducesNothing()
    {
        QCOMPARE(splitSentences(QStringLiteral("   \n\t  ")), QList<QString>());
    }

    // ---------- 无分隔符 ----------

    void textWithoutDelimiterStaysWhole()
    {
        const QString text = QStringLiteral("这是一个没有标点的长句子");
        QCOMPARE(splitSentences(text), QList<QString>({text}));
    }

    // ---------- 五种分隔符各自都能切开 ----------

    void fullStopSplits()
    {
        QCOMPARE(splitSentences(QStringLiteral("今天天气很好。我们出去走走。")),
                 QList<QString>({QStringLiteral("今天天气很好。"),
                                 QStringLiteral("我们出去走走。")}));
    }

    void exclamationSplits()
    {
        QCOMPARE(splitSentences(QStringLiteral("太棒了真是太好了！我们再试一次吧！")),
                 QList<QString>({QStringLiteral("太棒了真是太好了！"),
                                 QStringLiteral("我们再试一次吧！")}));
    }

    void questionMarkSplits()
    {
        QCOMPARE(splitSentences(QStringLiteral("你吃饭了吗？我还没吃呢。")),
                 QList<QString>({QStringLiteral("你吃饭了吗？"),
                                 QStringLiteral("我还没吃呢。")}));
    }

    void semicolonSplits()
    {
        QCOMPARE(splitSentences(QStringLiteral("第一点很重要；第二点同样重要。")),
                 QList<QString>({QStringLiteral("第一点很重要；"),
                                 QStringLiteral("第二点同样重要。")}));
    }

    void ellipsisSplits()
    {
        QCOMPARE(splitSentences(QStringLiteral("让我仔细想想…大概是这个意思。")),
                 QList<QString>({QStringLiteral("让我仔细想想…"),
                                 QStringLiteral("大概是这个意思。")}));
    }

    void newlineSplits()
    {
        QCOMPARE(splitSentences(QStringLiteral("第一行的内容比较长\n第二行的内容也比较长")),
                 QList<QString>({QStringLiteral("第一行的内容比较长"),
                                 QStringLiteral("第二行的内容也比较长")}));
    }

    // 分隔符保留在前一句末尾，朗读时停顿更自然。
    void delimiterIsKeptOnThePrecedingSentence()
    {
        const QList<QString> got = splitSentences(QStringLiteral("前半句在这里。后半句在这里。"));
        QCOMPARE(got.size(), 2);
        QVERIFY(got.at(0).endsWith(QChar(0x3002)));   // 。
        QVERIFY(got.at(1).endsWith(QChar(0x3002)));   // 。
    }

    // ---------- 连续分隔符只算一个边界 ----------

    void consecutiveDelimitersDoNotProduceEmptySentences()
    {
        QCOMPARE(splitSentences(QStringLiteral("这真是太好了！！！我们继续吧。")),
                 QList<QString>({QStringLiteral("这真是太好了！！！"),
                                 QStringLiteral("我们继续吧。")}));
    }

    void mixedConsecutiveDelimitersCollapse()
    {
        QCOMPARE(splitSentences(QStringLiteral("这是真的吗？！那就这样定了。")),
                 QList<QString>({QStringLiteral("这是真的吗？！"),
                                 QStringLiteral("那就这样定了。")}));
    }

    // \r\n 只能产生一个边界，不能切开两次。
    void crlfCountsAsOneBoundary()
    {
        QCOMPARE(splitSentences(QStringLiteral("第一行的内容比较长\r\n第二行的内容也比较长")),
                 QList<QString>({QStringLiteral("第一行的内容比较长"),
                                 QStringLiteral("第二行的内容也比较长")}));
    }

    // ---------- 不以分隔符结尾，不会产生空句 ----------

    void trailingDelimiterDoesNotProduceEmptySentence()
    {
        QCOMPARE(splitSentences(QStringLiteral("这句话到此为止。")),
                 QList<QString>({QStringLiteral("这句话到此为止。")}));
    }

    void textAfterLastDelimiterIsFlushed()
    {
        QCOMPARE(splitSentences(QStringLiteral("前面这句比较长。后面这句没有标点结尾")),
                 QList<QString>({QStringLiteral("前面这句比较长。"),
                                 QStringLiteral("后面这句没有标点结尾")}));
    }

    // ---------- 最小长度阈值：短句与后一句合并 ----------

    void shortSentenceMergesIntoTheFollowingOne()
    {
        // “好。” 只有 2 字符，低于 kMinSentenceLength，须并入下一句。
        const QList<QString> got = splitSentences(QStringLiteral("好。我不知道这件事。"));
        QCOMPARE(got.size(), 1);
        QCOMPARE(got.at(0), QStringLiteral("好。我不知道这件事。"));
    }

    void shortTrailingSentenceMergesBackIntoThePreviousOne()
    {
        // 末句没有后继可并，必须并入前一句而不是丢弃。
        const QList<QString> got = splitSentences(QStringLiteral("这是一句足够长的话。好的。"));
        QCOMPARE(got.size(), 1);
        QCOMPARE(got.at(0), QStringLiteral("这是一句足够长的话。好的。"));
    }

    void mergeNeverLosesText()
    {
        const QString text = QStringLiteral("好。我不知道这件事。嗯。那就先这样安排吧。");
        const QList<QString> got = splitSentences(text);
        QVERIFY(!got.isEmpty());
        for (const QString &s : got)
            QVERIFY(!s.trimmed().isEmpty());

        // 合并不得丢字：去掉空白后总长度应与原文一致。
        const QRegularExpression whitespace(QStringLiteral("\\s"));
        QString joined;
        for (const QString &s : got)
            joined += s;
        QCOMPARE(joined.remove(whitespace), QString(text).remove(whitespace));
    }

    void sentencesAtThresholdAreNotMerged()
    {
        // 恰好等于阈值时必须独立成句——这是阈值语义的边界。
        const QString exact(kMinSentenceLength, QChar(0x4E00));   // “一” × 阈值
        const QList<QString> got = splitSentences(exact + QStringLiteral("。"));
        QCOMPARE(got.size(), 1);
        QCOMPARE(got.at(0).size(), kMinSentenceLength + 1);       // 含句号
    }

    // ---------- 不在数字中间切 ----------

    void doesNotSplitBetweenDigits()
    {
        // “12。345” 里句号两侧都是数字，必须当作数字的一部分而不切开。
        const QList<QString> got = splitSentences(QStringLiteral("编号12。345号"));
        QCOMPARE(got.size(), 1);
        QVERIFY(got.at(0).contains(QStringLiteral("12。345")));
    }

    void decimalPointIsNotADelimiter()
    {
        // 半角句点根本不在分隔符集合里，小数天然安全。
        const QList<QString> got = splitSentences(QStringLiteral("圆周率约等于3.14这个值。"));
        QCOMPARE(got.size(), 1);
        QVERIFY(got.at(0).contains(QStringLiteral("3.14")));
    }

    void splitAfterDigitsIsAllowed()
    {
        // 只有一侧是数字时不构成“数字中间”，仍应正常切开。
        QCOMPARE(splitSentences(QStringLiteral("这个数字是12345。后面还有一句话。")),
                 QList<QString>({QStringLiteral("这个数字是12345。"),
                                 QStringLiteral("后面还有一句话。")}));
    }

    // ---------- 空白处理 ----------

    void surroundingWhitespaceIsTrimmed()
    {
        const QList<QString> got = splitSentences(QStringLiteral("  前面这句比较长。  后面这句也比较长。  "));
        QCOMPARE(got.size(), 2);
        QCOMPARE(got.at(0), QStringLiteral("前面这句比较长。"));
        QCOMPARE(got.at(1), QStringLiteral("后面这句也比较长。"));
    }

    // ---------- 真实回复形态的整句切分 ----------

    void realisticReply()
    {
        const QList<QString> got = splitSentences(
            QStringLiteral("根据刚才的问题，我给出三点建议。第一，先确认设备已配对；"
                           "第二，检查网络是否可用；第三，重启应用再试一次。"));
        QCOMPARE(got.size(), 4);
        QVERIFY(got.at(0).startsWith(QStringLiteral("根据刚才的问题")));
        QVERIFY(got.at(1).startsWith(QStringLiteral("第一")));
        QVERIFY(got.at(3).endsWith(QChar(0x3002)));
    }
};

QTEST_APPLESS_MAIN(TestSentenceSplitter)

#include "tst_sentencesplitter.moc"
