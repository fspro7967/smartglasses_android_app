#ifndef ASSETEXTRACTOR_H
#define ASSETEXTRACTOR_H

#include <QString>
#include <QStringList>

// 把 APK 内 `assets:/` 下的文件复制到应用私有目录。
//
// 为什么需要它（而不是继续用 mainwindow.cpp 里那个 extractModelToFile）：
//   * 那个函数用 readAll() 一次性读入 59.7 MB，在 Android 上有被低内存杀手
//     干掉的风险
//   * 它把「文件存在」当作「文件有效」，进程被杀留下的半截文件会被当成好文件
//   * 它只处理单个文件，而 TTS 模型是一个目录
//
// 本模块只做复制，**不做线程调度**——调用方必须自己放到后台线程执行
// （MainWindow 用 QtConcurrent::run），构造函数里绝不能直接调用。
//
// 四项保证，顺序不可交换：
//   1. 分块复制（约 1 MiB 一块），任意时刻内存里只有一个分块
//   2. 先写 <name>.tmp，全部写完才 rename 成正式名字——半截文件永远不会
//      顶着正式名字出现
//   3. rename 之后再核一遍落盘字节数
//   4. 「是否已提取」靠**清单 + 逐文件字节长度**判定，不是「文件存在即有效」；
//      清单在所有文件都 rename 成功之后才写，所以它本身就是「这次提取完整」
//      的唯一凭据
//
// 注意：Qt 的 `assets:/` 引擎只能随机读、**不能列目录**（QDir 迭代在 assets:/
// 上不工作），所以文件清单必须由调用方显式给出。
namespace AssetExtractor {

// 从 assetDir（形如 "assets:/models/tts-xxx"）复制 fileNames 列出的文件到
// targetDir。任一文件失败则整体失败，且不会写清单。
bool extractFiles(const QString &assetDir, const QString &targetDir,
                  const QStringList &fileNames, QString *errorOut = nullptr);

// 逐文件按字节长度校验 targetDir 是否已是 assetDir 的完整副本。
bool isExtracted(const QString &targetDir, const QStringList &fileNames);

// 已完整则立刻返回 true（不拷贝任何字节）；否则完整重新提取。
bool ensureExtracted(const QString &assetDir, const QString &targetDir,
                     const QStringList &fileNames, QString *errorOut = nullptr);

} // namespace AssetExtractor

#endif // ASSETEXTRACTOR_H
