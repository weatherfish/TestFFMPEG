#ifndef XTHREAD_H
#define XTHREAD_H

#include<thread>
#include <iostream>

enum XLogLevel{
    XLOG_TYPE_DEBUG,
    XLOG_TYPE_INFO,
    XLOG_TYPE_ERROR,
    XLOG_TYPE_FATAL,
};

#define XLOG_LEVEL_MIN 0

#define XLog(s, level) \
    if(level >= XLOG_LEVEL_MIN) \
    std::cout<<level<<":"<<__FILE__<<":"<<__LINE__<<":"<<s<<std::endl;

#define XLogDebug(s) XLog(s, XLOG_TYPE_DEBUG)
#define XLogInfo(s) XLog(s, XLOG_TYPE_INFO)
#define XLogError(s) XLog(s, XLOG_TYPE_ERROR)
#define XLogFatal(s) XLog(s, XLOG_TYPE_FATAL)

class XThread
{
public:
    virtual void Start(); //启动线程

    virtual void Stop();  //停止线程


protected:

    virtual void Main() = 0; //入口函数

    bool is_exit_ = false;
    std::thread th_;
    std::mutex mtx_;

    int index_;
};

#endif // XTHREAD_H
