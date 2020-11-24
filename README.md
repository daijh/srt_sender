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

# help
# Usage: srt_sender  -i input_file  output_url
./build/srt_sender -h

# <ip> is local ip, set <port> to 20000 for test
./build/srt_sender -i test.mp4  srt://<ip>:<port>?mode=listener


# remote
# check if srt supported
ffmpeg -protocols | grep srt

# playback
ffplay srt://<ip>:<port>?mode=caller
```

## Useful links
- [SRT Protocol Technical Overview](https://github.com/Haivision/srt)
- [SRT Cookbook](https://srtlab.github.io/srt-cookbook/apps/ffmpeg/)
- [FFmpeg SRT](https://ffmpeg.org/ffmpeg-protocols.html#srt)

