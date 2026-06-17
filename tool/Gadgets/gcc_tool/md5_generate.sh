#!/bin/bash

SOURCE_BIN="$1"
SOURCE_DIR=`dirname $SOURCE_BIN`
SOURCE_BASE_NAME=`basename  $SOURCE_BIN`

SOURCE_NAME=`echo $SOURCE_BASE_NAME|cut -d '.' -f1`
VERSION_FILE="$2"
echo $SOURCE_BIN
MD5_STR=`md5sum.exe $SOURCE_BIN | cut -c 1-32`
#echo $MD5_STR

MAJOR=$(awk '/^#define[[:space:]]+VERSION_MAJOR[[:space:]]+/ {print $3}' "$VERSION_FILE")
MINOR=$(awk '/^#define[[:space:]]+VERSION_MINOR[[:space:]]+/ {print $3}' "$VERSION_FILE")
REVISION=$(awk   '/^#define[[:space:]]+VERSION_REVISION[[:space:]]+/ {print $3}' "$VERSION_FILE")
BUILDNUM=$(awk '/^#define[[:space:]]+VERSION_BUILDNUM[[:space:]]+/ {print $3}' "$VERSION_FILE")
GCID=$(awk '/^#define[[:space:]]+VERSION_GCID[[:space:]]+/ {print $3}' "$VERSION_FILE"  | sed 's/^0[xX]//')
#version="${MAJOR}.${MINOR}.${REVISION}.${BUILDNUM}-${GCID}"
#echo "$version"
# echo $MAJOR
# echo $MINOR
# echo $REVISION
# echo $BUILDNUM
# echo $GCID
# echo $CUSTOMER_NAME

IMAGE_NAME=$SOURCE_DIR/$SOURCE_NAME-v$MAJOR.$MINOR.$REVISION.$BUILDNUM-$GCID-$MD5_STR.bin
echo $IMAGE_NAME
mv $SOURCE_BIN $IMAGE_NAME
#rm -f "./bin/"$1".bin"
#git checkout $VERSION_FILE
