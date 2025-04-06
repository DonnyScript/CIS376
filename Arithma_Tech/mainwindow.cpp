#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QFont>
#include <QMenuBar>
#include <QAction>
#include <QHeaderView>
#include <QDateTime>
#include <QTextBrowser>
#include <QDebug>
#include <QRadioButton>
#include <QGroupBox>
#include <QToolButton>
#include <QProgressBar>
#include <QLabel>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QMetaType>

// 1) CIS476Project Implementation
DropZone::DropZone(QWidget *parent)
    : QLabel(parent)
{
    setAlignment(Qt::AlignCenter);
    setText("Drag & Drop Image Here");
    setAcceptDrops(true);
    setMinimumHeight(120);

    setStyleSheet(
        "QLabel {"
        "  border: 2px dashed #6c7eb7;"
        "  border-radius: 8px;"
        "  padding: 25px;"
        "  background-color: #c0c0c0;" //  OLD color: f0f4ff
        "  color: #445277;"
        "  font-size: 14px;"
        "}"
        );
}

void DropZone::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        setStyleSheet(
            "QLabel {"
            "  border: 3px dashed #4d7bef;"
            "  border-radius: 8px;"
            "  padding: 25px;"
            "  background-color: #c0c0c0;" //OLD color: e6f0ff
            "  color: #2c5aa0;"
            "  font-weight: bold;"
            "  font-size: 14px;"
            "}"
            );
        event->acceptProposedAction();
    }
}

void DropZone::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void DropZone::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        if (!urlList.isEmpty()) {
            QString filePath = urlList.first().toLocalFile();
            emit fileDropped(filePath);
        }
    }

    // Reset style
    setStyleSheet(
        "QLabel {"
        "  border: 2px dashed #6c7eb7;"
        "  border-radius: 8px;"
        "  padding: 25px;"
        "  background-color: #f0f4ff;"
        "  color: #445277;"
        "  font-size: 14px;"
        "}"
        );
    event->acceptProposedAction();
}

// 2) FileHistoryDialog Implementation
FileHistoryDialog::FileHistoryDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("File History");
    resize(600, 400);

    QVBoxLayout *layout = new QVBoxLayout(this);

    historyTable = new QTableWidget(this);
    historyTable->setColumnCount(6);
    //store name, operation, data_type, file_path, text_content, timestamp
    historyTable->setHorizontalHeaderLabels(
        QStringList() << "Name" << "Operation" << "Type"
                      << "File Path" << "Text Content" << "Timestamp");
    historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    deleteButton = new QPushButton("Delete Selected", this);
    connect(deleteButton, &QPushButton::clicked, this, &FileHistoryDialog::deleteSelectedRow);

    layout->addWidget(historyTable);
    layout->addWidget(deleteButton);

    setModal(false);
}

void FileHistoryDialog::refreshHistory()
{
    historyTable->setRowCount(0);

    // SELECT from file_history table
    QSqlQuery query("SELECT name, operation, data_type, file_path, text_content, timestamp "
                    "FROM file_history ORDER BY id DESC");

    int row = 0;
    while (query.next()) {
        historyTable->insertRow(row);
        for (int col = 0; col < 6; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());
            historyTable->setItem(row, col, item);
        }
        row++;
    }
}

void FileHistoryDialog::deleteSelectedRow()
{
    int row = historyTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "No Selection", "Please select an entry to delete.");
        return;
    }

    //last column is the timestamp
    QString ts = historyTable->item(row, 5)->text();
    if (ts.isEmpty()) return;

    // Delete from DB
    QSqlQuery query;
    query.prepare("DELETE FROM file_history WHERE timestamp = :ts");
    query.bindValue(":ts", ts);
    if (!query.exec()) {
        QMessageBox::warning(this, "Delete Failed",
                             "Could not delete the entry.\n" + query.lastError().text());
        return;
    }

    // Refresh table
    refreshHistory();
}

//UserGuideDialog Implementation
UserGuideDialog::UserGuideDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("User Guide");
    resize(500, 400);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *heading = new QLabel("Arithmetic Encoding - Overview", this);
    heading->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; }");
    layout->addWidget(heading);

    QTextBrowser *browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(true);

    browser->setHtml(R"(
        <h3>What is Arithmetic Encoding?</h3>
        <p>
          Arithmetic Encoding is a form of entropy encoding used in lossless data compression.
          It can compress both text and image files (PNG, JPG, BMP, GIF).
          Instead of encoding each symbol with a fixed number of bits, it encodes
          the entire message into one number.
        </p>

        <h3>How to Use Arithma-Tech</h3>
        <ol>
          <li>Select <b>File Input</b> if you want to compress/decompress an image
              (PNG, JPG, BMP, GIF). Or choose <b>Text Input</b> to compress raw text.</li>
          <li>Either drag-and-drop your image or click <b>Browse...</b> to choose one.
              For text, simply type or paste it.</li>
          <li>Click <b>Compress</b> to encode your data, or <b>Decompress</b> to restore it.</li>
          <li>Open the <b>File History</b> dialog from the menu to review and manage logs.</li>
        </ol>
    )");
    layout->addWidget(browser);

    setLayout(layout);
    setModal(false);
}

//MainWindow Implementation
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    operationInProgress(false)
{

    setFixedSize(1100,800);

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;


    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);

    darkPalette.setColor(QPalette::Base, QColor(42,42,42));

    darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));

    darkPalette.setColor(QPalette::Text, Qt::white);    // I added this for text color

    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));

    darkPalette.setColor(QPalette::ButtonText, QColor(53, 53, 53));

    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    QApplication::setPalette(darkPalette);

    //darkPalette.setColor(QPalette::Window, QColor(253, 251, 252));
    //darkPalette.setColor(QPalette::WindowText, QColor(65, 72, 86));
    //darkPalette.setColor(QPalette::Base, Qt::white);
    //darkPalette.setColor(QPalette::AlternateBase, QColor(240, 244, 249));
    //darkPalette.setColor(QPalette::Button, QColor(247, 250, 252));
    //darkPalette.setColor(QPalette::ButtonText, QColor(65, 72, 86));
    //darkPalette.setColor(QPalette::Highlight, QColor(75, 139, 237));
    //darkPalette.setColor(QPalette::HighlightedText, Qt::white);


    QFont appFont("Segoe UI", 10);
    QApplication::setFont(appFont);


    //Window properties
    setWindowTitle("Arithma-Tech");
    resize(600, 650);
   // setStyleSheet("QMainWindow { background-color: #f9fafb; }");
    setStyleSheet("QMainWindow { background-color: #2e2e2e; }");

    //popup dialogs
    fileHistoryDialog = new FileHistoryDialog(this);
    userGuideDialog   = new UserGuideDialog(this);

    //menu with "File History" and "User Guide"a
    QMenuBar *menuBarPtr = new QMenuBar(this);

    menuBarPtr->setStyleSheet(
        "QMenuBar::item{"
        "color: #ffffff;"
        "}"
        "QMenuBar::item:selected {"
        "  background-color: #df00ff;"  // hover color
        "  color: #ffffff;"
        "}"
        );

    QAction *historyAction = new QAction("File History", this);
    connect(historyAction, &QAction::triggered, this, &MainWindow::showFileHistory);
    menuBarPtr->addAction(historyAction);

    QAction *guideAction = new QAction("User Guide", this);
    connect(guideAction, &QAction::triggered, this, &MainWindow::showUserGuide);
    menuBarPtr->addAction(guideAction);

    setMenuBar(menuBarPtr);


    QWidget *outerWidget = new QWidget(this);

    QHBoxLayout *outerLayout = new QHBoxLayout(outerWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    //central widget & main layout
    QWidget *central = new QWidget(this);
    central->setFixedWidth(900);

    outerLayout->addStretch();       // blank space on the left
    outerLayout->addWidget(central); // your 900px widget
    outerLayout->addStretch();       // blank space on the right

    setCentralWidget(outerWidget);


    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    mainLayout->setSpacing(16);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(24, 24, 24, 24);


    // Title
    QLabel *titleLabel = new QLabel("Arithma-Tech", central);
    titleLabel->setStyleSheet(
        "QLabel {"
        //"  color: #2c3e50;" // 4b0082
        "  color: #df00ff;"   // Light Purple
        "  font-size: 48px;" //was 24px
        "  font-weight: bold;"
        "  margin-bottom: 10px;"
        "}"
        );
    titleLabel->setAlignment(Qt::AlignCenter);

    // Separator
    QFrame *separator = new QFrame(central);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("QFrame { color: #dce1e8; }");

    // Subtitle
    subtitleLabel = new QLabel("Advanced Data Compression Using Arithmetic Encoding", central);
    subtitleLabel->setStyleSheet("QLabel { font-size: 14px; color: #c0c0c0; }");
    subtitleLabel->setAlignment(Qt::AlignHCenter);

    // Description
    descriptionLabel = new QLabel(
        "Arithma-Tech uses Arithmetic Encoding to achieve high compression ratios\n"
        "for both text and images. Simply drag and drop an image or type/paste text,\n"
        "then click Compress. You can Decompress anytime to restore the data.\n",
        central
        );
    descriptionLabel->setAlignment(Qt::AlignHCenter);
    descriptionLabel->setStyleSheet("QLabel { color: #c0c0c0; }");
    descriptionLabel->setWordWrap(true);
    inputModeGroup = new QGroupBox("", central);
    inputModeGroup->setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold;"
        "  border: 1px solid #dce1e8;"
        "  border-radius: 8px;"
        "  margin-top: 12px;"
        "  padding-top: 20px;"
        "  background-color: #555555 ;" // Color of the Text/File Input box
        "}"
        );

    QHBoxLayout *radioLayout = new QHBoxLayout;
    textModeRadio = new QRadioButton("Text Input", inputModeGroup);
    textModeRadio->setStyleSheet("QRadioButton { font-size: 14px; }");
    fileModeRadio = new QRadioButton("File Input", inputModeGroup);
    fileModeRadio->setStyleSheet("QRadioButton { font-size: 14px; }");

    connect(textModeRadio, &QRadioButton::clicked, this, &MainWindow::switchToTextMode);
    connect(fileModeRadio, &QRadioButton::clicked, this, &MainWindow::switchToFileMode);

    radioLayout->addWidget(textModeRadio);
    radioLayout->addWidget(fileModeRadio);
    inputModeGroup->setLayout(radioLayout);

    // Text input widget
    textInputWidget = new QWidget(central);
    //textInputWidget->setFixedSize(200, 150);
    textInputWidget->setStyleSheet(
        "QWidget {"
        "  background-color: #555555;"
        "  border: 1px solid #dce1e8;"
        "  border-radius: 8px;"
        "}"
        );
    {
        QVBoxLayout *textLayout = new QVBoxLayout(textInputWidget);
        textLayout->setContentsMargins(16, 16, 16, 16);

        QLabel *textLabel = new QLabel("Enter Text below", textInputWidget);
        textLabel->setStyleSheet("QLabel { font-weight: bold; color: #c0c0c0; }");

        textInput = new QTextEdit(textInputWidget);
        textInput->setStyleSheet(
            "QTextEdit {"
            "  border: 1px solid #c5d0e6;"
            "  border-radius: 5px;"
            "  padding: 8px;"
            "  background-color: #c0c0c0;"
            "  color: #000000;"
            "  font-family: 'Consolas', monospace;"
            "}"
            );
        textInput->setMinimumHeight(120);

        textLayout->addWidget(textLabel);
        textLayout->addWidget(textInput);
    }

    // File input widget
    fileInputWidget = new QWidget(central);
    fileInputWidget->setStyleSheet(
        "QWidget {"
        "  background-color: #555555 ;"
        "  border: 1px solid #dce1e8;"
        "  border-radius: 8px;"
        "}"
        );
    {
        QVBoxLayout *fileLayout = new QVBoxLayout(fileInputWidget);
        fileLayout->setContentsMargins(16, 16, 16, 16);

        dropZone = new DropZone(fileInputWidget);
        //dropZone->setFixedSize(200, 200); //White file dropzone resize
        connect(dropZone, &DropZone::fileDropped, this, &MainWindow::handleDroppedFile);

        QHBoxLayout *fileSelectionLayout = new QHBoxLayout;
        fileSelectionLayout->setSpacing(6);

        selectedFileLabel = new QLabel("No file selected", fileInputWidget);
        selectedFileLabel->setStyleSheet("QLabel { color: #808890; padding: 6px; }");

        cancelFileButton = new QToolButton(fileInputWidget);
        cancelFileButton->setText("x");
        cancelFileButton->setToolTip("Clear selected file");
        cancelFileButton->setStyleSheet(
            "QToolButton {"
            "  border: none;"
            "  font-size: 12px;"
            "  color: #c0c0c0;"
            "}"
            "QToolButton:hover {"
            "  color: #df00ff;"
            "}"
            );
        cancelFileButton->hide();
        connect(cancelFileButton, &QToolButton::clicked, this, &MainWindow::clearFileSelection);

        browseButton = new QPushButton("Browse...", fileInputWidget);
        browseButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #555555;"
            "  color: white;"
            "  padding: 8px 16px;"
            "  border: 1px solid #c5d0e6;"
            "  border-radius: 4px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #df00ff; }"
            "QPushButton:pressed { background-color: #d6e1ff; }"
            );
        connect(browseButton, &QPushButton::clicked, this, &MainWindow::browseFile);

        fileSelectionLayout->addWidget(selectedFileLabel);
        fileSelectionLayout->addWidget(cancelFileButton);
        fileSelectionLayout->addStretch();
        fileSelectionLayout->addWidget(browseButton);

        fileLayout->addWidget(dropZone);
        fileLayout->addLayout(fileSelectionLayout);
    }

    // Action widget
    actionWidget = new QWidget(central);
    actionWidget->setStyleSheet(
        "QWidget {"
        "  background-color: #555555;" // ffffff
        "  border: 1px solid #dce1e8;"
        "  border-radius: 8px;"
        "}"
        );
    {
        QVBoxLayout *actionLayout = new QVBoxLayout(actionWidget);
        actionLayout->setContentsMargins(20, 16, 20, 16);

        QHBoxLayout *buttonLayout = new QHBoxLayout;
        buttonLayout->setSpacing(16);

        compressButton = new QPushButton("Compress", actionWidget);
        compressButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #555555;" //  4b7bef
            "  color: white;"
            "  padding: 10px 24px;"
            "  border-radius: 4px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #df00ff; }" //  3d6edf
            "QPushButton:pressed { background-color: #df00ff; }" //  3260cc
            "QPushButton:disabled { background-color: #df00ff; }" //
            );
        compressButton->setMinimumWidth(120);

        decompressButton = new QPushButton("Decompress", actionWidget);
        decompressButton->setStyleSheet(
            "QPushButton {"
            "  background-color: #555555;" //6c7eb7
            "  color: white;"
            "  padding: 10px 24px;"
            "  border-radius: 4px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #df00ff; }"
            "QPushButton:pressed { background-color: #df00ff; }"
            "QPushButton:disabled { background-color: #df00ff; }"
            );
        decompressButton->setMinimumWidth(120);

        buttonLayout->addStretch();
        buttonLayout->addWidget(compressButton);
        buttonLayout->addWidget(decompressButton);
        buttonLayout->addStretch();

        connect(compressButton, &QPushButton::clicked, this, &MainWindow::compressFile);
        connect(decompressButton, &QPushButton::clicked, this, &MainWindow::decompressFile);

        QWidget *progressWidget = new QWidget(actionWidget);
        QVBoxLayout *progressLayout = new QVBoxLayout(progressWidget);
        progressLayout->setContentsMargins(0, 16, 0, 0);

        QLabel *progressLabel = new QLabel("Progress", progressWidget);
        progressLabel->setStyleSheet("QLabel { color: #c0c0c0; font-weight: bold; }"); //  445277
        progressLabel->setAlignment(Qt::AlignLeft);

        progressBar = new QProgressBar(progressWidget);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        progressBar->setMinimumHeight(20);
        progressBar->setStyleSheet(
            "QProgressBar {"
            "  border: 1px solid #dce1e8;"
            "  border-radius: 4px;"
            "  text-align: center;"
            "  background-color: #c0c0c0;" // f0f4ff
            "  color: #445277;"
            "}"
            "QProgressBar::chunk {"
            "  background-color: #df00ff;"
            "  border-radius: 3px;"
            "}"
            );

        progressLayout->addWidget(progressLabel);
        progressLayout->addWidget(progressBar);

        actionLayout->addLayout(buttonLayout);
        actionLayout->addWidget(progressWidget);

        statusLabel = new QLabel("Ready to start...", actionWidget);
        statusLabel->setStyleSheet(
            "QLabel {"
            "  color: #666f7f;"
            "  padding: 8px;"
            "  background-color: #c0c0c0;" // f0f4ff
            "  border: 1px solid #dce1e8;"
            "  border-radius: 4px;"
            "}"
            );
        statusLabel->setAlignment(Qt::AlignCenter);

        actionLayout->addWidget(statusLabel);
    }

    progressTimer = new QTimer(this);
    connect(progressTimer, &QTimer::timeout, this, &MainWindow::updateProgressBar);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(separator);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addWidget(descriptionLabel);

    mainLayout->addWidget(inputModeGroup);
    mainLayout->addWidget(textInputWidget);
    mainLayout->addWidget(fileInputWidget);
    mainLayout->addWidget(actionWidget);

   // setCentralWidget(central);

    // Default to file mode
    fileModeRadio->setChecked(true);
    textInputWidget->setVisible(false);
    fileInputWidget->setVisible(true);

    initializeDatabase();
}

MainWindow::~MainWindow()
{
    if (progressTimer->isActive()) {
        progressTimer->stop();
    }
}

// 4.1) Database Setup
void MainWindow::initializeDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("arithma_tech.db");

    if (!db.open()) {
        qDebug() << "Failed to open database:" << db.lastError().text();
        QMessageBox::critical(this, "Database Error", "Could not open the SQLite database.");
        return;
    }

    // Create table if not exists
    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS file_history ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "name TEXT,"
                    "operation TEXT,"
                    "data_type TEXT,"
                    "file_path TEXT,"
                    "text_content TEXT,"
                    "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP)"))
    {
        qDebug() << "Create table error:" << query.lastError().text();
    }
}

// Insert a record into DB
void MainWindow::addHistoryEntry(const QString &fileName, const QString &operation)
{
    QSqlQuery query;
    query.prepare("INSERT INTO file_history ("
                  "name, operation, data_type, file_path, text_content"
                  ") VALUES ("
                  ":name, :operation, :data_type, :file_path, :text_content)"
                  );

    bool isText = textModeRadio->isChecked();
    if (isText) {
        query.bindValue(":name", "Text Data");
        query.bindValue(":operation", operation);
        query.bindValue(":data_type", "Text");
        query.bindValue(":file_path", QString());  // store an empty string for file_path

        // For text_content, store the actual text
        query.bindValue(":text_content", textInput->toPlainText());

    } else {
        query.bindValue(":name", fileName);
        query.bindValue(":operation", operation);
        query.bindValue(":data_type", "File");
        query.bindValue(":file_path", currentFilePath);
        query.bindValue(":text_content", QString());
    }

    if (!query.exec()) {
        qDebug() << "Error inserting row in DB:" << query.lastError().text();
    }

    // Refresh the File History if open
    if (fileHistoryDialog) {
        fileHistoryDialog->refreshHistory();
    }
}

// Utility: isImageFile
bool MainWindow::isImageFile(const QString &filePath)
{
    static QStringList allowedExtensions = {"png","jpg","jpeg","bmp","gif"};
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    return allowedExtensions.contains(suffix);
}

// Switch input modes
void MainWindow::switchToTextMode()
{
    textInputWidget->setVisible(true);
    fileInputWidget->setVisible(false);

    clearFileSelection();
    statusLabel->setText("Text input mode selected");
}

void MainWindow::switchToFileMode()
{
    textInputWidget->setVisible(false);
    fileInputWidget->setVisible(true);

    textInput->clear();
    statusLabel->setText("File input mode selected");
}

// Browse for an image
void MainWindow::browseFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Image",
        "",
        "Images (*.png *.jpg *.jpeg *.bmp *.gif)" // only images
        );
    if (!filePath.isEmpty()) {
        handleDroppedFile(filePath);
    }
}

// Drag-and-drop or browsed file
void MainWindow::handleDroppedFile(const QString &filePath)
{
    if (!isImageFile(filePath)) {
        QMessageBox::warning(this, "Unsupported File",
                             "This is not a recognized image format.\n"
                             "Supported formats: PNG, JPG, JPEG, BMP, GIF.");
        return;
    }

    currentFilePath = filePath;
    QFileInfo fi(filePath);

    selectedFileLabel->setText(fi.fileName());
    selectedFileLabel->setStyleSheet(
        "QLabel { color: #445277; font-weight: bold; padding: 6px; }");
    cancelFileButton->show();

    statusLabel->setText("File selected: " + fi.fileName());
}

// Clear file selection
void MainWindow::clearFileSelection()
{
    currentFilePath.clear();
    selectedFileLabel->setText("No file selected");
    selectedFileLabel->setStyleSheet(
        "QLabel { color: white; padding: 6px; }");
    cancelFileButton->hide();
    statusLabel->setText("File selection cleared");
}

// Compress
void MainWindow::compressFile()
{
    if (operationInProgress) return;

    bool isText = textModeRadio->isChecked();

    if (isText) {
        QString text = textInput->toPlainText();
        if (text.isEmpty()) {
            QMessageBox::warning(this, "Empty Input", "Please enter text to compress");
            return;
        }
        operationInProgress = true;
        statusLabel->setText("⚙️ Compressing text...");

        addHistoryEntry("Text Data", "Compress");
    } else {
        // file mode
        if (currentFilePath.isEmpty()) {
            QMessageBox::warning(this, "No File Selected", "Please select an image to compress");
            return;
        }
        operationInProgress = true;
        statusLabel->setText("⚙️ Compressing: " + QFileInfo(currentFilePath).fileName());

        addHistoryEntry(QFileInfo(currentFilePath).fileName(), "Compress");
    }

    compressButton->setEnabled(false);
    decompressButton->setEnabled(false);

    resetProgressBar();
    progressTimer->start(150); // simulate progress
}

// Decompress
void MainWindow::decompressFile()
{
    if (operationInProgress) return;

    bool isText = textModeRadio->isChecked();

    if (isText) {
        QString text = textInput->toPlainText();
        if (text.isEmpty()) {
            QMessageBox::warning(this, "Empty Input", "Please enter text to decompress");
            return;
        }
        operationInProgress = true;
        statusLabel->setText("⚙️ Decompressing text...");

        addHistoryEntry("Text Data", "Decompress");
    } else {
        if (currentFilePath.isEmpty()) {
            QMessageBox::warning(this, "No File Selected", "Please select an image to decompress");
            return;
        }
        operationInProgress = true;
        statusLabel->setText("⚙️ Decompressing: " + QFileInfo(currentFilePath).fileName());

        addHistoryEntry(QFileInfo(currentFilePath).fileName(), "Decompress");
    }

    compressButton->setEnabled(false);
    decompressButton->setEnabled(false);

    resetProgressBar();
    progressTimer->start(200); // simulate progress
}

// Progress bar
void MainWindow::updateProgressBar()
{
    int val = progressBar->value();
    if (val < 100) {
        progressBar->setValue(val + 5);
    } else {
        progressTimer->stop();
        operationInProgress = false;
        compressButton->setEnabled(true);
        decompressButton->setEnabled(true);

        bool isTextMode = textModeRadio->isChecked();

        if (statusLabel->text().contains("Compressing")) {
            statusLabel->setText("✓ Compression completed successfully");
            statusLabel->setStyleSheet(
                "QLabel {"
                "  color: #2d8a54;"
                "  padding: 8px;"
                "  background-color: #e8f6ee;"
                "  border: 1px solid #b8e0c5;"
                "  border-radius: 4px;"
                "  font-weight: bold;"
                "}"
                );

            QString msg = isTextMode
                              ? "Your text has been compressed successfully!"
                              : "Your image has been compressed successfully!";
            QMessageBox::information(this, "Compression Complete", msg);

        } else if (statusLabel->text().contains("Decompressing")) {
            statusLabel->setText("✓ Decompression completed successfully");
            statusLabel->setStyleSheet(
                "QLabel {"
                "  color: #2d8a54;"
                "  padding: 8px;"
                "  background-color: #e8f6ee;"
                "  border: 1px solid #b8e0c5;"
                "  border-radius: 4px;"
                "  font-weight: bold;"
                "}"
                );

            QString msg = isTextMode
                              ? "Your text has been decompressed successfully!"
                              : "Your image has been decompressed successfully!";
            QMessageBox::information(this, "Decompression Complete", msg);
        }
    }
}

// Reset progress bar
void MainWindow::resetProgressBar()
{
    progressBar->setValue(0);
    statusLabel->setStyleSheet(
        "QLabel {"
        "  color: #666f7f;"
        "  padding: 8px;"
        "  background-color: #f0f4ff;"
        "  border: 1px solid #dce1e8;"
        "  border-radius: 4px;"
        "}"
        );
}

// Show file history
void MainWindow::showFileHistory()
{
    fileHistoryDialog->refreshHistory();
    fileHistoryDialog->show();
    fileHistoryDialog->raise();
    fileHistoryDialog->activateWindow();
}

// Show user guide
void MainWindow::showUserGuide()
{
    userGuideDialog->show();
    userGuideDialog->raise();
    userGuideDialog->activateWindow();
}
