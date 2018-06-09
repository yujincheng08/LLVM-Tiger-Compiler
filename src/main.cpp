#include <QApplication>
#include "IDE/mainwindow.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  //引入样式表
  QFile qss(":/style.qss");  //用相对路径
  qss.open(QFile::ReadOnly);
  qApp->setStyleSheet(qss.readAll());
  qss.close();

  MainWindow w;
  w.show();

  return a.exec();
  return 0;
}
