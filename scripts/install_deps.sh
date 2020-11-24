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
    sudo -E apt install -y libsdl2-dev
  else
    sudo -E yum groupinstall "Development Tools"
    sudo -E yum -y boost-devel wget
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

  git clone https://github.com/Haivision/srt.git
  cd srt
  git checkout v1.4.1

  ./configure --prefix=${PREFIX} --enable-shared=1 --enable-static=1
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

  wget ${SRC_URL}
  rm -rf ${DIR}
  tar xf ${SRC}

  cd ${DIR}
  ./configure --prefix=${PREFIX} --enable-shared --enable-static --disable-vaapi --enable-gpl --enable-libsrt

  make -j
  make install
}

mkdir -p ${BUILD}

install_deps

install_yasm

install_srt
install_ffmpeg

