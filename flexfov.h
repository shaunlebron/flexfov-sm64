#ifndef _FLEXFOV_H
#define _FLEXFOV_H

#include "include/types.h" // import Vec3f

#include "include/PR/gbi.h" // import Vp, Light_t
#include "src/engine/graph_node.h" // import GraphNodeRoot, Gfx

extern u8 flexFovSky;
extern s16 flexFovRoll;

u8 flexfov_is_on(void);
void flexfov_set_cam(Vec4f *m);
void flexfov_run_prehook(Gfx *cmd);
void flexfov_gfx_init(void);
void flexfov_geo_process_root(struct GraphNodeRoot *root, Vp *b, Vp *c, s32 clearColor);
void flexfov_resize_cubemap(void);
void flexfov_set_light_direction(Light_t *light);
void flexfov_update_input(void);

#endif // _FLEXFOV_H
