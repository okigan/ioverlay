#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

int main(int argc, char **argv) {
    int ret;

    AVFilterGraph *graph = avfilter_graph_alloc();

    const char *filtergraph =
        "movie@name1=/home/igor/ioverlay/testfiles/org_seg_720p0.ts,buffersink";
    ret = avfilter_graph_parse_ptr(graph, filtergraph, NULL, NULL, NULL);
    printf("%s\n", av_err2str(ret));

    ret = avfilter_graph_config(graph, NULL);
    printf("%s\n", av_err2str(ret));

    char *xx = avfilter_graph_dump(graph, "");
    printf("%s\n", xx);
    av_free(xx);

    AVFormatContext *ofmt_ctx = NULL;
    char *filename = "./out.mp4";
    ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
    printf("%s\n", av_err2str(ret));

    AVCodec *enc = avcodec_find_encoder_by_name("libx264");
    AVCodecContext *enc_ctx = avcodec_alloc_context3(enc);

    enc_ctx->width = 1280;
    enc_ctx->height = 720;
    enc_ctx->sample_aspect_ratio.num = 1;
    enc_ctx->sample_aspect_ratio.den = 1;
    enc_ctx->pix_fmt = enc->pix_fmts[0];
    enc_ctx->time_base.num = 1;
    enc_ctx->time_base.den = 1;

    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        ofmt_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(enc_ctx, enc, NULL);
    printf("%s\n", av_err2str(ret));

    AVStream *out_stream = avformat_new_stream(ofmt_ctx, enc);
    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    printf("%s\n", av_err2str(ret));

    ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
    printf("%s\n", av_err2str(ret));

    ret = avformat_write_header(ofmt_ctx, NULL);
    printf("%s\n", av_err2str(ret));

    int done_with_frames = 0;
    for (int i = 0;; i++) {
        AVFrame *frame = NULL;
        if (done_with_frames == 0) {
            frame = av_frame_alloc();
            ret = av_buffersink_get_frame_flags(graph->filters[1], frame, 0);
            printf("%s\n", av_err2str(ret));
            if (ret == AVERROR_EOF) {
                done_with_frames = 1;
            }
            if (ret != 0) {
                av_frame_unref(frame);
                av_frame_free(&frame);
            }

            if (frame != NULL) {
                printf("%d -- %ld\n", i, frame->pkt_pos);
            }
        }
        
        ret = avcodec_send_frame(enc_ctx, frame);
        printf("%s\n", av_err2str(ret));

        AVPacket enc_pkt;
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_receive_packet(enc_ctx, &enc_pkt);
        printf("%s\n", av_err2str(ret));
        if (ret == 0) {
            ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
            printf("%s\n", av_err2str(ret));
        }
        av_frame_unref(frame);
        av_frame_free(&frame);
        if (ret == AVERROR_EOF) {
            break;
        }
    }

    // TODO: flush encoder
    printf("%s\n", av_err2str(ret));

    ret = av_write_trailer(ofmt_ctx);
    printf("%s\n", av_err2str(ret));

    avcodec_free_context(&enc_ctx);
    avformat_free_context(ofmt_ctx);

    avfilter_graph_free(&graph);
}
