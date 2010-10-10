#ifndef __Q2BSP_HEADER_INCLUDED
#define __Q2BSP_HEADER_INCLUDED

typedef struct _q2bsp *q2bsp_t;

void q2bsp_render(q2bsp_t map);
q2bsp_t q2bsp_load(const char *);
void q2bsp_free(q2bsp_t map);

#endif /* __Q2BSP_HEADER_INCLUDED */
