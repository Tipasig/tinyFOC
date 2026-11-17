#!/bin/sh

DST=./library
mkdir -p $DST
rm release.log
rm $DST/libtinyfoc*.lib

echo "compiling libtinyfoc_Cortex-M0+_ARMCC.lib"
make clean 1>>release.log 2>>release.log
make CPU=Cortex-M0+ 1>>release.log 2>>release.log
cp build/libtinyfoc_Cortex-M0+_ARMCC.lib $DST
cp build/libtinyfoc_Cortex-M0+_ARMCC.lib $DSTlibtinyfoc_Cortex-M0_ARMCC.lib

echo "compiling libtinyfoc_Cortex-M3_ARMCC.lib"
make clean 1>>release.log 2>>release.log
make CPU=Cortex-M3 1>>release.log 2>>release.log
cp build/libtinyfoc_Cortex-M3_ARMCC.lib $DST

echo "compiling libtinyfoc_Cortex-M4_ARMCC.lib"
make clean 1>>release.log 2>>release.log
make CPU=Cortex-M4 1>>release.log 2>>release.log
cp build/libtinyfoc_Cortex-M4_ARMCC.lib $DST

echo "releasing to hunter"
cp src/tinyfoc_api.h $DST
arm-none-eabi-strip -S $DSTlibtinyfoc_Cortex-M0_ARMCC.lib
arm-none-eabi-strip -S $DSTlibtinyfoc_Cortex-M0+_ARMCC.lib
arm-none-eabi-strip -S $DSTlibtinyfoc_Cortex-M3_ARMCC.lib
arm-none-eabi-strip -S $DSTlibtinyfoc_Cortex-M4_ARMCC.lib

