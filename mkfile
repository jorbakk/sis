</$objtype/mkfile

## FIXME checking function type signatures with -T leads to linker errors 'incompatible type signatures ...'
RES_PREFIX="/lib/sis"
CFLAGS=-p -I. -I3rd-party/ -I/sys/include/npe -I/sys/include/npe/SDL2 -D__plan9__ -D__${objtype}__ -DNO_THREADING -DSW_ONLY -DRES_PREFIX=$RES_PREFIX
B = build
BIN=/$objtype/bin
OBJ=sis.$O stbimg.$O algorithm.$O get_opt.$O 3rd-party/cwalk.$O
TARG = $B/sis $B/sisui
LDLIBS=-lnpe_sdl2 -lnpe
ASSETDIR = /lib/sis/assets
DEPTHDIR = /lib/sis/depthmaps
TEXDIR = /lib/sis/textures

all:V: $B $TARG

$B:V:
	mkdir -p $B
clean:V:
	rm -f main.$O mainui.$O $OBJ 3rd-party/cwalk.$O 3rd-party/nanovg_xc.$O $B/sis $B/sisui

%.$O: %.c
	$CC $CFLAGS -o $target $prereq

$B/sis: main.$O $OBJ
	$LD -o $target $prereq $LDLIBS

$B/sisui: mainui.$O $OBJ 3rd-party/nanovg_xc.$O
	$LD -o $target $prereq $LDLIBS

install: all
	for (i in $TARG)
		cp $i $BIN
	mkdir -p $ASSETDIR $DEPTHDIR $TEXDIR
	dircp assets $ASSETDIR
	dircp depthmaps $DEPTHDIR
	dircp textures $TEXDIR
	cp doc/sis.1 /sys/man/1/sis

uninstall:
	for (i in $TARG)
		rm -f $BIN/`{basename $i}
	rm -rf $ASSETDIR $DEPTHDIR $TEXDIR
	rm -f /sys/man/1/sis
	rm -rf /lib/sis
