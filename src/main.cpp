#include <QApplication>
#include <QString>
#ifdef ORT_API_MANUAL_INIT
#include <QMessageBox>
#include "OrtLoader.h"
#endif

#include "Widgets/MainWindow.h"


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setFont({"Microsoft YaHei UI", 9});
    a.setStyle("fusion");
#ifdef ORT_API_MANUAL_INIT
    QString errorString;
    bool ok = InitOrtLibrary(&errorString);
    if (!ok) {
        QMessageBox::critical(nullptr, "Error", "Could not load ONNX Runtime library:\n" + errorString);
        return -1;
    }
#endif
    MainWindow w;
    w.show();
    return a.exec();
}
