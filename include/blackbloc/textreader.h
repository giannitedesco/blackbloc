/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef _TEXTREADER_HEADER_INCLUDED_
#define _TEXTREADER_HEADER_INCLUDED_

typedef struct _textreader *textreader_t;

textreader_t textreader_new(const char *ptr, size_t len);
char *textreader_gets(textreader_t txt);
void textreader_free(textreader_t txt);

#endif /* _TEXTREADER_HEADER_INCLUDED_ */
