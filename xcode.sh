#!/bin/sh
BASEDIR=`dirname $0`
mkdir -p $BASEDIR/project
pushd $BASEDIR/project >/dev/null
cmake -GXcode ..
popd >/dev/null
