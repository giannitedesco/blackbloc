## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I../include

bin_PROGRAMS = blackbloc

#blackbloc_svr_LDADD = @MATHLIB@
#blackbloc_svr_SOURCES = \
#	sv_spawn.c \
#	sv_main.c \
#	sv_net.c \
#	vector.c \
#	netbuf.c \
#	gfile.c \
#	q2pak.c \
#	md2.c

blackbloc_LDADD = -lpng @MATHLIB@ @SDL_LIBS@
blackbloc_SOURCES = \
	md2_render.c \
	gl_render.c \
	sdl_mouse.c \
	sdl_keyb.c \
	teximage.c \
	particle.c \
	cl_main.c \
	cl_move.c \
	gl_init.c \
	img_pcx.c \
	img_png.c \
	vector.c \
	cl_cmd.c \
	q2pak.c \
	gfile.c \
	main.c \
	hud.c \
	md2.c

#	netbuf.c
#	cl_net.c
