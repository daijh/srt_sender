#!/bin/bash
THIS=`pwd`/$BASH_SOURCE
if [ ! -f ${THIS} ]; then
  THIS=$BASH_SOURCE
fi
ROOT=`dirname ${THIS}`/..

export MANPATH=${ROOT}/deps/release/share/man:$MANPATH
export LD_LIBRARY_PATH=${ROOT}/deps/release/lib:$LD_LIBRARY_PATH
export PATH=${ROOT}/deps/release/bin:$PATH

