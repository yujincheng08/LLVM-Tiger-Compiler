#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QColorDialog>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QIODevice>
#include <QMainWindow>
#include <QMessageBox>
#include <QProcess>
#include <QString>
#include <QTextStream>
#include <QTreeWidgetItem>
#include <iostream>
#include "compiler.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

 private:
  QString title;
  std::string compilerPath =
      "/Users/fanghao/workstations/QT/"
      "build-Tiny-Tiger-Desktop_Qt_5_10_0_clang_64bit-Debug/Tiny-Tiger.app/"
      "Contents/MacOS/Tiny-Tiger";

  //槽函数集
 private slots:
  void on_FPrint_triggered();

  void on_FExit_triggered();

  void on_FNew_triggered();

  void on_FSaveAs_triggered();

  void on_FSave_triggered();

  void on_FOpen_triggered();

  void on_EUndo_triggered();

  void on_ECut_triggered();

  void on_ECopy_triggered();

  void on_EPaste_triggered();

  void on_EDelete_triggered();

  void on_ESelectAll_triggered();

  void on_ERedo_triggered();

  void on_BCompile_triggered();

  void on_BRun_triggered();

  void on_SFont_triggered();

  void on_SBackGroundColor_triggered();

  void on_O_About_triggered();

  void on_O_Help_triggered();

 private:
  Ui::MainWindow *ui;

  QTreeWidgetItem *root;
  Compiler *compiler;
};

#endif  // MAINWINDOW_H
