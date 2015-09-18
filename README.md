## comskip

commercial detector
http://www.kaashoek.com/comskip/

### requirements

- recent build of ffmpeg with headers and shared libraries (2.4+)
- libargtable2 for option parsing
- optional: sdl for gui on linux/mac

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
$ brew install argtable ffmpeg sdl

$ git clone git://github.com/erikkaashoek/Comskip
$ cd Comskip
$ make

$ ./comskip
```

#### linux

##### ubuntu vivid (15.04)

```
$ apt-get install -y git build-essential libargtable2-dev libavformat-ffmpeg-dev libsdl1.2-dev

$ git clone git://github.com/erikkaashoek/Comskip
$ cd Comskip
$ make

$ ./comskip
```

##### ubuntu trusty (14.04)

```
$ add-apt-repository -y ppa:mc3man/trusty-media
$ apt-get update

$ apt-get install -y git build-essential libargtable2-dev libsdl1.2-dev
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

##### ubuntu precise (12.04)

```
$ add-apt-repository -y ppa:pavlyshko/precise
$ add-apt-repository -y ppa:chris-lea/zeromq
$ apt-get update

$ wget https://launchpad.net/ubuntu/+archive/primary/+files/libfdk-aac0_0.1.1%2B20130514-2_amd64.deb
$ wget https://launchpad.net/ubuntu/+archive/primary/+files/libopus0_1.0.1-0ubuntu2_amd64.deb
$ wget https://launchpadlibrarian.net/205263953/libwebp5_0.4.1-1.2pmo1%7Eprecise_amd64.deb
$ dpkg -i libfdk-aac0_0.1.1+20130514-2_amd64.deb
$ dpkg -i libopus0_1.0.1-0ubuntu2_amd64.deb
$ dpkg -i libwebp5_0.4.1-1.2pmo1~precise_amd64.deb

$ apt-get install -y git build-essential libargtable2-dev libsdl1.2-dev
$ apt-get install -y ffmpeg-opti libavformat-ffmpeg-opti-dev

$ git clone git://github.com/erikkaashoek/Comskip
$ cd Comskip
$ make LIBS="-L/opt/ffmpeg/lib" SHLIBS="-lavutil.ffmpeg -lavformat.ffmpeg -lavcodec.ffmpeg -largtable2"

$ ./comskip
```
