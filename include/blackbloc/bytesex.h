#ifndef __BYTESEX_H
#define __BYTESEX_H

static inline _constfn uint16_t bswap16(uint16_t x)
{
	return ((uint16_t)(
		(((uint16_t)(x) & (uint16_t)0x00ffU) << 8) |
	 	(((uint16_t)(x) & (uint16_t)0xff00U) >> 8) ));
}

static inline _constfn uint32_t bswap32(uint32_t x)
{
	return ((uint32_t)(
		(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |
		(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |
		(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |
		(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24) ));
}

static inline _constfn float bswapf(float x)
{
	union {
		float f;
		unsigned char b[4];
	}dat1, dat2;

	dat1.f = x;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];

	return dat2.f;
}


#if __BYTE_ORDER == __BIG_ENDIAN
#define le_float(x) bswapf(x)
#define be_float(x) (x)
#define le_32(x) bswap32(x)
#define le_16(x) bswap16(x)
#define be_32(x) (x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define le_float(x) (x)
#define be_float(x) bswapf(x)
#define le_32(x) (x)
#define le_16(x) (x)
#define be_32(x) bswap32(x)
#define be_16(x) bswap16(x)
#else
#error "What in hells name?!"
#endif

#endif /* __BYTESEX_H */
