#ifndef XMUX_H
#define XMUX_H

#include "xformat.h"

class XMux : public XFormat
{
public:
    static AVFormatContext* Open(const char* url);

    bool WriteHead();
    bool Write(AVPacket *pkt);
    bool WriteEnd();
};

#endif // XMUX_H
