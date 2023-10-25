#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QString>
#include <QValidator>
#include <QProgressBar>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QScreen>
#include <QDesktopWidget>
#include <QApplication>
#include <QDateTime>
#include <QRadioButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QDir>
#include <QAbstractItemView>

#include "FileSelectionWidget.h"
#include "MainWindow.h"
#include "Worker.h"


inline void addSpacerToAlignWithRadioButton(QHBoxLayout *layout, QRadioButton *radioButton);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      centralWidget(new QWidget(this)),
      vLayout(new QVBoxLayout(centralWidget)),
      vlModel(new QVBoxLayout(centralWidget)),
      vlEngine(new QVBoxLayout(centralWidget)),
      appHeader(new QLabel(centralWidget)),
      grpInput(new QGroupBox(centralWidget)),
      grpModel(new QGroupBox(centralWidget)),
      grpEngine(new QGroupBox(centralWidget)),
      formLayoutInput(new QFormLayout(centralWidget)),
      hBoxModel(new QHBoxLayout(centralWidget)),
      hBoxModelList(new QHBoxLayout(centralWidget)),
      fswModel(new FileSelectionWidget(false, QString("*.onnx"), centralWidget)),
      fswAudio(new FileSelectionWidget(false, QString("*.wav"), centralWidget)),
      fswMIDI(new FileSelectionWidget(true, QString("*.mid"), centralWidget)),
      txtTempo(new QLineEdit("120", centralWidget)),
      radioSelectGroup(new QButtonGroup(centralWidget)),
      radioSelectFromList(new QRadioButton("Select model from list", centralWidget)),
      radioSelectFromPath(new QRadioButton("Select model from file path", centralWidget)),
      cmbModel(new QComboBox(centralWidget)),
      formLayoutEngine(new QFormLayout(centralWidget)),
      cmbEP(new QComboBox(centralWidget)),
      txtDeviceIndex(new QLineEdit(centralWidget)),
      hBoxModelAndEngine(new QHBoxLayout(centralWidget)),
      btnStart(new QPushButton("Start", centralWidget)),
      progressBar(new QProgressBar(centralWidget)),
      loggingArea(new QTextEdit(centralWidget)),
      isModelFromPath(false)
{
    initUI();

    connect(btnStart, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(radioSelectFromList, &QAbstractButton::clicked, [this](bool checked) {
        setModelSelectMode(!checked);
    });
    connect(radioSelectFromPath, &QAbstractButton::clicked, [this](bool checked) {
        setModelSelectMode(checked);
    });
    loadModelList();

    connect(loggingArea->document(), &QTextDocument::contentsChanged, [this]() {
        QTextCursor cursor(loggingArea->document());
        cursor.movePosition(QTextCursor::End);
        loggingArea->setTextCursor(cursor);
        loggingArea->ensureCursorVisible();
    });
}

MainWindow::~MainWindow() = default;


void MainWindow::initUI() {
    setCentralWidget(centralWidget);

    centralWidget->setLayout(vLayout);

    appHeader->setText("SOME: Singing-Oriented MIDI Extractor");
    {
        auto font = appHeader->font();
        font.setPointSize(12);
        appHeader->setFont(font);
        auto margins = appHeader->contentsMargins();
        margins.setTop(10);
        margins.setBottom(10);
        appHeader->setContentsMargins(margins);
    }

    appHeader->setAlignment(Qt::AlignCenter);
    vLayout->addWidget(appHeader);
    // BEGIN: GroupBox Input

    formLayoutInput->addRow("Input Audio File", fswAudio);

    auto validator = new QDoubleValidator(txtTempo);
    validator->setBottom(0);
    txtTempo->setValidator(validator);

    formLayoutInput->addRow("Tempo", txtTempo);

    grpInput->setLayout(formLayoutInput);
    grpInput->setTitle("Input/Output");

    vLayout->addWidget(grpInput);

    formLayoutInput->addRow("Output MIDI File", fswMIDI);

    setTabOrder(fswAudio->getButton(), txtTempo);
    setTabOrder(txtTempo, fswMIDI->getLineEdit());

    // END: GroupBox Input

    // BEGIN: GroupBox Model

    grpModel->setTitle("Model");
    radioSelectGroup->addButton(radioSelectFromList);
    radioSelectGroup->addButton(radioSelectFromPath);
    radioSelectGroup->setExclusive(true);
    radioSelectFromList->setChecked(true);
    setModelSelectMode(false);
    vlModel->addWidget(radioSelectFromList);
    addSpacerToAlignWithRadioButton(hBoxModelList, radioSelectFromList);
    cmbModel->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    {
        auto view = cmbModel->view();
        if (view) {
            view->setTextElideMode(Qt::ElideMiddle);
        }
    }
    hBoxModelList->addWidget(cmbModel);
    vlModel->addLayout(hBoxModelList);
    vlModel->addWidget(radioSelectFromPath);
    addSpacerToAlignWithRadioButton(hBoxModel, radioSelectFromPath);
    //hBoxModel->addWidget(txtModel);
    //hBoxModel->addWidget(btnBrowseModel);
    hBoxModel->addWidget(fswModel);

    vlModel->addLayout(hBoxModel);
    grpModel->setLayout(vlModel);
    hBoxModelAndEngine->addWidget(grpModel);
    // END: GroupBox Model

    // BEGIN: GroupBox Engine
    cmbEP->addItem("CPU", QVariant::fromValue(some::ExecutionProvider::CPU));
#ifdef ONNXRUNTIME_ENABLE_DML
    cmbEP->addItem("DirectML", QVariant::fromValue(some::ExecutionProvider::DirectML));
#endif
#ifdef ONNXRUNTIME_ENABLE_CUDA
    cmbEP->addItem("CUDA", QVariant::fromValue(some::ExecutionProvider::CUDA));
#endif
    auto validatorInt = new QIntValidator(txtDeviceIndex);
    validatorInt->setBottom(0);
    txtDeviceIndex->setText("0");
    txtDeviceIndex->setValidator(validatorInt);
    formLayoutEngine->addRow("Execution Provider", cmbEP);
    formLayoutEngine->addRow("GPU Device Index", txtDeviceIndex);
    vlEngine->addLayout(formLayoutEngine);
    grpEngine->setTitle("Engine");
    grpEngine->setLayout(vlEngine);
    hBoxModelAndEngine->addWidget(grpEngine);
    // END: GroupBox Engine
    hBoxModelAndEngine->setStretch(0, 1);
    hBoxModelAndEngine->setStretch(1, 0);
    vLayout->addLayout(hBoxModelAndEngine);

    vLayout->addWidget(btnStart);
    loggingArea->setReadOnly(true);
    loggingArea->ensureCursorVisible();
    vLayout->addWidget(loggingArea);
    vLayout->addWidget(progressBar);

    m_originalTextColor = loggingArea->textColor();
}

void addSpacerToAlignWithRadioButton(QHBoxLayout *layout, QRadioButton *radioButton) {
    if (!layout || !radioButton) {
        return;
    }
    auto style = radioButton->style();
    if (style) {
        auto w = style->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth);
        w += style->pixelMetric(QStyle::PM_RadioButtonLabelSpacing);
        auto spacer = new QSpacerItem(w, 0);
        layout->addSpacerItem(spacer);
    }
}

void MainWindow::onStartButtonClicked() {
    auto ep = cmbEP->currentData().value<some::ExecutionProvider>();
    auto deviceIndex = txtDeviceIndex->text().toInt();

    QString modelPath = isModelFromPath ? fswModel->filePath() :
            (cmbModel->count() ? cmbModel->currentData().toString() : QString());

    {
        bool ok = true;
        QString errMsg;
        if (fswAudio->filePath().isEmpty()) {
            ok = false;
            errMsg += "[Input Audio File]";
        }
        if (fswMIDI->filePath().isEmpty()) {
            if (!ok) {
                errMsg += ", ";
            }
            ok = false;
            errMsg += "[Output MIDI File]";
        }
        if (modelPath.isEmpty()) {
            if (!ok) {
                errMsg += ", ";
            }
            ok = false;
            errMsg += "[Model Path]";
        }
        if (!ok) {
            errMsg += " must not be empty!";
            QMessageBox::critical(this, "Error", errMsg);
            return;
        }
    }

    auto worker = new Worker(
                   modelPath,
                   fswAudio->filePath(),
                   txtTempo->text().toDouble(),
                   fswMIDI->filePath(),
                   ep,
                   deviceIndex,
                   1,
                   this);
    connect(worker, &Worker::logMsgInfo, this, &MainWindow::logMsgInfo);
    connect(worker, &Worker::logMsgError, this, &MainWindow::logMsgError);
    connect(worker, &Worker::logMsgWithColor, this, &MainWindow::logMsgWithColor);
    connect(worker, &QThread::finished, this, &MainWindow::onFinished);
    connect(worker, &QThread::finished, worker, &QThread::deleteLater);
    btnStart->setEnabled(false);
    progressBar->setRange(0, 0);
    worker->start();
}

void MainWindow::browseOpenFile(QLineEdit *widget, const QString &filter) {
    auto filename = QFileDialog::getOpenFileName(this, QString(), QString(), filter);
    if (!filename.isEmpty()) {
        if (widget) {
            widget->setText(filename);
        }
    }
}

void MainWindow::browseSaveFile(QLineEdit *widget, const QString &filter) {
    auto filename = QFileDialog::getSaveFileName(this, QString(), QString(), filter);
    if (!filename.isEmpty()) {
        if (widget) {
            widget->setText(filename);
        }
    }
}

void MainWindow::showEvent(QShowEvent *event) {
    QSize windowSize(640, 480);
#ifdef Q_OS_MAC
    auto dpiScale = 1.0;
    // It seems that on macOS, logicalDotsPerInch() always return 72.
    resize(windowSize);
#else
    // Get current screen
    auto currentScreen = screen();
    if (currentScreen) {
        // Try calculate screen DPI scale factor. If currentScreen is nullptr, use 1.0.
        auto dpiScale = currentScreen->logicalDotsPerInch() / 96.0;
        // Get current window size
        auto oldSize = windowSize;
        // Calculate DPI scaled window size
        auto newSize = dpiScale * oldSize;
        // Resize window to DPI scaled size
        resize(newSize);
        // Move window to screen center
        auto center = currentScreen->geometry().center();
        move(center.x() - size().width() / 2, center.y() - size().height() / 2);
    }
#endif
}

void MainWindow::logMsgInfo(const QString &msg) {
    logMsgWithColor(msg, m_originalTextColor);
}

void MainWindow::logMsgError(const QString &msg) {
    logMsgWithColor(msg, Qt::red);
}

void MainWindow::logMsgWithColor(const QString &msg, const QColor &color) {
    if (!msg.isEmpty()) {
        // Insert timestamp
        loggingArea->moveCursor(QTextCursor::End);
        loggingArea->setTextColor(Qt::gray);
        auto timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        loggingArea->insertPlainText(QString('[') + timestamp + QString(']'));

        // Insert delimiter between timestamp and message (here it is space)
        loggingArea->moveCursor(QTextCursor::End);
        loggingArea->setTextColor(m_originalTextColor);
        loggingArea->insertPlainText(" ");

        // Insert message
        loggingArea->moveCursor(QTextCursor::End);
        loggingArea->setTextColor(color);
        loggingArea->insertPlainText(msg);

        // Insert newline
        loggingArea->moveCursor(QTextCursor::End);
        loggingArea->setTextColor(m_originalTextColor);
        loggingArea->insertPlainText("\n");
    }
}

void MainWindow::onFinished() {
    btnStart->setEnabled(true);
    progressBar->setRange(0, 100);
}

void MainWindow::setModelSelectMode(bool modelFromPath) {
    isModelFromPath = modelFromPath;
    cmbModel->setEnabled(!isModelFromPath);
    //cmbModel->setVisible(!isModelFromPath);
    fswModel->setEnabled(isModelFromPath);
    //fswModel->setVisible(isModelFromPath);
}

void MainWindow::loadModelList() {
    auto appPath = qApp->applicationDirPath();
    auto modelsPath = appPath + '/' + "models";
    auto modelsDir = QDir(modelsPath);
    modelsDir.setNameFilters({"*.onnx"});
    auto onnxFiles = modelsDir.entryList(QDir::Files);
    for (const auto &onnxFile : onnxFiles) {
        auto fullPath = QDir::cleanPath(modelsPath + '/' + onnxFile);
        cmbModel->addItem(onnxFile, QVariant::fromValue(fullPath));
    }
}
