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
```bash
# set library path
source ./scripts/setupvars.sh

# help
# Usage: srt_sender  -i input_file  output_url
./build/srt_sender -h

# ${IP} is local ip
./build/srt_sender  -i test.mp4  srt://${IP}:20000?mode=listener


# remote
# check if srt supported
ffmpeg -protocols | grep srt

# playback
ffplay srt://${IP}:20000
```
