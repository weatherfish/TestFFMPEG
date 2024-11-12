#include "xthread.h"



void XThread::Start(){
    std::unique_lock<std::mutex>lock(mtx_);

    static int i = 0;
    i++;
    index_ = i;

    is_exit_ = false;
    th_ = std::thread(&XThread::Main, this);

    std::stringstream ss;
    ss<<"XThread Start"<<index_;
    XLogInfo(ss.str()) ;
    th_.detach(); //分离主线程和子线程
}

void XThread::Stop(){
    std::stringstream ss;
    ss<<"XThread Stop begin"<<index_;
    XLogInfo(ss.str()) ;
    is_exit_ = true;
    if(th_.joinable()){ // 判断线程是否可以等待子线程退出
        th_.join(); //等待子线程退出
    }
    ss.str("");
    ss<<"XThread Stop end"<<index_;
    XLogInfo(ss.str()) ;
}
