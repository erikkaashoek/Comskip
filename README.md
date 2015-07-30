## comskip

commercial detector
http://www.kaashoek.com/comskip/

### building

#### windows

```
$ mingw32-make CC=mingw32-gcc INCLUDES="-I./ffmpeg/include" LIBS="-L./ffmpeg/lib"
$ comskip.exe
```

#### macosx

```
$ brew install argtable ffmpeg
$ make
$ ./comskip
```

#### linux

```
$ apt-get install libargtable2-dev libavformat-dev
$ make
$ ./comskip
```
