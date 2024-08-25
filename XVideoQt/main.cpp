#include "sdl_qt.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SDLQt w;
    w.show();
    return a.exec();
}
