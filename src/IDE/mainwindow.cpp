#include "mainwindow.h"
#include <stdlib.h>
#include <unistd.h>
#include "highlighter.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  Highlighter *highlighter = new Highlighter(
      ui->textEdit->document());  //调用方法，新建对象并传递document
  this->title = "";
  this->setWindowTitle("Untitled----Super Tiger");

  this->ui->textEdit->setCurrentFont(QFont("Times", 28));

  // Tree
  ui->treeWidget->setHeaderLabels(QStringList() << "AST Tree");
  root = new QTreeWidgetItem();
  root->setText(0, "Root");
  ui->treeWidget->addTopLevelItem(root);
  compiler = new Compiler();
}

MainWindow::~MainWindow() {
  delete ui;
  delete compiler;
  delete root;
}

//文件栏
void MainWindow::on_FPrint_triggered() { qDebug() << "Print the text!"; }

void MainWindow::on_FExit_triggered() { close(); }

void MainWindow::on_FNew_triggered() {
  if (ui->textEdit->document()->isModified()) {
    int ok = QMessageBox::question(
        this, "Question",
        "The text has been modified, do you wanted to save the document?");
    if (ok == 16384) {
      //保存修改后的文档
      this->on_FSaveAs_triggered();
    }
  }
  //清空内容，新建文本
  ui->textEdit->clear();
  this->setWindowTitle("Untitled----Super Tiger");
  this->title = "";
  this->ui->textEdit->setCurrentFont(QFont("Times", 28));
}

void MainWindow::on_FSaveAs_triggered() {
  //通过对话框的形式获得文件名，并自动添加.tig后缀
  this->title = QFileDialog::getSaveFileName(
      this, "Save File", QDir::currentPath(), "tig(*.tig);;all(*.*)");
  if (this->title.isEmpty()) {
    QMessageBox::warning(this, "Error", "Please enter a file name!");
    return;
  }
  QFile *file = new QFile;
  file->setFileName(this->title);
  bool ok = file->open(QIODevice::WriteOnly);

  if (ok) {
    //基于文本流的写方法，效率较高
    QTextStream out(file);               //文件与文本流相关联
    out << ui->textEdit->toPlainText();  //纯文本,直接存到磁盘中
    file->close();
    delete file;  // Qt的垃圾自动回收仅限于UI部分，所以这些值要手动释放
    this->setWindowTitle(this->title + "----Super Assembler");
  } else {
    QMessageBox::warning(this, "Error", "File can't be saved!");
    return;
  }
  this->ui->textEdit->setCurrentFont(QFont("Times", 28));
}

void MainWindow::on_FSave_triggered() {
  if (!ui->textEdit->document()->isModified() && !this->title.isEmpty()) return;
  if (this->title.isEmpty()) {
    this->on_FSaveAs_triggered();  //如果未文件没有命名，调用save as
    return;
  }

  //否则直接在原来的文件上写
  QFile *file = new QFile;
  file->setFileName(this->title);
  bool ok = file->open(QIODevice::WriteOnly);
  if (ok) {
    //与上面完全一致
    QTextStream out(file);
    out << ui->textEdit->toPlainText();
    file->close();
    delete file;
  } else {
    QMessageBox::warning(this, "Error", "File can't be saved!");
    return;
  }
  this->ui->textEdit->document()->setDefaultFont(QFont("Times", 28));
}

void MainWindow::on_FOpen_triggered() {
  //打开新文档前询问是否要保存现有文档（如果被修改过的话）
  if (ui->textEdit->document()->isModified()) {
    int ok = QMessageBox::question(this, "Question",
                                   "The text has been modified, do you want to "
                                   "save the document before open a new text?");
    if (ok) {
      //保存修改后的文档
      this->on_FSave_triggered();
    }
  }

  this->title = QFileDialog::getOpenFileName(
      this, "Open File", QDir::currentPath(), "tig(*.tig);;all(*.*)");

  if (this->title.isEmpty()) {
    QMessageBox::warning(this, "Error", "Please select a file");
    return;
  }
  QFile *file = new QFile;
  file->setFileName(this->title);
  bool ok = file->open(QIODevice::ReadOnly);  //只读模式读取文件流
  if (ok) {
    QTextStream in(file);
    ui->textEdit->setText(in.readAll());
    file->close();
    delete file;
    this->setWindowTitle(this->title + "----Super Assembler");
  }
}

//编辑栏
void MainWindow::on_EUndo_triggered() { ui->textEdit->undo(); }

void MainWindow::on_ECut_triggered() { ui->textEdit->cut(); }

void MainWindow::on_ECopy_triggered() { ui->textEdit->copy(); }

void MainWindow::on_EPaste_triggered() { ui->textEdit->paste(); }

void MainWindow::on_EDelete_triggered() { qDebug() << "delete"; }

void MainWindow::on_ESelectAll_triggered() { ui->textEdit->selectAll(); }

void MainWindow::on_ERedo_triggered() { ui->textEdit->redo(); }

void MainWindow::on_BRun_triggered() {
  //  if (ui->textEdit->document()->isModified()) {
  //    this->on_FSaveAs_triggered();
  //  }
  this->ui->treeWidget->clear();
  root = new QTreeWidgetItem();
  root->setText(0, "Root");
  ui->treeWidget->addTopLevelItem(root);
  qDebug() << "run";
}

void MainWindow::on_BCompile_triggered() {
  QString text = ui->textEdit->toPlainText();
  if (text.length() == 0) return;

  //  system("rm ast");
  //  std::string cmd =
  //      "echo '" + text.toStdString() + "' | " + compilerPath + " >> ast";
  //  system(cmd.c_str());

  this->ui->treeWidget->clear();
  root = new QTreeWidgetItem();
  root->setText(0, "Root");
  ui->treeWidget->addTopLevelItem(root);

  this->compiler->compile(root, text.toStdString());

  qDebug() << "compile";
  this->ui->textBrowser->setText(text);
}

void MainWindow::on_SFont_triggered() {
  // get user selected font
  bool ok;
  QFont font = QFontDialog::getFont(&ok, QFont("Times", 16), this);
  if (ok) {
    // font is set to the font the user selected
    this->ui->textEdit->document()->setDefaultFont(font);
  } else {
    // the user canceled the dialog; font is set to the initial
    // value, in this case Times, 12.
    QMessageBox::information(this, "Error", "Error font set");
  }
}

void MainWindow::on_SBackGroundColor_triggered() {
  QColor color = QColorDialog::getColor(
      Qt::red, this);  //色调盘的初始颜色，不是文本的初始颜色
  if (color.isValid()) {
    ui->textEdit->setTextBackgroundColor(color);
  } else {
    QMessageBox::information(this, "Error", "The Color set error");
  }
}

void MainWindow::on_O_About_triggered() {
  QMessageBox::information(
      this, "About Author",
      "    The Spuer Tiger is designed by fanghao, yujincheng & liu qi, students in ZJU of computer science and technology college.\n \
                                                    ——June 2018");
}

void MainWindow::on_O_Help_triggered() {
  QString text;

  text =
      "The Super Tiger has five main funkctions:File operation, Edit operation, Build operation\
, Setting operation and Other operation.\n\n\
File: read or write '.tig' files;\n\
Edit: common editor operation;\n\
Build: assemble '.tig' files and run obj code;\n\
Setting: set font and text background color;\n\
Other: brief introduction;\n\n\
Learn more in help document.";

  QMessageBox::information(this, "Help", text);
}
