#include "assetextractor.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QTextStream>
#include <QStringConverter>

namespace {

// 分块大小：现有 whisper 提取用 readAll() 一次吞下 59.7 MB，这里改成 1 MiB 一块。
constexpr qint64 kChunkBytes = 1024 * 1024;

// 提取清单：所有目标文件都 rename 成功之后才写它。
constexpr char kManifestName[] = ".extract-manifest";

// 清单首行：格式标记 + 制表符 + 提取时的应用版本。
//
// 旧格式（v1）的清单没有这一行，第一行直接就是「文件名\t长度」。读到它时
// 首行对不上标记，isExtracted() 返回 false，于是重新提取并写出新格式——
// 只多提取这一次，不会被当成损坏而反复重试。
constexpr char kManifestHeader[] = "#extract-manifest-v2";

// 当前应用版本。Android 上 Qt 默认取 APK 的 versionName。
// 它覆盖「新旧模型长度恰好相同」的情况：升级必然换版本号，换了就重提。
QString currentAppVersion()
{
    return QCoreApplication::applicationVersion();
}

QString pathIn(const QString &dir, const QString &name)
{
    return dir + QLatin1Char('/') + name;
}

void setError(QString *out, const QString &msg)
{
    if (out)
        *out = msg;
}

bool writeManifest(const QString &targetDir, const QStringList &fileNames,
                   const QList<qint64> &sizes)
{
    QFile file(pathIn(targetDir, QLatin1String(kManifestName)));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << kManifestHeader << '\t' << currentAppVersion() << '\n';
    for (int i = 0; i < fileNames.size(); ++i)
        out << fileNames.at(i) << '\t' << sizes.at(i) << '\n';
    out.flush();

    const bool ok = (file.error() == QFileDevice::NoError);
    file.close();
    return ok;
}

} // namespace

namespace AssetExtractor {

bool isExtracted(const QString &assetDir, const QString &targetDir,
                 const QStringList &fileNames)
{
    if (fileNames.isEmpty())
        return false;

    QFile manifest(pathIn(targetDir, QLatin1String(kManifestName)));
    if (!manifest.open(QIODevice::ReadOnly))
        return false;

    QHash<QString, qint64> recorded;
    QTextStream in(&manifest);
    in.setEncoding(QStringConverter::Utf8);

    // 首行必须是「当前格式 + 当前应用版本」。旧格式清单、别的版本留下的清单
    // 都在这里被判为「不是这份 APK 的副本」。
    const QString expectedHeader = QLatin1String(kManifestHeader)
                                   + QLatin1Char('\t') + currentAppVersion();
    if (in.atEnd() || in.readLine() != expectedHeader)
        return false;

    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.isEmpty())
            continue;

        const int tab = line.indexOf(QLatin1Char('\t'));
        if (tab <= 0)
            return false;

        bool ok = false;
        const qint64 size = line.mid(tab + 1).toLongLong(&ok);
        if (!ok || size < 0)
            return false;

        recorded.insert(line.left(tab), size);
    }
    manifest.close();

    // 清单必须与期望的文件集合**完全一致**：名字对不上或数量对不上都算不完整，
    // 这样模型换了文件之后会自动重新提取。
    if (recorded.size() != fileNames.size())
        return false;

    for (const QString &name : fileNames) {
        const auto it = recorded.constFind(name);
        if (it == recorded.constEnd())
            return false;

        const QFileInfo info(pathIn(targetDir, name));
        if (!info.exists() || !info.isFile())
            return false;

        // 按字节长度校验，而不是「存在即有效」。
        if (info.size() != it.value())
            return false;

        // 再和 APK 里的源资产比：清单记的是「上次复制出来的长度」，只拿落盘文件
        // 去比是自洽的，永远相等。源资产换了（长度不同）必须能看出来。
        // assets:/ 上 QFileInfo::size() 可用；量不到（源不存在）同样算对不上。
        const QFileInfo source(pathIn(assetDir, name));
        if (!source.exists() || source.size() != it.value())
            return false;
    }

    return true;
}

bool extractFiles(const QString &assetDir, const QString &targetDir,
                  const QStringList &fileNames, QString *errorOut)
{
    if (fileNames.isEmpty()) {
        setError(errorOut, QStringLiteral("文件清单为空"));
        return false;
    }

    if (!QDir().mkpath(targetDir)) {
        setError(errorOut, QStringLiteral("无法创建目录: %1").arg(targetDir));
        return false;
    }

    QList<qint64> sizes;
    sizes.reserve(fileNames.size());

    for (const QString &name : fileNames) {
        const QString sourcePath = pathIn(assetDir, name);
        const QString targetPath = pathIn(targetDir, name);
        const QString tempPath   = targetPath + QLatin1String(".tmp");

        QFile source(sourcePath);
        if (!source.open(QIODevice::ReadOnly)) {
            setError(errorOut, QStringLiteral("无法读取 assets 文件: %1").arg(sourcePath));
            return false;
        }

        // 上一次进程被杀可能留下 .tmp，先清掉，避免它被误当成有效产物。
        QFile::remove(tempPath);

        QFile temp(tempPath);
        if (!temp.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            source.close();
            setError(errorOut, QStringLiteral("无法写入: %1").arg(tempPath));
            return false;
        }

        qint64 written = 0;
        bool copyOk = true;
        for (;;) {
            const QByteArray chunk = source.read(kChunkBytes);
            if (chunk.isEmpty())
                break;
            if (temp.write(chunk) != chunk.size()) {
                copyOk = false;
                break;
            }
            written += chunk.size();
        }
        if (source.error() != QFileDevice::NoError)
            copyOk = false;

        temp.close();
        source.close();

        if (!copyOk) {
            QFile::remove(tempPath);
            setError(errorOut, QStringLiteral("复制失败: %1").arg(sourcePath));
            return false;
        }

        // 只有完整写完的 .tmp 才有资格变成正式名字。
        QFile::remove(targetPath);
        if (!QFile::rename(tempPath, targetPath)) {
            QFile::remove(tempPath);
            setError(errorOut, QStringLiteral("重命名失败: %1").arg(targetPath));
            return false;
        }

        // rename 成功不等于字节数正确，落盘后再核一次。
        const qint64 onDisk = QFileInfo(targetPath).size();
        if (onDisk != written) {
            QFile::remove(targetPath);
            setError(errorOut, QStringLiteral("字节长度校验失败: %1（写入 %2，落盘 %3）")
                     .arg(targetPath).arg(written).arg(onDisk));
            return false;
        }

        sizes.append(written);
    }

    // 清单最后写：它是「本次提取完整」的唯一凭据。
    if (!writeManifest(targetDir, fileNames, sizes)) {
        setError(errorOut, QStringLiteral("无法写入提取清单: %1")
                 .arg(pathIn(targetDir, QLatin1String(kManifestName))));
        return false;
    }

    return true;
}

bool ensureExtracted(const QString &assetDir, const QString &targetDir,
                     const QStringList &fileNames, QString *errorOut)
{
    if (isExtracted(assetDir, targetDir, fileNames))
        return true;

    return extractFiles(assetDir, targetDir, fileNames, errorOut);
}

} // namespace AssetExtractor
