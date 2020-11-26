#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>

#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

char errbuff[500];
char* ff_err2str(int errRet) {
  av_strerror(errRet, (char*)(&errbuff), 500);
  return errbuff;
}

void usage(int argc, char* argv[]) {
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

  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "hi:")) != -1)
    switch (c) {
      case 'i':
        i_input_file = optarg;
        break;
      case 'h':
        usage(argc, argv);
        break;
      case '?':
        printf("Unknown option `-%c'.\n", optopt);
        usage(argc, argv);
        break;
      default:
        usage(argc, argv);
    }

  if (i_input_file.empty()) {
    printf("Invalid parameter, no input file\n");

    usage(argc, argv);
  }

  if (access(i_input_file.c_str(), F_OK) != 0) {
    printf("Invalid parameter, input file not exist, %s\n",
           i_input_file.c_str());

    usage(argc, argv);
  }

  if (optind >= argc) {
    printf("Invalid parameter, no output url\n");

    usage(argc, argv);
  } else {
    i_output_url = argv[optind];
  }

  int ret;

  AVFormatContext* inputContext = nullptr;
  AVStream* inputVideoStream = nullptr;
  AVStream* inputAudioStream = nullptr;

  int inputVideoStreamIndex = -1;
  int inputAudioStreamIndex = -1;

  AVFormatContext* outputContext = nullptr;
  AVStream* outputVideoStream = nullptr;
  AVStream* outputAudioStream = nullptr;

  std::string format = "mpegts";

  int64_t output_start_ts = AV_NOPTS_VALUE;
  int64_t output_video_frames = 0;

  // open input
  inputContext = avformat_alloc_context();
  ret = avformat_open_input(&inputContext, i_input_file.c_str(), nullptr,
                            nullptr);
  if (ret != 0) {
    printf("Error opening input %s\n", ff_err2str(ret));

    goto end;
  }

  inputContext->fps_probe_size = 0;
  inputContext->max_ts_probe = 0;
  ret = avformat_find_stream_info(inputContext, NULL);
  if (ret < 0) {
    printf("Error finding stream info %s\n", ff_err2str(ret));

    goto end;
  }

  av_dump_format(inputContext, 0, i_input_file.c_str(), 0);

  // find video stream
  inputVideoStreamIndex =
      av_find_best_stream(inputContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (inputVideoStreamIndex < 0) {
    printf("No video stream\n");
  } else {
    inputVideoStream = inputContext->streams[inputVideoStreamIndex];
  }

  // find audio stream
  inputAudioStreamIndex =
      av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
  if (inputAudioStreamIndex < 0) {
    printf("No audio stream\n");
  } else {
    inputAudioStream = inputContext->streams[inputAudioStreamIndex];
  }

  if (!inputVideoStream && !inputAudioStream) {
    printf("Error, no stream found\n");

    ret = AVERROR_UNKNOWN;
    goto end;
  }

  // open output
  avformat_alloc_output_context2(&outputContext, nullptr, format.c_str(),
                                 i_output_url.c_str());
  if (!outputContext) {
    printf("Cannot allocate output context, %s, format(%s)\n",
           i_output_url.c_str(), format.c_str());

    ret = AVERROR_UNKNOWN;
    goto end;
  }

  if (!(outputContext->oformat->flags & AVFMT_NOFILE)) {
    ret = avio_open(&outputContext->pb, outputContext->url, AVIO_FLAG_WRITE);
    if (ret < 0) {
      printf("Cannot open avio, %s\n", ff_err2str(ret));
      goto end;
    }
  }

  // output video stream
  if (inputVideoStream) {
    outputVideoStream = avformat_new_stream(outputContext, NULL);
    if (!outputVideoStream) {
      printf("Failed allocating output stream\n");

      ret = AVERROR_UNKNOWN;
      goto end;
    }

    ret = avcodec_parameters_copy(outputVideoStream->codecpar,
                                  inputVideoStream->codecpar);
    if (ret < 0) {
      printf("Failed to copy codec parameters\n");

      ret = AVERROR_UNKNOWN;
      goto end;
    }
    outputVideoStream->codecpar->codec_tag = 0;
  }

  // output audio stream
  if (inputAudioStream) {
    outputAudioStream = avformat_new_stream(outputContext, NULL);
    if (!outputAudioStream) {
      printf("Failed allocating output stream\n");

      ret = AVERROR_UNKNOWN;
      goto end;
    }

    ret = avcodec_parameters_copy(outputAudioStream->codecpar,
                                  inputAudioStream->codecpar);
    if (ret < 0) {
      printf("Failed to copy codec parameters\n");

      ret = AVERROR_UNKNOWN;
      goto end;
    }
    outputAudioStream->codecpar->codec_tag = 0;
  }

  av_dump_format(outputContext, 0, i_output_url.c_str(), 1);

  ret = avformat_write_header(outputContext, NULL);
  if (ret < 0) {
    printf("Error occurred when opening output file\n");

    goto end;
  }

  while (1) {
    AVStream* inStream = nullptr;
    AVStream* outStream = nullptr;
    AVPacket pkt;

    ret = av_read_frame(inputContext, &pkt);
    if (ret < 0)
      break;

    if (inputAudioStream && outputAudioStream &&
        inputAudioStream->index == pkt.stream_index) {
      inStream = inputAudioStream;
      outStream = outputAudioStream;
    } else if (inputVideoStream && outputVideoStream &&
               inputVideoStream->index == pkt.stream_index) {
      inStream = inputVideoStream;
      outStream = outputVideoStream;
    } else {
      av_packet_unref(&pkt);
      continue;
    }

    /* copy packet */
    pkt.pts = av_rescale_q_rnd(
        pkt.pts, inStream->time_base, outStream->time_base,
        (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.dts = av_rescale_q_rnd(
        pkt.dts, inStream->time_base, outStream->time_base,
        (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.duration =
        av_rescale_q(pkt.duration, inStream->time_base, outStream->time_base);
    pkt.pos = -1;

    if (output_start_ts == AV_NOPTS_VALUE) {
      output_start_ts = av_gettime_relative();
    } else {
      int64_t pts = av_rescale_q_rnd(
          pkt.dts, outStream->time_base, AV_TIME_BASE_Q,
          (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
      int64_t now = av_gettime_relative() - output_start_ts;

      if (pts > now)
        usleep(pts - now);
    }

    ret = av_interleaved_write_frame(outputContext, &pkt);
    if (ret < 0) {
      printf("Error muxing packet\n");
      goto end;
    }
    av_packet_unref(&pkt);

    if (outStream == outputVideoStream) {
      output_video_frames++;
      if (output_video_frames % 100 == 0)
        printf("Write video frames %ld\n", output_video_frames);
    }
  }

  av_write_trailer(outputContext);
  printf("Done\n");

end:
  if (inputContext)
    avformat_close_input(&inputContext);

  /* close output */
  if (outputContext && !(outputContext->flags & AVFMT_NOFILE))
    avio_closep(&outputContext->pb);
  avformat_free_context(outputContext);

  if (ret < 0 && ret != AVERROR_EOF) {
    fprintf(stderr, "Error occurred: %s\n", ff_err2str(ret));
    return 1;
  }

  return 0;
}
