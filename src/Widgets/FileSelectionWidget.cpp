#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include "FileSelectionWidget.h"

FileSelectionWidget::FileSelectionWidget(bool isSaveDialog, const QString &filter, QWidget *parent)
        : QWidget(parent), m_isSaveDialog(isSaveDialog), m_filter(filter),
          m_layout(new QHBoxLayout(this)),
          m_lineEdit(new QLineEdit(this)),
          m_browseButton(new QPushButton("Browse...", this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(m_lineEdit);
    m_layout->addWidget(m_browseButton);
    setLayout(m_layout);

    connect(m_browseButton, &QPushButton::clicked, [this]() {
        QString filePath = m_isSaveDialog ?
                QFileDialog::getSaveFileName(this, QString(), QString(), m_filter):
                QFileDialog::getOpenFileName(this, QString(), QString(), m_filter);
        if (!filePath.isEmpty()) {
            m_lineEdit->setText(filePath);
        }
    });

    setAcceptDrops(true);

    setTabOrder(m_lineEdit, m_browseButton);
}

QString FileSelectionWidget::filePath() const {
    return m_lineEdit->text();
}

void FileSelectionWidget::dragEnterEvent(QDragEnterEvent *event) {
    const auto mimeData = event->mimeData();
    if (mimeData && mimeData->hasUrls()) {
        event->acceptProposedAction();
    }
}

void FileSelectionWidget::dropEvent(QDropEvent *event) {
    const auto mimeData = event->mimeData();
    if (mimeData && mimeData->hasUrls()) {
        const auto &urls = mimeData->urls();
        if (!urls.isEmpty()) {
            const auto filePath = urls.first().toLocalFile();
            if (!filePath.isEmpty()) {
                m_lineEdit->setText(filePath);
            }
        }
    }
}

bool FileSelectionWidget::isSaveDialog() const {
    return m_isSaveDialog;
}

void FileSelectionWidget::setSaveDialog(bool isSaveDialog) {
    m_isSaveDialog = isSaveDialog;
}

void FileSelectionWidget::setOpenDialog() {
    m_isSaveDialog = false;
}

void FileSelectionWidget::setFilter(const QString &filter) {
    m_filter = filter;
}

QString FileSelectionWidget::filter() const {
    return m_filter;
}

QLineEdit *FileSelectionWidget::getLineEdit() const {
    return m_lineEdit;
}

QPushButton *FileSelectionWidget::getButton() const {
    return m_browseButton;
}
