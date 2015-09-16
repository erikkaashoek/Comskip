## comskip

commercial detector
http://www.kaashoek.com/comskip/

### building

#### windows

```
$ git clone git://github.com/erikkaashoek/Comskip
$ cd Comskip
$ mingw32-make CC=mingw32-gcc INCLUDES="-I./ffmpeg/include" LIBS="-L./ffmpeg/lib"

$ comskip.exe
```

#### macosx

```
$ brew install argtable ffmpeg

$ git clone git://github.com/erikkaashoek/Comskip
$ cd Comskip
$ make

$ ./comskip
```

#### linux

##### ubuntu 15.04

```
$ apt-get install -y git
$ git clone git://github.com/erikkaashoek/Comskip
$ cd Comskip

$ apt-get install -y build-essential libargtable2-dev libavformat-ffmpeg-dev
$ make

$ ./comskip
```

##### ubuntu 14.04

```
$ add-apt-repository -y ppa:mc3man/trusty-media
$ apt-get update
$ apt-get install -y git
$ apt-get install -y build-essential libargtable2-dev
$ apt-get install -y ffmpeg libva-dev libsoxr-dev libvorbis-dev libbz2-dev zlib1g-dev libxvidcore-dev libvpx-dev libx264-dev libx265-dev libspeex-dev libfdk-aac-dev libvorbisenc2 libopus-dev libmp3lame-dev libdca-dev libfaac-dev libopencore-amrnb-dev libvo-aacenc-dev libopencore-amrwb-dev

$ git clone https://github.com/foo86/dcadec
$ cd dcadec
$ make install
$ cd ..

$ git clone git://github.com/erikkaashoek/Comskip
$ cd Comskip
$ make INCLUDES=-I/opt/ffmpeg/include LIBS="-L/opt/ffmpeg/lib -lavutil -lavformat -lavcodec -ldl -lva -lswscale -lswresample -lsoxr -lvorbis -lbz2 -lz -lxvidcore -lvpx -lx264 -lx265 -lspeex -lfdk-aac -lvorbisenc -lopus -lmp3lame -ldca -lfaac -lopencore-amrnb -lvo-aacenc -lopencore-amrwb -ldcadec"

$ ./comskip
```
