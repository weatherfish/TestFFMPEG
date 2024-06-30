#!/bin/zsh

clang -g -o extra_audio extra_audio.c `pkg-config --libs --cflags libavutil libavformat libavcodec`
# clang -g -o extra_video extra_video.c `pkg-config --libs --cflags libavutil libavformat libavcodec`