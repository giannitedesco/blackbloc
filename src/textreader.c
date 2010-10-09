/*
* This file is part of blackbloc
* Copyright (c) 2008 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#include <blackbloc/blackbloc.h>
#include <blackbloc/textreader.h>
#include <stdio.h>

struct _textreader {
	const char *txt_ptr;
	const char *txt_end;
	char *txt_retbuf;
	size_t txt_buflen;
};

textreader_t textreader_new(const char *ptr, size_t len)
{
	struct _textreader *txt;

	txt = calloc(1, sizeof(*txt));
	if ( NULL == txt )
		return NULL;

	txt->txt_ptr = ptr;
	txt->txt_end = ptr + len;
	return txt;
}

static void to_buf(struct _textreader *txt, const char *tmp)
{
	size_t len = tmp - txt->txt_ptr;

	if ( len + 1 > txt->txt_buflen ) {
		free(txt->txt_retbuf);
		txt->txt_buflen = len + 1;
		txt->txt_retbuf = malloc(len + 1);
		if ( NULL == txt->txt_retbuf ) {
			txt->txt_ptr = txt->txt_end;
			return;
		}
	}

	snprintf(txt->txt_retbuf, txt->txt_buflen,
		 "%.*s", len, txt->txt_ptr);
}

char *textreader_gets(textreader_t txt)
{
	const char *tmp;

	if ( txt->txt_ptr >= txt->txt_end ) {
		free(txt->txt_retbuf);
		txt->txt_retbuf = NULL;
		return NULL;
	}

	for(tmp = txt->txt_ptr; tmp < txt->txt_end; tmp++) {
		if ( *tmp == '\r' || *tmp == '\n' ) {
			to_buf(txt, tmp);

			txt->txt_ptr = tmp + 1;
			if ( tmp + 1 < txt->txt_end &&
					tmp[0] == '\r' && tmp[1] == '\n' )
				txt->txt_ptr++;
			return txt->txt_retbuf;
		}
	}

	/* lack of trailing line-break */
	to_buf(txt, tmp);
	txt->txt_ptr = tmp + 1;
	return txt->txt_retbuf;
}

void textreader_free(textreader_t txt)
{
	free(txt->txt_retbuf);
	free(txt);
}
