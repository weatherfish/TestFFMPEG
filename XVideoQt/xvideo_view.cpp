#include "xvideo_view.h"
#include "xsdl.h"

XVideoView::XVideoView() {}

XVideoView* XVideoView::create(RenderType type){
    switch (type) {
    case SDL:
        return new XSDL();
        break;
    default:
        break;
    }
    return nullptr;
}
