#include "sdl_qt.h"
#include "./ui_sdl_qt.h"
#include "xvideo_view.h"
#include <iostream>
#include <QMessageBox>
#include <QSpinBox>
#include <QFileDialog>
#include <vector>


extern "C"{
#include <libavcodec/avcodec.h>
}


static std::vector<XVideoView*> views;

void SDLQt::timerEvent(QTimerEvent *ev){

}

SDLQt::SDLQt(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SDLQt)
{

    ui->setupUi(this);

    connect(this, SIGNAL(ViewS()), this, SLOT(View()));//绑定信号和槽:只要调用了ViewS，会主动调用View

    views.push_back(XVideoView::create());
    views.push_back(XVideoView::create());

    auto wid1 = ui->video1->winId();
    auto wid2 = ui->video2->winId();

    views[0]->setWinID((void*)wid1);
    views[1]->setWinID((void*)wid2);

    th_ = std::thread(&SDLQt::Main, this);
}

void SDLQt::resizeEvent(QResizeEvent *ev) {
    std::cout<<"resizeEvent w=" <<ui->video1->width()<<", h="<<ui->video1->height()<<std::endl;
//     ui->video1->resize(400, 300);
//     ui->video2->resize(400, 300);
//     // ui->video->move(0, 0);

//     // view->scale(width(), height());  //设置为窗口的宽高
}

void SDLQt::View(){
    for (int i = 0; i < views.size(); ++i) {
        auto frame = views[i]->Read();
        if(!frame)continue;
        views[i]->DrawFrame(frame);

        //显示fps
        std::stringstream ss;
        ss<<"fps:"<<views[i]->renderFps();
        if(i == 0){

        }else{

        }

    }
}

void SDLQt::Main(){
    while (!isExit_) {
        emit ViewS();
        MSleep(20);
    }
}

void SDLQt::Open1()
{
    Open(0);
}
void SDLQt::Open2(){
    Open(1);
}
void SDLQt::Open(int index){
    QFileDialog fd;
    auto filename = fd.getOpenFileName();
    if(filename.isEmpty()) return;
    std::cout<<filename.toLocal8Bit().data()<<std::endl;

    if(!views[index]->Open(filename.toStdString())){
        std::cout<<"Open Failed"<< filename.toStdString()<<std::endl;
        return;
    }

    int w = index == 0 ? ui->width1->value() : ui->width2->value();
    int h = index == 0 ? ui->height1->value() : ui->height2->value();
    auto pix = index == 0 ? ui->pix1->currentText() : ui->pix2->currentText();

    XVideoView::VideoFormat fmt = XVideoView::YUV420P;
    if(pix == "YUV420P"){
        fmt = XVideoView::YUV420P;
    } else if (pix == "ARGB") {
        fmt = XVideoView::ARGB;
    }else if (pix == "BGRA") {
        fmt = XVideoView::BGRA;
    }else if (pix == "RGBA") {
        fmt = XVideoView::RGBA;
    }

    //初始化窗口和材质
    views[index]->Init(w, h, fmt);

}

void SDLQt::MSleep(unsigned int ms){
    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ms; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto end = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() >= ms)break;
    }
}

SDLQt::~SDLQt()
{
    isExit_ = true;
    th_.join();  //等待渲染线程退出
    delete ui;
}
