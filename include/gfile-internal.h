/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef _GFILE_INTERNAL_HEADER_INCLUDED_
#define _GFILE_INTERNAL_HEADER_INCLUDED_

#define GFILE_MAGIC 0xdead1337

struct gfile_blk {
	uint16_t b_type;
	uint16_t b_id;
	uint32_t b_ofs;
	uint32_t b_len;
	uint32_t b_flags;
}_packed;

struct gfile_hdr {
	uint32_t h_magic;
	uint32_t h_num_blk;
	struct gfile_blk h_blk[0];
}_packed;

/* Align all records to 8 bytes */
#define GFILE_ALIGN_BITS 3UL
#define GFILE_ALIGN (1UL << GFILE_ALIGN_BITS)
#define GFILE_ALIGN_MASK (GFILE_ALIGN - 1UL)
#define GFILE_LEN_MASK (~GFILE_ALIGN_MASK)
#define ALIGN_GFILE(x) ({__typeof__(x) _fa_x = (x); \
			(_fa_x + GFILE_ALIGN_MASK) & GFILE_LEN_MASK;})

/* block types and ID's */
#define GFILE_TYPE_BLOB		0 /* Files are concatenated in to a blob */
#define  GFILE_BLOB_FILES	0
#define GFILE_TYPE_NIDX		1 /* name index */
#define  GFILE_NIDX_FILENAMES	0

struct gfile_name {
	/** Offset of file in to record block. */
	uint32_t n_ofs;
	/** Length of this file. */
	uint32_t n_len;
	/** Offset of filename in to string block. */
	uint32_t n_name;
	/** Length of the file-name. */
	uint16_t n_nlen;
	/** Reserved for mode flags. */
	uint16_t n_mode;
}_packed;

struct gfile_nidx {
	/** Number of names in index. */
	uint32_t i_num_names;
	/** Size of string table for names. */
	uint32_t i_strtab_sz;
	/** name array follows idx header */
	struct gfile_name i_name[0];
}_packed;

#endif /* _GFILE_INTERNAL_HEADER_INCLUDED_ */
