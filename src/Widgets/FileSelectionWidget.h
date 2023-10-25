#ifndef SOME_GUI_FILESELECTIONWIDGET_H
#define SOME_GUI_FILESELECTIONWIDGET_H

#include <QObject>
#include <QWidget>
#include <QString>

class QHBoxLayout;
class QLineEdit;
class QPushButton;
class QDragEnterEvent;
class QDropEvent;

class FileSelectionWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool isSaveDialog READ isSaveDialog WRITE setSaveDialog)
    Q_PROPERTY(QString filter READ filter WRITE setFilter)
    Q_PROPERTY(QString filePath READ filePath)
public:
    explicit FileSelectionWidget(bool isSaveDialog = false, const QString& filter = QString(), QWidget* parent = nullptr);
    QString filePath() const;
    bool isSaveDialog() const;
    void setSaveDialog(bool isSaveDialog = true);
    void setOpenDialog();
    void setFilter(const QString &filter);
    QString filter() const;
    QLineEdit *getLineEdit() const;
    QPushButton *getButton() const;
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
private:
    QHBoxLayout *m_layout;
    QLineEdit* m_lineEdit;
    QPushButton* m_browseButton;

    QString m_filter;
    bool m_isSaveDialog;
};


#endif //SOME_GUI_FILESELECTIONWIDGET_H
