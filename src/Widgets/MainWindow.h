
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QColor>

class QWidget;
class QLabel;
class QFormLayout;
class QHBoxLayout;
class QVBoxLayout;
class QGroupBox;
class QLineEdit;
class QPushButton;
class QProgressBar;
class QTextEdit;
class QRadioButton;
class QButtonGroup;
class QComboBox;

class FileSelectionWidget;
class FileDropLineEdit;

class MainWindow : public QMainWindow

{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QWidget *centralWidget;
    QVBoxLayout *vLayout, *vlModel, *vlEngine;
    QLabel *appHeader;
    QGroupBox *grpInput, *grpModel, *grpEngine;
    QFormLayout *formLayoutInput, *formLayoutEngine;
    QHBoxLayout *hBoxModel, *hBoxModelList;
    QHBoxLayout *hBoxModelAndEngine;
    QLineEdit *txtTempo;
    FileSelectionWidget *fswAudio, *fswModel, *fswMIDI;
    QButtonGroup *radioSelectGroup;
    QRadioButton *radioSelectFromList, *radioSelectFromPath;
    QComboBox *cmbModel;
    QComboBox *cmbEP;
    QLineEdit *txtDeviceIndex;
    QPushButton *btnStart;
    QProgressBar *progressBar;
    QTextEdit *loggingArea;

    void initUI();

private:
    bool isModelFromPath;
    QColor m_originalTextColor;

public Q_SLOTS:
    void onStartButtonClicked();
    void onFinished();
    void logMsgInfo(const QString &msg);
    void logMsgError(const QString &msg);
    void logMsgWithColor(const QString &msg, const QColor &color);

private:
    void browseOpenFile(QLineEdit *widget, const QString &filter = QString());
    void browseSaveFile(QLineEdit *widget, const QString &filter = QString());
    void setModelSelectMode(bool modelFromPath);
    void loadModelList();

protected:
    void showEvent(QShowEvent *event) override;
};

#endif // MAINWINDOW_H
