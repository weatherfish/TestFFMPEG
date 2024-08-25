#include "sdl_qt.h"
#include "./ui_sdl_qt.h"
#include "xvideo_view.h"
#include <fstream>
#include <QMessageBox>

static XVideoView *view = nullptr;
static int sdl_width = 0;
static int sdl_height = 0;
static unsigned char* yuv = nullptr;
static std::ifstream yuvFile;

void SDLQt::timerEvent(QTimerEvent *ev){
    if(view->IsExit()){
        view->Close();
        exit(0);
    }
    yuvFile.read((char*)yuv, sdl_width * sdl_height * 1.5);
    view->Draw(yuv);
}

SDLQt::SDLQt(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SDLQt)
{
    yuvFile.open("/Users/wangqiang/Movies/output.yuv", std::ios::binary);
    if(!yuvFile){
        QMessageBox::information(this, "", "open yuv failed!");
    }

    ui->setupUi(this);
    sdl_width = 400;
    sdl_height = 300;
    ui->label->resize(sdl_width, sdl_height);
    view = XVideoView::create();
    view->Init(sdl_width, sdl_height, XVideoView::YUV420P, (void*)ui->label->winId());

    yuv = new unsigned char[sdl_width * sdl_height * 1.5];
    startTimer(10);
}

void SDLQt::resizeEvent(QResizeEvent *ev) {
    ui->label->resize(size());
    ui->label->move(0, 0);

    // view->scale(width(), height());  //设置为窗口的宽高
}

SDLQt::~SDLQt()
{
    delete ui;
}
