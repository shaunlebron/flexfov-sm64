#ifndef _FLEXFOV_H
#define _FLEXFOV_H

#include "include/types.h" // import Vec4f, Vec3f, Mat4

#include "include/PR/gbi.h" // import Vp, Light_t
#include "src/engine/graph_node.h" // import GraphNodeRoot, Gfx, GraphNodePerspective

extern u8 flexFovSky;

u8 flexfov_is_on(void);
void flexfov_set_cam(Vec4f *m);
void flexfov_run_prehook(Gfx *cmd);
void flexfov_gfx_init(void);
void flexfov_geo_process_root(struct GraphNodeRoot *root, Vp *b, Vp *c, s32 clearColor);
void flexfov_resize_cubemap(void);
void flexfov_set_light_direction(Light_t *light);
void flexfov_set_fog_scale(float m[4][4], float v[3], float *z, float *w);
void flexfov_set_fog_planes(struct GraphNodePerspective *node);
void flexfov_update_input(void);
void flexfov_mtxf_cylboard(Mat4 dest, Mat4 src, Vec3f pos, Vec3f cam);
void flexfov_mtxf_sphereboard(Mat4 dest, Mat4 src, Vec3f pos);

#endif // _FLEXFOV_H
