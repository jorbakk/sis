</$objtype/mkfile

## FIXME checking function type signatures with -T leads to linker errors 'incompatible type signatures ...'
CFLAGS=-p -I. -I3rd-party/ -I/sys/include/npe -I/sys/include/npe/SDL2 -D__plan9__ -D__${objtype}__ -DNO_THREADING -DSW_ONLY
OBJ=build/sis.$O stbimg.$O algorithm.$O get_opt.$O
LDLIBS=-lnpe_sdl2 -lnpe

all:V: build/sis build/sisui
clean:V:
	rm -f build/main.$O build/mainui.$O $OBJ 3rd-party/nanovg_xc.$O build/sis build/sisui

build/%.$O: %.c
	$CC $CFLAGS -o $target $prereq

%.$O: %.c
	$CC $CFLAGS -o $target $prereq

build/sis: build/main.$O $OBJ
	$LD -o $target $prereq $LDLIBS

build/sisui: build/mainui.$O $OBJ 3rd-party/nanovg_xc.$O
	$LD -o $target $prereq $LDLIBS

