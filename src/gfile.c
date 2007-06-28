/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Game file API, hides different game archive formats.
* Only QuakeII PAK files are supported currently.
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <blackbloc.h>
#include <gfile.h>
#include <q2pak.h>

static const char *game_root = "/home/scara/Development/blackbloc/data";
static void direct_close(struct gfile *f);
static int direct_open(struct gfile *f, const char *fn);
static struct gfile_ops direct_ops={
	.close = direct_close,
	.open = direct_open,
	.type_name = "direct",
	.type_desc= "Direct Filesystem Access",
};

static void direct_close(struct gfile *f)
{
	free(f->priv);
	munmap(f->data, f->len);
}

static int direct_open(struct gfile *f, const char *fn)
{
	size_t grlen = strlen(game_root);
	size_t len = grlen + strlen(fn) + 2;
	struct stat st;
	char *map;
	char *buf;
	int fd;

	/* TODO: Detect and strip .. path components */
	buf = malloc(len);
	if ( buf == NULL ) {
		con_printf("%s: malloc(): %s\n", fn, get_err());
		goto err;
	}

	snprintf(buf, len, "%s/%s", game_root, fn);

	fd = open(buf, O_RDONLY);
	if ( fd < 0 ) {
		if ( errno != ENOENT )
			con_printf("%s: open(): %s\n", buf, get_err());
		goto err_free;
	}

	if ( fstat(fd, &st) ) {
		con_printf("%s: fstat(): %s\n", buf, get_err());
		goto err_close;
	}

	map = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if ( map == MAP_FAILED ) {
		con_printf("%s: mmap(): %s\n", buf, get_err());
		goto err_close;
	}

	close(fd);

	f->data = map;
	f->len = st.st_size;
	f->name = buf + grlen + 1;
	f->ops = &direct_ops;
	f->priv = buf;

	return 0;
err_close:
	close(fd);
err_free:
	free(buf);
err:
	return -1;
}

/* Open a game file */
int gfile_open(struct gfile *f, const char *name)
{
	/* Strip leading slashes */
	while ( *name == '/' )
		name++;

	if ( *name == '\0' )
		return -1;

	if ( !direct_open(f, name) )
		goto success;

	if ( !q2pak_open(f, name) )
		goto success;

	return -1;
success:
	f->ofs = 0;
	return 0;
}

/* Close a game file */
void gfile_close(struct gfile *f)
{
	if ( !f->ops->close )
		return;

	f->ops->close(f);
}
