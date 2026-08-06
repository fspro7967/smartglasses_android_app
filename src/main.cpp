#include "mainwindow.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QLocale>
#include <QTranslator>
#include <QQmlContext>


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "smartglasses_android_app_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }

    QQmlApplicationEngine engine;

    // 将业务逻辑桥接类暴露给 QML（在 qml/Main.qml 中以 backend 引用）
    MainWindow mainWindow;
    engine.rootContext()->setContextProperty("backend", &mainWindow);

    engine.loadFromModule("SmartGlasses", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
