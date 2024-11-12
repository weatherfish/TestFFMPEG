#ifndef XDEMUX_H
#define XDEMUX_H

#include "xformat.h"

class XDemux : public XFormat
{
public:
    //打开解封装，支持rtsp
    static AVFormatContext* Open(const char* url);
    bool Read(AVPacket *pkt);

    bool Seek(long long pts, int stream_index);
};

#endif // XDEMUX_H
