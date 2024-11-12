#ifndef SDL_QT_H
#define SDL_QT_H

#include <QWidget>
#include <thread>

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

    void timerEvent(QTimerEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;

    void MSleep(unsigned int ms);

    void Main();
signals:
    void ViewS(); //信号函数，将任务放入列表
public slots:
    void View();//显示的槽函数，
    void Open1();
    void Open2();
    void Open(int index);
private:
    std::thread th_;
    bool isExit_ = false;
    Ui::SDLQt *ui;
};
#endif // SDL_QT_H
