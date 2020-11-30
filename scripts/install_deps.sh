#!/bin/bash -ex

THIS=`pwd`/$0
if [ ! -f ${THIS} ]; then
  THIS=$0
fi

ROOT=`dirname ${THIS}`/../deps
BUILD=${ROOT}/build
PREFIX=${ROOT}/release

install_deps() {
  source /etc/os-release
  if [ $NAME = "Ubuntu" ]; then
    sudo -E apt install -y build-essential cmake wget
    sudo -E apt install -y yasm libsdl2-dev libx264-dev libx265-dev
  else
    sudo -E yum groupinstall "Development Tools"
    sudo -E yum -y wget
  fi
}

install_yasm() {
  cd ${BUILD}

  wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz
  tar zxvf yasm-1.3.0.tar.gz

  cd yasm-1.3.0

  ./configure --prefix=${PREFIX}
  make -j
  make install
}

install_srt(){
  cd ${BUILD}

  if [ ! -e srt ];then
    git clone https://github.com/Haivision/srt.git
  fi
  cd srt
  git reset --hard v1.4.1

  ./configure --prefix=${PREFIX} --enable-shared=1 --enable-static=0
  make -j
  make install
}

install_ffmpeg(){
  local VERSION="4.1.3"
  local DIR="ffmpeg-${VERSION}"
  local SRC="${DIR}.tar.bz2"
  local SRC_URL="http://ffmpeg.org/releases/${SRC}"

  export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH

  cd ${BUILD}

  if [ ! -e ${SRC} ];then
    wget ${SRC_URL}
  fi

  if [ ! -e ${DIR} ];then
    tar xf ${SRC}
  fi

  cd ${DIR}
  ./configure --prefix=${PREFIX} --enable-shared --disable-static --disable-vaapi --enable-gpl --enable-libsrt --enable-libx264 --enable-libx265

  make -j
  make install
}

mkdir -p ${BUILD}

install_deps

#install_yasm

install_srt
install_ffmpeg

