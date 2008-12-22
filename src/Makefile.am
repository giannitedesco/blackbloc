## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I../include

bin_PROGRAMS = blackbloc mkgfile

mkgfile_SOURCES = \
	mkgfile.c \
	mpool.c \
	gang.c

blackbloc_LDADD = -lpng @MATHLIB@ @SDL_LIBS@
blackbloc_SOURCES = \
	md2_render.c \
	gl_render.c \
	sdl_mouse.c \
	sdl_keyb.c \
	teximage.c \
	md5anim.c \
	md5mesh.c \
	cl_main.c \
	cl_move.c \
	img_pcx.c \
	img_png.c \
	vector.c \
	cl_cmd.c \
	q2bsp.c \
	q2wal.c \
	gfile.c \
	main.c \
	hud.c \
	md2.c
