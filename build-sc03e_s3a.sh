#!/bin/bash


device=sc03e

export CM_BUILDTYPE="KBC"

. build/envsetup.sh
export USE_CCACHE=1
cd device/samsung/$device/
./extract-files.sh ./proprietary

croot
time brunch $device

