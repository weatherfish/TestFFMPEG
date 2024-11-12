 #ifndef XDEMUXTASK_H
#define XDEMUXTASK_H

#include "xdemux.h"
#include "xthread.h"

class XDemuxTask : public XThread
{
public:
    void Main() override;

    bool Open(std::string url, int timeout = 1000);

private:
    XDemux demux_;
};

#endif // XDEMUXTASK_H
