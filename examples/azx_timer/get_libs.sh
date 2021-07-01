#!/bin/bash

LIBS=`cat azx.in`

DEST=`readlink -f $0 | xargs dirname`
ROOT=`readlink -f "$DEST/../../"`
SRC="${ROOT}/azx"

if [ -n "$1" ]
then
  SRC="$1"
fi

if [ -n "$2" ]
then
  DEST="$2"
fi

if [ -n "$3" ]
then
  LIBS_FOLDER_NAME="$3"
else
  LIBS_FOLDER_NAME="azx"
fi

echo "Sourcing ${SRC}/import_libs.script"
source "${SRC}/import_libs.script"
echo "Copy ${SRC} to ${DEST}"
copy_telit_libs "${SRC}" "${DEST}" "${LIBS_FOLDER_NAME}"
