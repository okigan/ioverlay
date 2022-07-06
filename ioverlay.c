#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define RET_ON_ERR(x) \
    // printf("%s\n", av_err2str(ret)); \
    {   \
        if (x < 0) {    \
        }   \
    }

static void avlog_cb(void *v, int level, const char *szFmt, va_list varg) {
    // do nothing...
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("%s <source> <overlay> <output>\n", argv[0]);
        return 1;
    }

    const char *source = argv[1];
    const char *overlay = argv[2];
    const char *output = argv[3];

    int ret;

    av_log_set_callback(avlog_cb);

    AVFilterGraph *graph = avfilter_graph_alloc();

    char filtergraph[4096] = "";
    const char *template =
        "movie@name1=%s [base];"
        "movie@name1=%s [image];"
        "[base] [image]overlay [o];"
        "[o]buffersink";
    snprintf(filtergraph, ARRAY_SIZE(filtergraph), template, source, overlay);

    ret = avfilter_graph_parse_ptr(graph, filtergraph, NULL, NULL, NULL);
    RET_ON_ERR(ret)

    ret = avfilter_graph_config(graph, NULL);
    RET_ON_ERR(ret)

    // char *xx = avfilter_graph_dump(graph, "");
    // printf("%s\n", xx);
    // av_free(xx);

    AVFilterContext *graph_output_filter = graph->filters[3];

    AVFormatContext *ofmt_ctx = NULL;
    const char *filename = output;
    ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
    RET_ON_ERR(ret)

    AVCodec *enc = avcodec_find_encoder_by_name("libx264");
    AVCodecContext *enc_ctx = avcodec_alloc_context3(enc);

    enc_ctx->width = av_buffersink_get_w(graph_output_filter);
    enc_ctx->height = av_buffersink_get_h(graph_output_filter);
    enc_ctx->sample_aspect_ratio.num = 1;
    enc_ctx->sample_aspect_ratio.den = 1;
    enc_ctx->pix_fmt = enc->pix_fmts[0];
    enc_ctx->time_base.num = 1;
    enc_ctx->time_base.den = 1;

    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        ofmt_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(enc_ctx, enc, NULL);
    RET_ON_ERR(ret)

    AVStream *out_stream = avformat_new_stream(ofmt_ctx, enc);
    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    RET_ON_ERR(ret)

    ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
    RET_ON_ERR(ret)

    ret = avformat_write_header(ofmt_ctx, NULL);
    RET_ON_ERR(ret)

    int done_with_frames = 0;
    for (int i = 0;; i++) {
        AVFrame *frame = NULL;
        if (done_with_frames == 0) {
            frame = av_frame_alloc();
            ret = av_buffersink_get_frame_flags(graph_output_filter, frame, 0);
            RET_ON_ERR(ret)
            if (ret == AVERROR_EOF) {
                done_with_frames = 1;
            }
            if (ret != 0) {
                av_frame_unref(frame);
                av_frame_free(&frame);
            }

            if (frame != NULL) {
                // printf("%d -- %ld\n", i, frame->pkt_pos);
            }
        }

        ret = avcodec_send_frame(enc_ctx, frame);
        RET_ON_ERR(ret)

        AVPacket enc_pkt;
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_receive_packet(enc_ctx, &enc_pkt);
        RET_ON_ERR(ret)
        if (ret == 0) {
            ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
            RET_ON_ERR(ret)
        }
        av_frame_unref(frame);
        av_frame_free(&frame);
        if (ret == AVERROR_EOF) {
            break;
        }
    }

    ret = av_write_trailer(ofmt_ctx);
    RET_ON_ERR(ret)

    avcodec_free_context(&enc_ctx);
    avformat_free_context(ofmt_ctx);

    avfilter_graph_free(&graph);
}
