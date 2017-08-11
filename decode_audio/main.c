//
//  main.c
//  decode_audio
//
//  Created by cuifei on 2017/7/15.
//  Copyright © 2017年 cuifei. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libavutil/frame.h"
#include "libavutil/mem.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

static void decode(AVCodecContext *dec_ctx,AVPacket *packet,AVFrame *frame, FILE *outfile){
    
    int i,ch;
    int ret, data_size;
    
    ret = avcodec_send_packet(dec_ctx, packet);
    if (ret < 0) {
        fprintf(stderr, "Error submitting the packet to the decoder\n");
        exit(1);
    }
    
    
    while (ret >=0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        }else if(ret < 0){
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        
        data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        if (data_size < 0) {
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }
        
        for (i =0; i< frame->nb_samples; i++) {
            for (ch = 0; ch < dec_ctx->channels; ch++) {
                fwrite(frame->data[ch]+data_size*i, 1, data_size, outfile);
            }
        }
    }
    
    
}



int main(int argc, const char * argv[]) {
    // insert code here...
//    const char *inputFileName ="/Users/cuifei/Documents/te.mov";
    const char *inputFileName ="/Users/edz/Documents/testvideo/sintel.ts";
    const char *outfileName = "/Users/edz/Documents/testvideo/test.aac";
    const AVCodec *codec;
    AVCodecContext *c = NULL;
    AVCodecContext *enCodecContext = NULL;
    AVCodecParserContext *parser = NULL;
    
    
    int len,ret;
    
    FILE *f, *outfile;
    
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t data_size;
    
    AVPacket *packet;
    AVFrame *decoded_frame = NULL;
    AVFormatContext *avFormatContext = NULL;
    AVFormatContext *outputFormatContext = NULL;
    AVOutputFormat *avOutputFormat = NULL;
    AVStream *audio_st = NULL;
    AVPacket avPacket ;
    AVFrame *avFrame = NULL;
    
    
    
    av_register_all();
    packet = av_packet_alloc();
    if (!packet) {
        printf("error alloc packet\n");
        exit(1);
    }
    avFormatContext = avformat_alloc_context();
    
    ret = avformat_open_input(&avFormatContext, inputFileName, NULL, NULL);
    if (ret <0) {
        printf("av_open_input_file error\n");
    }
    
    outputFormatContext = avformat_alloc_context();
    
    avOutputFormat = av_guess_format(NULL, outfileName, NULL);
    outputFormatContext->oformat = avOutputFormat;
    ret = avformat_find_stream_info(avFormatContext,NULL);
    if (avio_open(&outputFormatContext->pb, outfileName, AVIO_FLAG_READ_WRITE)<0) {
        printf("Failed to open output file!\n");
        exit(1);
    }
    if (ret < 0) {
        printf("av_find_stream_info error\n");
    }
    int i;
    int audioIndex = -1;
    for (i =0; i< avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioIndex = i;
        }
    }
    if( audioIndex ==-1){
        printf("Didn't find a video stream.\n");
        exit(1);
    }
    
    codec = avcodec_find_decoder(avFormatContext->streams[audioIndex]->codec->codec_id);
    
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }
    
//    parser = av_parser_init(codec->id);
//    if (!parser) {
//        fprintf(stderr, "Parser not found\n");
//        exit(1);
//    }
    
    c = avcodec_alloc_context3(codec);
    
    enCodecContext = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }
   enCodecContext->bit_rate = c->bit_rate = avFormatContext->streams[audioIndex]->codec->bit_rate;
    c->sample_rate = avFormatContext->streams[audioIndex]->codec->sample_rate;
    enCodecContext->channels = c->channels = avFormatContext->streams[audioIndex]->codec->channels;
     enCodecContext->block_align = c->block_align = avFormatContext->streams[audioIndex]->codec->block_align;
    enCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
    enCodecContext->frame_size = avFormatContext->streams[audioIndex]->codec->frame_size;
    enCodecContext->sample_fmt = c->sample_fmt = avFormatContext->streams[audioIndex]->codec->sample_fmt;
    outfile = fopen(outfileName, "wb");

      av_dump_format(outputFormatContext, 0, outfileName, 1);
   
    if (avcodec_open2(c, codec, NULL)< 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
    if (avcodec_open2(enCodecContext, codec, NULL)< 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
//    f = fopen(inputFileName, "rb");
//    if (!f) {
//        fprintf(stderr, "Could not open %s\n", inputFileName);
//        exit(1);
//    }
    if (!decoded_frame) {
        if (!(decoded_frame = av_frame_alloc())) {
            fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
        }
                }
       if (!outfile) {
        av_free(c);
        exit(1);
    }
    audio_st = avformat_new_stream(outputFormatContext, 0);
    int got_frame;
    int size = av_samples_get_buffer_size(NULL, enCodecContext->channels, enCodecContext->frame_size, enCodecContext->sample_fmt, 1);
    printf("size= %d\n",&size);
    uint8_t *frame_buf = (uint8_t *)av_malloc(size);
    av_new_packet(&avPacket, size);
   
    avformat_write_header(outputFormatContext, NULL);

    int count = 0;
    while (av_read_frame(avFormatContext, packet) == 0) {
        if (packet->stream_index == audioIndex) {
            //            decode(c, packet, decoded_frame, outfile);
            data_size = av_get_bytes_per_sample(c->sample_fmt);
            if (data_size < 0) {
                fprintf(stderr, "Failed to calculate data size\n");
                exit(1);
            }
           
//            packet->stream_index = audio_st->index;
//            av_write_frame(outputFormatContext, packet);
            
            len = avcodec_decode_audio4(c, decoded_frame, &got_frame, packet);
                printf("===len=%d\n",&len);
             printf("===got_frame=%d\n",&got_frame);
                if (got_frame) {
                    int i;
                    int ch;
                    fprintf(stderr, "nb_samples=: %d",decoded_frame->nb_samples);
            
                    
                    for (i =0; i< decoded_frame->nb_samples; i++) {
                       

                        for (ch = 0; ch < c->channels; ch++) {
                            fwrite(decoded_frame->data[ch]+data_size*i, 1, data_size, outfile);
                        }
                    }
                    

                }

        }
    }
    av_write_trailer(outputFormatContext);
    
//    data = inbuf;
//    data_size = fread(inbuf, 1, AUDIO_INBUF_SIZE, f);
//    while (data_size> 0) {
//        if (!decoded_frame) {
//            if (!(decoded_frame = av_frame_alloc())) {
//                fprintf(stderr, "Could not allocate audio frame\n");
//                exit(1);
//            }
//        }
//        ret = av_parser_parse2(parser, c, &packet->data, &packet->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
//        if (ret < 0) {
//            fprintf(stderr, "Error while parsing\n");
//            exit(1);
//        }
//        
//        data +=ret;
//        data_size -= ret;
//        
//        if (packet->size) {
//            printf("=====%d",packet->size);
//            decode(c, packet, decoded_frame, outfile);
//        }
//        
//        if (data_size < AUDIO_REFILL_THRESH) {
//            memmove(inbuf, data, data_size);
//            data = inbuf;
//            len = fread(data + data_size, 1, AUDIO_INBUF_SIZE - data_size, f);
//            
//            if (len > 0) {
//                data_size += len;
//            }
//        }
//        
//    }
//    
//    packet->data = NULL;
//    packet->size = 0;
//    decode(c, packet, decoded_frame, outfile);
    fclose(outfile);
//    fclose(f);
    
    avcodec_free_context(&c);
    av_parser_close(parser);
    av_frame_free(&decoded_frame);
    
    av_packet_free(&packet);
    avformat_free_context(avFormatContext);
    
    
    return 0;
//static void decode(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame,
//                   FILE *outfile)
//{
//    int i, ch;
//    int ret, data_size;
//    /* send the packet with the compressed data to the decoder */
//    ret = avcodec_send_packet(dec_ctx, pkt);
//    if (ret < 0) {
//        fprintf(stderr, "Error submitting the packet to the decoder\n");
//        exit(1);
//    }
//    /* read all the output frames (in general there may be any number of them */
//    while (ret >= 0) {
//        ret = avcodec_receive_frame(dec_ctx, frame);
//        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
//            return;
//        else if (ret < 0) {
//            fprintf(stderr, "Error during decoding\n");
//            exit(1);
//        }
//        data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
//        if (data_size < 0) {
//            /* This should not occur, checking just for paranoia */
//            fprintf(stderr, "Failed to calculate data size\n");
//            exit(1);
//        }
//        for (i = 0; i < frame->nb_samples; i++)
//            for (ch = 0; ch < dec_ctx->channels; ch++)
//                fwrite(frame->data[ch] + data_size*i, 1, data_size, outfile);
//    }
//}
//int main(int argc, char **argv)
//{
//    const char *outfilename, *filename;
//    const AVCodec *codec;
//    AVCodecContext *c= NULL;
//    AVCodecParserContext *parser = NULL;
//    int len, ret;
//    FILE *f, *outfile;
//    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
//    uint8_t *data;
//    size_t   data_size;
//    AVPacket *pkt;
//    AVFrame *decoded_frame = NULL;
////    if (argc <= 2) {
////        fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
////        exit(0);
////    }
//    filename    = "/Users/cuifei/Documents/te.mov";
//    outfilename = "/Users/cuifei/Documents/test.pcm";
//    /* register all the codecs */
//    avcodec_register_all();
//    pkt = av_packet_alloc();
//    /* find the MPEG audio decoder */
//    codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
//    if (!codec) {
//        fprintf(stderr, "Codec not found\n");
//        exit(1);
//    }
//    parser = av_parser_init(codec->id);
//    if (!parser) {
//        fprintf(stderr, "Parser not found\n");
//        exit(1);
//    }
//    c = avcodec_alloc_context3(codec);
//    if (!c) {
//        fprintf(stderr, "Could not allocate audio codec context\n");
//        exit(1);
//    }
//    /* open it */
//    if (avcodec_open2(c, codec, NULL) < 0) {
//        fprintf(stderr, "Could not open codec\n");
//        exit(1);
//    }
//    f = fopen(filename, "rb");
//    if (!f) {
//        fprintf(stderr, "Could not open %s\n", filename);
//        exit(1);
//    }
//    outfile = fopen(outfilename, "wb");
//    if (!outfile) {
//        av_free(c);
//        exit(1);
//    }
//    /* decode until eof */
//    data      = inbuf;
//    data_size = fread(inbuf, 1, AUDIO_INBUF_SIZE, f);
//    while (data_size > 0) {
//        if (!decoded_frame) {
//            if (!(decoded_frame = av_frame_alloc())) {
//                fprintf(stderr, "Could not allocate audio frame\n");
//                exit(1);
//            }
//        }
//        ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
//                               data, data_size,
//                               AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
//        if (ret < 0) {
//            fprintf(stderr, "Error while parsing\n");
//            exit(1);
//        }
//        data      += ret;
//        data_size -= ret;
//        if (pkt->size)
//            decode(c, pkt, decoded_frame, outfile);
//        if (data_size < AUDIO_REFILL_THRESH) {
//            memmove(inbuf, data, data_size);
//            data = inbuf;
//            len = fread(data + data_size, 1,
//                        AUDIO_INBUF_SIZE - data_size, f);
//            if (len > 0)
//                data_size += len;
//        }
//    }
//    /* flush the decoder */
//    pkt->data = NULL;
//    pkt->size = 0;
//    decode(c, pkt, decoded_frame, outfile);
//    fclose(outfile);
//    fclose(f);
//    avcodec_free_context(&c);
//    av_parser_close(parser);
//    av_frame_free(&decoded_frame);
//    av_packet_free(&pkt);
//    return 0;
}









































