#!/usr/bin/env bash
# Copyright RBoy, licensed under GPL version 3
# set -x

echo "Starting building of Comskip components..."

echo "Building ffmpeg, this may take about 4GB of free space"
if [[ ! -d patches || ! -f cross_compile_ffmpeg.sh ]]; then
  echo "ERROR: Cannot find ffmpeg build script files"
  exit 1
fi
unset CFLAGS
unset LDFLAGS
unset PKG_CONFIG_PATH
./cross_compile_ffmpeg.sh --disable-nonfree=y --build-intel-qsv=y --build-amd-amf=y --compiler-flavors=win32 --git-get-latest=n || exit 1
cc_path="sandbox/cross_compilers/mingw-w64-i686"

echo "Building Argtable2"
wget https://downloads.sourceforge.net/project/argtable/argtable/argtable-2.13/argtable2-13.tar.gz || exit 1
tar -xf argtable2-13.tar.gz || exit 1
cd argtable2-13
export CFLAGS='-I../$cc_path/i686-w64-mingw32/include'
./configure --host=i686-w64-mingw32 --prefix=$PWD/../$cc_path/i686-w64-mingw32 || exit 1
make CC=$PWD/../$cc_path/bin/i686-w64-mingw32-gcc AR=$PWD/../$cc_path/bin/i686-w64-mingw32-ar PREFIX=$PWD/../$cc_path/i686-w64-mingw32 RANLIB=$PWD/../$cc_path/bin/i686-w64-mingw32-ranlib LD=$PWD/../$cc_path/bin/i686-w64-mingw32-ld STRIP=$PWD/../$cc_path/bin/i686-w64-mingw32-strip CXX=$PWD/../$cc_path/bin/i686-w64-mingw32-g++ || exit 1
cd ..

echo "Building Comskip"
org_path=$PATH # Save the current path
export PATH="/home/comskip/Comskip/sandbox/mingw-w64-i686/bin:$PATH"
export PKG_CONFIG_PATH='./$cc_path/i686-w64-mingw32/lib/pkgconfig:./argtable2-13'
export CFLAGS='-I./$cc_path/i686-w64-mingw32/include -I./argtable2-13/src'
export LDFLAGS='-L./$cc_path/i686-w64-mingw32/lib -L./argtable2-13/src/.libs'
./autogen.sh || exit 1
./configure --host=i686-w64-mingw32 --prefix=$PWD/$cc_path/i686-w64-mingw32 || exit 1
make clean
make CC=$PWD/$cc_path/bin/i686-w64-mingw32-gcc AR=$PWD/$cc_path/bin/i686-w64-mingw32-ar PREFIX=$PWD/$cc_path/i686-w64-mingw32 RANLIB=$PWD/$cc_path/bin/i686-w64-mingw32-ranlib LD=$PWD/$cc_path/bin/i686-w64-mingw32-ld STRIP=$PWD/$cc_path/bin/i686-w64-mingw32-strip CXX=$PWD/$cc_path/bin/i686-w64-mingw32-g++ || exit 1
./$cc_path/bin/i686-w64-mingw32-strip -s comskip || exit 1
export PATH=$org_path # Restore original path
mv comskip comskip.exe # rename it to something Windows can use
if [[ ! -f comskip.exe ]]; then
  echo "ERROR: Comskip did not build"
  exit 1
else
  echo "Comskip.exe was built successfully!!"
fi
