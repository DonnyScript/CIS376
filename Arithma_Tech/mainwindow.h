#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QDialog>
#include <QTableWidget>
#include <QRadioButton>
#include <QGroupBox>
#include <QTextEdit>
#include <QToolButton>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

class QDropEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QMimeData;

/**
 * @brief DropZone
 *        A QLabel subclass that accepts drag-and-drop of image files.
 */
class DropZone : public QLabel
{
    Q_OBJECT
public:
    explicit DropZone(QWidget *parent = nullptr);

signals:
    void fileDropped(const QString &filePath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

/**
 * @brief FileHistoryDialog
 *        Shows the DB-stored history of compressed/decompressed items.
 */
class FileHistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FileHistoryDialog(QWidget *parent = nullptr);

    // Refresh the table from database
    void refreshHistory();

private slots:
    // Delete currently selected row
    void deleteSelectedRow();

private:
    QTableWidget *historyTable;
    QPushButton  *deleteButton;
};

/**
 * @brief UserGuideDialog
 *        A pop-up with instructions about Arithmetic Encoding and usage.
 */
class UserGuideDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UserGuideDialog(QWidget *parent = nullptr);
};

/**
 * @brief MainWindow
 *        The main UI for the Arithmetic Encoding app:
 *        - Text or File mode
 *        - Compress/Decompress
 *        - Database logging
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Input mode (Text vs File)
    void switchToTextMode();
    void switchToFileMode();

    // File input actions
    void browseFile();
    void handleDroppedFile(const QString &filePath);
    void clearFileSelection();

    // Compression & Decompression
    void compressFile();
    void decompressFile();

    // Progress bar updates
    void updateProgressBar();
    void resetProgressBar();

    // Show pop-up dialogs
    void showFileHistory();
    void showUserGuide();

private:
    // Database initialization
    void initializeDatabase();

    // Insert a record into the DB's history
    void addHistoryEntry(const QString &fileName, const QString &operation);

    // Utility
    bool isImageFile(const QString &filePath);

    // Instances of dialogs
    FileHistoryDialog *fileHistoryDialog;
    UserGuideDialog   *userGuideDialog;

    // UI Elements
    QGroupBox    *inputModeGroup;
    QRadioButton *textModeRadio;
    QRadioButton *fileModeRadio;

    QWidget      *textInputWidget;
    QTextEdit    *textInput;

    QWidget      *fileInputWidget;
    DropZone     *dropZone;
    QLabel       *selectedFileLabel;
    QToolButton  *cancelFileButton;
    QPushButton  *browseButton;

    QWidget      *actionWidget;
    QPushButton  *compressButton;
    QPushButton  *decompressButton;
    QProgressBar *progressBar;
    QLabel       *statusLabel;

    QLabel       *subtitleLabel;
    QLabel       *descriptionLabel;

    QTimer       *progressTimer;
    QString       currentFilePath;
    bool          operationInProgress;
};

#endif // MAINWINDOW_H
