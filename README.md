# srt_sender
[FFmpeg](https://github.com/FFmpeg/FFmpeg) utility to read media file and send out by [SRT](https://github.com/Haivision/srt)

## Setup
- Install dependancies, FFmpeg and SRT libraries
```bash
./scripts/install_deps.sh
```
- Build
```bash
mkdir build
cd build
cmake ..
make
```

## Run
- The Caller-Listener Handshake <br>
Start SRT *listener* firstly, waiting for connection by *caller*

```bash
# set library path
source ./scripts/setupvars.sh

# Usage: srt_sender  -i input_file  output_url
./build/srt_sender -h

# <ip> is local ip, set <port> to 20000 for test
./build/srt_sender -i test.mp4  srt://<ip>:<port>?mode=listener

# multiple connections
./build/srt_sender -i test.mp4  srt://<ip>:<port-1>?mode=listener & \
./build/srt_sender -i test.mp4  srt://<ip>:<port-2>?mode=listener & \
./build/srt_sender -i test.mp4  srt://<ip>:<port-3>?mode=listener & \
./build/srt_sender -i test.mp4  srt://<ip>:<port-4>?mode=listener

# remote playback
# check if srt supported
ffmpeg -protocols | grep srt

# play
ffplay srt://<ip>:<port>?mode=caller
```

## B-frames
B-frames usually are not used for low latency streaming. Only use non-B-frames clip as the input.

```bash
# check B-frames
ffprobe -show_frames test.mp4 | grep "pict_type"

# transcode to non B-frames h264
ffmpeg -i test.mp4 -acodec copy -vcodec libx264 -x264opts bframes=0:ref=1 non-b-frames-test-h264.mp4

# transcode to non B-frames hevc
ffmpeg -i test.mp4 -acodec copy -vcodec libx265 -x265opts bframes=0:ref=1 non-b-frames-test-hevc.mp4
```

## Useful links
- [SRT Protocol Technical Overview](https://github.com/Haivision/srt)
- [SRT Cookbook](https://srtlab.github.io/srt-cookbook/apps/ffmpeg/)
- [FFmpeg SRT](https://ffmpeg.org/ffmpeg-protocols.html#srt)

