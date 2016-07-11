import sys
import os
import re
import subprocess

def executeCmd(cmd):
    try:
        out = subprocess.check_output(cmd, shell=True)
    except subprocess.CalledProcessError as e:
        return (e.returncode, e.output)
    return (0, out)

if __name__ == "__main__":
    left = 0
    index = 1
    top = 256
    appendFlags = ' -trim '
    while left < 1024:
        executeCmd('convert grassland_tiles.png -crop 64x64+{}+{} {} images/{}.png'.format(left, top, appendFlags, index))
        left += 64
        index += 1
    left = 0
    top += 64
    while left < 1024:
        executeCmd('convert grassland_tiles.png -crop 64x64+{}+{} {} images/{}.png'.format(left, top, appendFlags, index))
        left += 64
        index += 1
    left = 0
    top += 64
    while left < 1024:
        executeCmd('convert grassland_tiles.png -crop 64x96+{}+{} {} images/{}.png'.format(left, top, appendFlags, index))
        left += 64
        index += 1
