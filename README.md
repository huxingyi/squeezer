Overview <img src="https://img.shields.io/github/license/mashape/apistatus.svg?maxAge=2592000" width="122" height="20"/> [![Build Status](https://travis-ci.org/huxingyi/squeezer.svg?branch=master)](https://travis-ci.org/huxingyi/squeezer)
------------
**Texture Packer** for Game Development Using MaxRects Algorithm.  

<img src="https://raw.githubusercontent.com/huxingyi/squeezer/master/example/squeezer.png" width="512" height="256"/>  

*Note: The game assets used in this example were download from [Grassland Tileset](http://opengameart.org/content/grassland-tileset), thanks `Clint Bellanger` for made so many awesome game resources.*  

Building
---------------
```sh
# Install ImageMagick via `brew install imagemagick` on Mac or visit http://imagemagick.org

$ cd ./src
$ export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
$ make
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
        --output <output filename>
        --verbose
        --version
```

Example
------------
```sh
$ cd ./src
$ export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
$ make
$ ./squeezerw ../example/images --verbose --width 512 --height 256 --border 1 --output ../example/squeezer.png
```

Licensing
-----------------
Licensed under the MIT license.  

References
------------
1. http://www.blackpawn.com/texts/lightmaps/  
2. http://clb.demon.fi/projects/more-rectangle-bin-packing  
3. http://wiki.unity3d.com/index.php?title=MaxRectsBinPack  
4. https://github.com/juj/RectangleBinPack  
5. http://opengameart.org/content/flare-weapon-icons-2  
