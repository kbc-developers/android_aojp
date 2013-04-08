#!/bin/bash

#device=t0lte
device=sc02e

. build/envsetup.sh
export USE_CCACHE=1
cd device/samsung/$device/
./extract-files.sh ./proprietary
#./extract-files.sh /home/ma34s/work/SC02E/cm-10.1-20130301-NIGHTLY-t0lte

croot
time brunch $device


