## Process this file with automake to produce Makefile.in

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
	hud.c \
	\
	cl_main.c \
	cl_move.c \
	cl_cmd.c \
	\
	img_pcx.c \
	img_tga.c \
	img_png.c \
	q2wal.c \
	\
	md2.c \
	md5anim.c \
	md5mesh.c \
	\
	q2bsp.c \
	\
	vector.c \
	gfile.c \
	main.c
