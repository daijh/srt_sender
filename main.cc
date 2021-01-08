#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#include <string>

void Usage(int argc, char* argv[]) {
  printf("\n");
  printf("Usage: %s  -i input_file  output_url\n", argv[0]);

  printf("Options:\n");
  printf("    -h  help\n");
  printf("    -i  input file\n");

  exit(1);
}

int main(int argc, char** argv) {
  std::string i_input_file;
  std::string i_output_url;

  int c = -1;
  opterr = 0;
  while ((c = getopt(argc, argv, "hi:")) != -1) {
    switch (c) {
      case 'i':
        i_input_file = optarg;
        break;
      case 'h':
        Usage(argc, argv);
        break;
      case '?':
        printf("Unknown option `-%c'.\n", optopt);
        Usage(argc, argv);
        break;
      default:
        Usage(argc, argv);
    }
  }

  if (i_input_file.empty()) {
    printf("Invalid parameter, no input file\n");

    Usage(argc, argv);
  }

  if (access(i_input_file.c_str(), F_OK) != 0) {
    printf("Invalid parameter, input file not exist, %s\n",
           i_input_file.c_str());

    Usage(argc, argv);
  }

  if (optind >= argc) {
    printf("Invalid parameter, no output url\n");

    Usage(argc, argv);
  } else {
    i_output_url = argv[optind];
  }

  // av_log_set_level(AV_LOG_DEBUG);

  int ret = 0;

  AVFormatContext* input_context = nullptr;
  AVStream* input_video_stream = nullptr;
  AVStream* input_audio_stream = nullptr;

  int input_video_streamIndex = -1;
  int input_audio_streamIndex = -1;

  AVFormatContext* output_context = nullptr;
  AVStream* output_video_stream = nullptr;
  AVStream* output_audio_stream = nullptr;

  std::string format = "mpegts";

  int64_t output_start_ts = AV_NOPTS_VALUE;
  int64_t output_video_frames = 0;

  // open input
  input_context = avformat_alloc_context();
  ret = avformat_open_input(&input_context, i_input_file.c_str(), nullptr,
                            nullptr);
  if (ret != 0) {
    printf("Error opening input\n");

    goto end;
  }

  input_context->fps_probe_size = 0;
  input_context->max_ts_probe = 0;
  ret = avformat_find_stream_info(input_context, NULL);
  if (ret < 0) {
    printf("Error finding stream info\n");

    goto end;
  }

  av_dump_format(input_context, 0, i_input_file.c_str(), 0);

  // find video stream
  input_video_streamIndex =
      av_find_best_stream(input_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (input_video_streamIndex < 0) {
    printf("No video stream\n");
  } else {
    input_video_stream = input_context->streams[input_video_streamIndex];
  }

  // find audio stream
  input_audio_streamIndex =
      av_find_best_stream(input_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
  if (input_audio_streamIndex < 0) {
    printf("No audio stream\n");
  } else {
    input_audio_stream = input_context->streams[input_audio_streamIndex];
  }

  if (!input_video_stream && !input_audio_stream) {
    printf("Error, no stream found\n");

    ret = AVERROR_UNKNOWN;
    goto end;
  }

  // open output
  avformat_alloc_output_context2(&output_context, nullptr, format.c_str(),
                                 i_output_url.c_str());
  if (!output_context) {
    printf("Cannot allocate output context, %s, %s\n",
           i_output_url.c_str(), format.c_str());

    ret = AVERROR_UNKNOWN;
    goto end;
  }

  if (!(output_context->oformat->flags & AVFMT_NOFILE)) {
    ret = avio_open(&output_context->pb, output_context->url, AVIO_FLAG_WRITE);
    if (ret < 0) {
      printf("Cannot open avio\n");
      goto end;
    }
  }

  // output video stream
  if (input_video_stream) {
    output_video_stream = avformat_new_stream(output_context, NULL);
    if (!output_video_stream) {
      printf("Cannot allocate output stream\n");

      ret = AVERROR_UNKNOWN;
      goto end;
    }

    ret = avcodec_parameters_copy(output_video_stream->codecpar,
                                  input_video_stream->codecpar);
    if (ret < 0) {
      printf("Failed to copy codec parameters\n");

      ret = AVERROR_UNKNOWN;
      goto end;
    }
    output_video_stream->codecpar->codec_tag = 0;
  }

  // output audio stream
  if (input_audio_stream) {
    output_audio_stream = avformat_new_stream(output_context, NULL);
    if (!output_audio_stream) {
      printf("Cannot allocate output stream\n");

      ret = AVERROR_UNKNOWN;
      goto end;
    }

    ret = avcodec_parameters_copy(output_audio_stream->codecpar,
                                  input_audio_stream->codecpar);
    if (ret < 0) {
      printf("Failed to copy codec parameters\n");

      ret = AVERROR_UNKNOWN;
      goto end;
    }
    output_audio_stream->codecpar->codec_tag = 0;
  }

  av_dump_format(output_context, 0, i_output_url.c_str(), 1);

  ret = avformat_write_header(output_context, NULL);
  if (ret < 0) {
    printf("Error occurred when opening output file\n");

    goto end;
  }

  while (1) {
    AVStream* in_stream = nullptr;
    AVStream* out_stream = nullptr;
    AVPacket pkt;

    ret = av_read_frame(input_context, &pkt);
    if (ret < 0) break;

    if (input_audio_stream && output_audio_stream &&
        input_audio_stream->index == pkt.stream_index) {
      in_stream = input_audio_stream;
      out_stream = output_audio_stream;
    } else if (input_video_stream && output_video_stream &&
               input_video_stream->index == pkt.stream_index) {
      in_stream = input_video_stream;
      out_stream = output_video_stream;
    } else {
      av_packet_unref(&pkt);
      continue;
    }

    /* copy packet */
    pkt.pts = av_rescale_q_rnd(
        pkt.pts, in_stream->time_base, out_stream->time_base,
        (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.dts = av_rescale_q_rnd(
        pkt.dts, in_stream->time_base, out_stream->time_base,
        (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.duration =
        av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
    pkt.pos = -1;

    if (output_start_ts == AV_NOPTS_VALUE) {
      output_start_ts = av_gettime_relative();
    } else {
      int64_t pts = av_rescale_q_rnd(
          pkt.dts, out_stream->time_base, AV_TIME_BASE_Q,
          (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
      int64_t now = av_gettime_relative() - output_start_ts;

      if (pts > now) usleep(pts - now);
    }

    //int64_t pts_ms = av_rescale_q_rnd(
    //    pkt.pts, out_stream->time_base, av_make_q(1, 1000),
    //    (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    // printf("Write out pts %ld (ms)\n", pts_ms);

    ret = av_interleaved_write_frame(output_context, &pkt);
    if (ret < 0) {
      printf("Error muxing packet\n");
      goto end;
    }
    av_packet_unref(&pkt);

    if (out_stream == output_video_stream) {
      output_video_frames++;
      if (output_video_frames % 100 == 0)
        printf("Write video frames %ld\n", output_video_frames);
    }
  }

  av_write_trailer(output_context);
  printf("Done\n");

end:
  if (input_context) avformat_close_input(&input_context);

  /* close output */
  if (output_context && !(output_context->flags & AVFMT_NOFILE))
    avio_closep(&output_context->pb);
  avformat_free_context(output_context);

  if (ret < 0 && ret != AVERROR_EOF) {
    char error_buffer[128] = {'\0'};
    av_strerror(ret, error_buffer, 128);
    fprintf(stderr, "Error occurred: %s\n", error_buffer);
    return 1;
  }

  return 0;
}
