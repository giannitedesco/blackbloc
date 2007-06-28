/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __GFILE_HEADER_INCLUDED__
#define __GFILE_HEADER_INCLUDED__

struct gfile {
	void *data;
	size_t len;
	char *name;
	struct gfile_ops *ops;
	void *priv;
	size_t ofs;
};

struct gfile_ops {
	void (*close)(struct gfile *);
	int (*open)(struct gfile *, const char *);
	const char *type_name;
	const char *type_desc;
};

int gfile_open(struct gfile *f, const char *name);
void gfile_close(struct gfile *f);

#endif /* __GFILE_HEADER_INCLUDED__ */
