#ifndef QUEUE_H
#define QUEUE_H

#include <libavutil/avutil.h>
#include <libavutil/fifo.h>
#include <SDL.h>
#include <libavcodec/avcodec.h>

typedef struct _MyPacketElement {
    AVPacket *pkt;
} MyPacketElement;

typedef struct _PacketQueue {
    AVFifo *pkts;
    int nb_packets;
    int size;
    int64_t duration;

    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

static int packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->pkts = av_fifo_alloc2(1, sizeof(MyPacketElement), AV_FIFO_FLAG_AUTO_GROW);
    if (!q->pkts) {
        return AVERROR(ENOMEM);
    }
    q->mutex = SDL_CreateMutex();
    if (!q->mutex) {
        return AVERROR(ENOMEM);
    }
    q->cond = SDL_CreateCond();
    if (!q->cond) {
        return AVERROR(ENOMEM);
    }
    return 0;
}

static int packet_queue_put_priv(PacketQueue *q, AVPacket *pkt) {
    MyPacketElement elem;
    int ret = -1;
    elem.pkt = pkt;
    ret = av_fifo_write(q->pkts, &elem, 1);
    if (ret < 0) {
        return ret;
    }
    q->nb_packets++;
    q->size += elem.pkt->size + sizeof(elem);
    q->duration += elem.pkt->duration;

    SDL_CondSignal(q->cond);

    return ret;
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    AVPacket *pkt1;
    int ret;
    pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt);
        return -1;
    }
    av_packet_move_ref(pkt1, pkt);

    SDL_LockMutex(q->mutex);
    ret = packet_queue_put_priv(q, pkt1);
    SDL_UnlockMutex(q->mutex);

    if (ret < 0) {
        av_packet_unref(pkt);
        return -1;
    }
    return ret;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    MyPacketElement *element;
    int ret = -1;

    SDL_LockMutex(q->mutex);
    for (;;) {
        if (av_fifo_read(q->pkts, &element, 1) >= 0) {
            q->nb_packets--;
            q->size -= element->pkt->size + sizeof(element);
            q->duration -= element->pkt->duration;
            av_packet_move_ref(pkt, element->pkt);
            av_packet_free(&element->pkt);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }

    SDL_UnlockMutex(q->mutex);
    return ret;
}

static void packet_queue_flush(PacketQueue *q) {
    MyPacketElement elem;
    SDL_LockMutex(q->mutex);
    while (av_fifo_read(q->pkts, &elem, 1) > 0) {
        av_packet_free(&elem.pkt);
    }
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;

    SDL_UnlockMutex(q->mutex);
}

static void packet_queue_destroy(PacketQueue *q) {
    packet_queue_flush(q);
    av_fifo_freep2(&q->pkts);
    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond);
}

#endif