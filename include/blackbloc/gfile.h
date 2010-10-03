/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef _GFILE_HEADER_INCLUDED_
#define _GFILE_HEADER_INCLUDED_

typedef struct _gnode *gnode_t;
typedef struct _gfs *gfs_t;

struct gfile {
	const void *f_ptr;
	size_t f_len;
	const char *f_name;
};

gfs_t gfs_open(const char *filename) _nonull(1) _check_result;
void gfs_close(gfs_t fs) _nonull(1);

int gfile_open(gfs_t fs, struct gfile *f, const char *name)
			_nonull(1,2,3) _check_result;
void gfile_close(gfs_t fs, struct gfile *f)
			_nonull(1);

#endif /* _GFILE_HEADER_INCLUDED_ */
