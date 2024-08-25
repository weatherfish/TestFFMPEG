#ifndef SDL_QT_H
#define SDL_QT_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class SDLQt;
}
QT_END_NAMESPACE

class SDLQt : public QWidget
{
    Q_OBJECT

public:
    SDLQt(QWidget *parent = nullptr);
    ~SDLQt();

    void timerEvent(QTimerEvent *ev);

private:
    Ui::SDLQt *ui;
};
#endif // SDL_QT_H
