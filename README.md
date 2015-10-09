## Comskip

Commercial detector
http://www.kaashoek.com/comskip/

### Requirements

- FFmpeg with headers and shared libraries (2.4+)
- libargtable2 for option parsing
- Optional: SDL for building the Comskip GUI

### Building

#### Building from GitHub

Building directly from GitHub also requires GNU autotools (autoconf, automake, and libtool).

```
$ git clone git://github.com/erikkaashoek/Comskip
$ cd Comskip
$ ./autogen.sh
$ ./configure
$ make
```

#### Building from a tarball

```
$ tar zxpfv comskip-<version>.tar.gz
$ cd comskip-<version>
$ ./configure
$ make
```

### Setting up a build environment

#### Windows

There are many ways to setup a build system on Windows. These instructions use MSYS2 and MinGW-w64, but other environments should also work:

- Install MSYS2 by following the instructions at https://msys2.github.io/
- Install the essential build tools: `pacman -S mingw-w64-i686-gcc autoconf automake libtool pkgconfig`
- Close the MSYS2 shell and launch a MinGW-w64 shell; this is the shell you'll use for all builds
- Build FFmpeg (http://ffmpeg.org/) and argtable2 (http://argtable.sourceforge.net/) from source and install them to /usr/local
- Add /usr/local/lib/pkg-config to PKG_CONFIG_PATH (e.g., add `export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig` to ~/.bashrc, then type `source ~/.bashrc`)

#### Mac OS X

First install Xcode (freely available from the Mac OS X App Store). After it's installed, install the Xcode command-line tools by executing `xcode-select --install` from a terminal.

The easiest way to install Comskip's dependencies is via Homebrew (http://brew.sh/):

```
$ brew install autoconf automake libtool pkgconfig argtable ffmpeg sdl
```

#### Linux

##### Ubuntu Vivid (15.04)

```
$ apt-get install -y git build-essential libargtable2-dev libavformat-ffmpeg-dev libsdl1.2-dev
```

##### Ubuntu Trusty (14.04)

```
$ add-apt-repository -y ppa:mc3man/trusty-media
$ apt-get update

$ apt-get install -y git build-essential libargtable2-dev libsdl1.2-dev
$ apt-get install -y ffmpeg libva-dev libsoxr-dev libvorbis-dev libbz2-dev zlib1g-dev libxvidcore-dev libvpx-dev libx264-dev libx265-dev libspeex-dev libfdk-aac-dev libvorbisenc2 libopus-dev libmp3lame-dev libdca-dev libfaac-dev libopencore-amrnb-dev libvo-aacenc-dev libopencore-amrwb-dev

$ git clone https://github.com/foo86/dcadec
$ cd dcadec
$ make install
```

##### Ubuntu Precise (12.04)

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
```
