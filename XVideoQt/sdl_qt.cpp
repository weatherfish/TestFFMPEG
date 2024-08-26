#include "sdl_qt.h"
#include "./ui_sdl_qt.h"
#include "xvideo_view.h"
#include <fstream>
#include <iostream>
#include <QMessageBox>
#include <QSpinBox>

extern "C"{
#include <libavcodec/avcodec.h>
}

static XVideoView *view = nullptr;
static int sdl_width = 0;
static int sdl_height = 0;
static std::ifstream yuvFile;
static long long fileSize = 0;

static QLabel *fpsLabel = nullptr;
static QSpinBox *fpsSpinBox = nullptr;
int fps = 25;

static AVFrame *frame = nullptr;

void SDLQt::timerEvent(QTimerEvent *ev){
    if(view->IsExit()){
        view->Close();
        exit(0);
    }

    //yuv420p读取
    yuvFile.read((char*)frame->data[0], sdl_width * sdl_height);
    yuvFile.read((char*)frame->data[1], sdl_width * sdl_height / 4);
    yuvFile.read((char*)frame->data[2], sdl_width * sdl_height / 4);
    frame->linesize[0] = sdl_width;
    frame->linesize[1] = sdl_width/2;
    frame->linesize[2] = sdl_width/2;

    view->DrawFrame(frame);
}

SDLQt::SDLQt(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SDLQt)
{
    yuvFile.open("/Users/wangqiang/Movies/output.yuv", std::ios::binary);
    if(!yuvFile){
        QMessageBox::information(this, "", "open yuv failed!");
    }
    yuvFile.seekg(0, std::ios::end);//移动到文件结尾
    fileSize = yuvFile.tellg();//获取文件大小
    yuvFile.seekg(0, std::ios::beg);//移动到文件开头

    ui->setupUi(this);

    connect(this, SIGNAL(ViewS()), this, SLOT(View()));//绑定信号和槽:只要调用了ViewS，会主动调用View


    sdl_width = 400;
    sdl_height = 300;
    ui->label->resize(sdl_width, sdl_height);
    view = XVideoView::create();
    view->Init(sdl_width, sdl_height, XVideoView::YUV420P, (void*)ui->label->winId());

    fpsLabel = new QLabel(this);
    fpsLabel->resize(50, 15);

    fpsSpinBox = new QSpinBox(this);
    fpsSpinBox->move(60, 0);
    fpsSpinBox->setValue(fps );
    fpsSpinBox->setRange(1, 100);

    // // 创建一个新的调色板对象
    // QPalette palette = fpsLabel->palette();
    // // 设置文本颜色为红色
    // palette.setColor(QPalette::WindowText, Qt::red);
    // // 应用新的调色板到QLabel上
    // fpsLabel->setPalette(palette);

    // // 创建一个新的字体对象，并设置字号为20像素
    // QFont font = fpsLabel->font();
    // font.setPointSize(20);
    // // 设置QLabel的字体为新创建的字体对象
    // fpsLabel->setFont(font);


    frame = av_frame_alloc(); //申请对象空间
    frame->width = sdl_width;
    frame->height = sdl_height;
    frame->format = AV_PIX_FMT_YUV420P;

    auto re = av_frame_get_buffer(frame, 0); //申请图像空间，使用默认对齐方式  32位
    if(re != 0){
        av_log(NULL, AV_LOG_ERROR, "get buffer errror %s\n", av_err2str(re));
    }
    // startTimer(10);
    th_ = std::thread(&SDLQt::Main, this);
}

void SDLQt::resizeEvent(QResizeEvent *ev) {
    ui->label->resize(size());
    ui->label->move(0, 0);

    // view->scale(width(), height());  //设置为窗口的宽高
}

void SDLQt::View(){
    if(view->IsExit()){
        view->Close();
        exit(0);
    }

    //yuv420p读取
    yuvFile.read((char*)frame->data[0], sdl_width * sdl_height);
    yuvFile.read((char*)frame->data[1], sdl_width * sdl_height / 4);
    yuvFile.read((char*)frame->data[2], sdl_width * sdl_height / 4);
    frame->linesize[0] = sdl_width;
    frame->linesize[1] = sdl_width/2;
    frame->linesize[2] = sdl_width/2;

    if(fileSize == yuvFile.tellg()){ //已经读取到末尾
        yuvFile.seekg(0, std::ios::beg);//移动到文件开头
    }

    view->DrawFrame(frame);

    std::stringstream ss;
    ss<<"fps:"<<view->renderFps();

    fpsLabel->setText(ss.str().c_str()); //只能在槽函数中调用

    fps = fpsSpinBox->value(); //拿到设置的帧率
}

void SDLQt::Main(){
    while (!isExit_) {
        emit ViewS();
        if(fps > 0){
            view->MSleep(1000/fps);
            // std::this_thread::sleep_for(std::chrono::milliseconds(1000/fps));
        }else {
            // std::this_thread::sleep_for(std::chrono::milliseconds(40));
            view->MSleep(40);
        }

    }
}

SDLQt::~SDLQt()
{
    isExit_ = true;
    th_.join();  //等待渲染线程退出
    delete ui;
}
