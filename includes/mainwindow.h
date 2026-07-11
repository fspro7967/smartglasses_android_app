#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTreeWidget>
#include <QTabWidget>
#include <QLabel>
#include "devicehandler.h"

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

    // 服务和特征树点击
    void onServiceTreeItemClicked(QTreeWidgetItem *item, int column);

private:
    void requestAndroidPermissions();
    void buildServiceTree();

    void onStartRecording();
    void onStopRecording();
    QString saveAudioToFile(const QByteArray &audioData);

    DeviceHandler *m_deviceHandler;
    QListWidget  *m_deviceList;
    QPushButton  *m_scanButton;
    QTextEdit    *m_dataDisplay;
    QTreeWidget  *m_serviceTree;
    // 选项卡
    QTabWidget *m_mainTabWidget;       // 主选项卡（连接 | 数据）
    QTabWidget *m_connectTab;          // 连接选项卡（扫描 | 服务）
    QTabWidget *m_dataTab;             // 数据选项卡（实时数据 | 录音）

    // 录音相关
    QPushButton *m_recStartBtn;
    QPushButton *m_recStopBtn;
    QLabel *m_recStatusLabel;
    QByteArray m_recBuffer;            // 录音缓冲区
    bool m_isRecording = false;

    QList<QBluetoothDeviceInfo> m_discoveredDevices;
};

#endif // MAINWINDOW_H