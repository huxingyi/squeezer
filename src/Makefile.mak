cc=cl.exe
link=link.exe

all: squeezerw.exe

squeezerw.exe: maxrects.obj squeezer.obj squeezerw.obj lodepng.obj
  $(link) -out:squeezerw.exe $**

clean:
  del squeezerw.exe maxrects.obj squeezer.obj squeezerw.obj lodepng.obj
