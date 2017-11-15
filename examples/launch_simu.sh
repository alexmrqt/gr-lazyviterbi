#!/bin/sh
export LD_LIBRARY_PATH=$PWD/../build/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$PWD/../build/swig:$PYTHONPATH
/usr/bin/python2 $1

