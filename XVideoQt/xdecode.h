#ifndef XDECODE_H
#define XDECODE_H

#include "xcodec.h"
#include <vector>

class XDecode : public XCodec
{
public:
    bool Send(const AVPacket *pkt);
    bool Recieve(AVFrame *frame);

    std::vector<AVFrame*> End();//返回缓存的数据

    //AV_HWDEVICE_TYPE_DXVA2 = 4
    bool initHW(int type = 4);//初始化硬件加速

private:

};

#endif // XDECODE_H
