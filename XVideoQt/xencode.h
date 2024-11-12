#ifndef XENCODE_H
#define XENCODE_H

#include "xcodec.h"
#include <vector>

class XEncode: public XCodec
{
public:


    AVPacket* Encode(const AVFrame* frame);

    std::vector<AVPacket*> End();//返回缓存的数据


};

#endif // XENCODE_H
