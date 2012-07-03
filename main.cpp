#include <QtGui/QApplication>
#include "mainwindow.h"
#include "pgmimage.h"

/** TODO
 * -> global error codes
 * -> source kernel out
 * -> file headers
 */

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    PgmImage pgmImage;

    MainWindow w(&pgmImage);
    w.show();

    return a.exec();
}
