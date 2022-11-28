#include "treeviewdialog.h"
#include <QApplication>
#include <QTimer>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    TreeViewDialog* w = new TreeViewDialog;
    w->show();

    QTimer::singleShot(0, w, &TreeViewDialog::init);

    return a.exec();
}
