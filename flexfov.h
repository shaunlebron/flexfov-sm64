#ifndef _FLEXFOV_H
#define _FLEXFOV_H

#include "include/types.h" // import Vec3f

#include "include/PR/gbi.h" // import Vp, Light_t
#include "src/engine/graph_node.h" // import GraphNodeRoot, Gfx

extern u8 flexFovSky;
extern u8 flexFovSide;
extern s16 flexFovRoll;

enum FLEXFOV_CUBE_SIDE {
  FLEXFOV_CUBE_FRONT,
  FLEXFOV_CUBE_LEFT,
  FLEXFOV_CUBE_RIGHT,
  FLEXFOV_CUBE_BACK,
  FLEXFOV_CUBE_UP,
  FLEXFOV_CUBE_DOWN
};

void flexfov_set_cam(Vec4f *m);
u8 flexfov_is_on(void);
void flexfov_run_prehook(Gfx *cmd);
void flexfov_gfx_init(void);
void flexfov_render_world(struct GraphNodeRoot *root, Vp *b, Vp *c, s32 clearColor);
void flexfov_resize_cubemap(void);
void flexfov_set_light_direction(Light_t *light);
void flexfov_update_input(void);

#endif // _FLEXFOV_H
