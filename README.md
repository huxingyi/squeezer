Overview <img src="https://img.shields.io/github/license/mashape/apistatus.svg?maxAge=2592000" width="122" height="20"/> [![Build Status](https://travis-ci.org/huxingyi/squeezer.svg?branch=master)](https://travis-ci.org/huxingyi/squeezer) [![Build status](https://ci.appveyor.com/api/projects/status/h9vckwyht5e0pj87/branch/master?svg=true)](https://ci.appveyor.com/project/huxingyi/squeezer/branch/master)
------------
**Texture Packer** for Game Development Using MaxRects Algorithm.  

<img src="https://raw.githubusercontent.com/huxingyi/squeezer/master/example/squeezer.png" width="512" height="256"/>  

*Note: The game assets used in this example were download from [Grassland Tileset](http://opengameart.org/content/grassland-tileset), thanks `Clint Bellanger` for made so many awesome game resources.*  

Contributing
--------------


Building(Linux/OSX)
---------------
```sh
$ cd ./src
$ make
```

Building(Windows)
--------------
```sh
$ cd .\src
$ nmake -f Makefile.mak
```

Usage
--------------
```sh
usage: squeezerw [options] <sprite image dir>
    options:
        --width <output width>
        --height <output height>
        --allowRotations <1/0/true/false/yes/no>
        --border <1/0/true/false/yes/no>
        --outputTexture <output texture filename>
        --outputInfo <output sprite info filename>
        --infoHeader <output header template>
        --infoBody <output body template>
        --infoSplit <output body split template>
        --infoFooter <output footer template>
        --verbose
        --version
    format specifiers of infoHeader/infoBody/infoFooter:
        %W: output width
        %H: output height
        %n: image name
        %w: image width
        %h: image height
        %x: left on output image
        %y: top on output image
        %l: trim offset left
        %t: trim offset top
        %c: original width
        %r: original height
        %f: 1 if rotated else 0
        and '\n', '\r', '\t'
```

Example
------------
```sh
$ cd ./src
$ make

$ ./squeezerw ../example/images --verbose --width 512 --height 256 --border 1 --outputTexture ../example/squeezer.png --outputInfo ../example/squeezer.xml

$ ./squeezerw ../example/images --verbose --width 512 --height 256 --border 1 --outputTexture ../example/squeezer.png --outputInfo ../example/squeezer.json --infoHeader "{\"textureWidth\":\"%W\", \"textureHeight\":\"%H\", \"items\":[\n" --infoFooter "]}" --infoBody "{\"name\":\"%n\", \"width\":\"%w\", \"height\":\"%h\", \"left\":\"%x\", \"top\":\"%y\", \"rotated\":\"%f\", \"trimOffsetLeft\":\"%l\", \"trimOffsetTop\":\"%t\", \"originWidth\":\"%c\", \"originHeight\":\"%r\"}" --infoSplit "\n,"
```

Licensing
-----------------
Licensed under the MIT license except lodepng.c and lodepng.h.  

*Note: [lodepng](https://github.com/lvandeve/lodepng) used in this project to decode and encode png format. For the license used in lodepng please visit [lodepng](https://github.com/lvandeve/lodepng) directly.*

References
------------
1. http://www.blackpawn.com/texts/lightmaps/  
2. http://clb.demon.fi/projects/more-rectangle-bin-packing  
3. http://wiki.unity3d.com/index.php?title=MaxRectsBinPack  
4. https://github.com/juj/RectangleBinPack  
5. http://opengameart.org/content/flare-weapon-icons-2  
