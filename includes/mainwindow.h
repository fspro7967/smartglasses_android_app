#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTreeWidget>
#include "devicehandler.h"
#include "whisper_manager.h"   
#include "msgsender.h"

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
    void onServiceTreeItemClicked(QTreeWidgetItem *item, int column);
    void onTranscriptionResult(const QString &text);
    void onAIResponse(const QString &response);
    void onWhisperError(const QString &error);
    void onSelectAudioFileClicked();
    void sendMessageToServer(const QString &message);

private:
    void requestAndroidPermissions();
    void buildServiceTree();
    QString extractModelToFile();
    QString m_modelPath;

    DeviceHandler *m_deviceHandler;
    MsgSender *m_msgsender;
    QListWidget  *m_deviceList;
    QPushButton  *m_scanButton;
    QPushButton  *m_selectAudioButton;
    QTextEdit    *m_dataDisplay;
    QTextEdit    *m_transcriptionDisplay;
    QTreeWidget  *m_serviceTree;

    QList<QBluetoothDeviceInfo> m_discoveredDevices;

    WhisperManager *m_whisperManager = nullptr;
    QByteArray m_audioBuffer;
};

#endif // MAINWINDOW_H