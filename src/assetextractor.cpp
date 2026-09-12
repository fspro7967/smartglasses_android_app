#include "assetextractor.h"

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
    for (int i = 0; i < fileNames.size(); ++i)
        out << fileNames.at(i) << '\t' << sizes.at(i) << '\n';
    out.flush();

    const bool ok = (file.error() == QFileDevice::NoError);
    file.close();
    return ok;
}

} // namespace

namespace AssetExtractor {

bool isExtracted(const QString &targetDir, const QStringList &fileNames)
{
    if (fileNames.isEmpty())
        return false;

    QFile manifest(pathIn(targetDir, QLatin1String(kManifestName)));
    if (!manifest.open(QIODevice::ReadOnly))
        return false;

    QHash<QString, qint64> recorded;
    QTextStream in(&manifest);
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
    if (isExtracted(targetDir, fileNames))
        return true;

    return extractFiles(assetDir, targetDir, fileNames, errorOut);
}

} // namespace AssetExtractor
