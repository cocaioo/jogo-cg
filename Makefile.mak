CPP    = g++
WINDRES= windres
RM     = rm -f
OBJS   = main.o \
         AppResource.res

LIBS   = -mwindows -static -lglut32 -lglu32 -lopengl32 -lwinmm -lgdi32
CFLAGS = -DGLUT_STATIC

.PHONY: jogo\ cg.exe clean clean-after

all: jogo\ cg.exe

clean:
	$(RM) $(OBJS) jogo\ cg.exe

clean-after:
	$(RM) $(OBJS)

jogo\ cg.exe: $(OBJS)
	$(CPP) -Wall -s -o '$@' $(OBJS) $(LIBS)

main.o: main.cpp stb_image.h
	$(CPP) -Wall -s -c $< -o $@ $(CFLAGS)

AppResource.res: AppResource.rc
	$(WINDRES) -i AppResource.rc -J rc -o AppResource.res -O coff

