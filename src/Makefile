all: squeezerw
CFLAGS = -g
LDFLAGS =

.c.o:
	cc $(CFLAGS) -c $<

squeezerw: squeezerw.o squeezer.o maxrects.o imageops.o lodepng.o
	cc -o squeezerw squeezerw.o squeezer.o maxrects.o imageops.o lodepng.o $(LDFLAGS)

clean:
	rm -f squeezerw *.o
