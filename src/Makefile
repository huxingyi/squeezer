all: squeezerw
CFLAGS = -g `pkg-config --cflags MagickWand`
LDFLAGS = `pkg-config --libs MagickWand`

.c.o:
	cc $(CFLAGS) -c $<

squeezerw: squeezerw.o squeezer.o maxrects.o
	cc -o squeezerw squeezerw.o squeezer.o maxrects.o $(LDFLAGS)

clean:
	rm -f squeezerw *.o
