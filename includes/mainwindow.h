#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTreeWidget>
#include "devicehandler.h"
#include "whisper_manager.h"   // ← 新增

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onScanClicked();
    void onDeviceSelected(QListWidgetItem *item);
    void onDataReceived(const QByteArray &data);
    void appendStatus(const QString &msg);
    void onBluetoothPermissionGranted();
    //void onStoragePermissionGranted();
    void onServiceTreeItemClicked(QTreeWidgetItem *item, int column);

    // ← 新增两个槽
    void onTranscriptionResult(const QString &text);
    void onWhisperError(const QString &error);

private:
    void requestAndroidPermissions();
    void buildServiceTree();

    DeviceHandler *m_deviceHandler;
    QListWidget  *m_deviceList;
    QPushButton  *m_scanButton;
    QTextEdit    *m_dataDisplay;
    QTreeWidget  *m_serviceTree;

    QList<QBluetoothDeviceInfo> m_discoveredDevices;

    // ← 新增成员
    WhisperManager *m_whisperManager = nullptr;
    QByteArray m_audioBuffer;
};

#endif // MAINWINDOW_H