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
//   5. 「已提取」的含义是**落盘副本确实来自当前这份 APK**，而不只是「落盘文件
//      没被动过」：清单记下提取时的应用版本，校验时还要重新量一遍源资产的长度。
//      换了版本或换了模型文件都会重新提取——否则装上带新模型的 APK 之后，
//      旧副本会一直被当成有效的继续用，而且没有任何提示。
//
// 注意：Qt 的 `assets:/` 引擎只能随机读、**不能列目录**（QDir 迭代在 assets:/
// 上不工作），所以文件清单必须由调用方显式给出。
namespace AssetExtractor {

// 从 assetDir（形如 "assets:/models/tts-xxx"）复制 fileNames 列出的文件到
// targetDir。任一文件失败则整体失败，且不会写清单。
bool extractFiles(const QString &assetDir, const QString &targetDir,
                  const QStringList &fileNames, QString *errorOut = nullptr);

// targetDir 是否已是 assetDir（当前这份 APK 里的资产）的完整副本：
// 清单格式与应用版本一致、文件集合一致，且每个文件的落盘长度、清单记录、
// 源资产长度三者相等。任何一项对不上都返回 false（随后会整体重新提取）。
bool isExtracted(const QString &assetDir, const QString &targetDir,
                 const QStringList &fileNames);

// 已完整则立刻返回 true（不拷贝任何字节）；否则完整重新提取。
bool ensureExtracted(const QString &assetDir, const QString &targetDir,
                     const QStringList &fileNames, QString *errorOut = nullptr);

} // namespace AssetExtractor

#endif // ASSETEXTRACTOR_H
