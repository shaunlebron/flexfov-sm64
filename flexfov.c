#include "flexfov.h"

#include <stdio.h> // import printf

#include "rendering_graph_node.h" // import geo_process_root
#include "src/engine/math_util.h" // import atan2s
#include "include/config.h" // import SCREEN_WIDTH, SCREEN_HEIGHT
#include "src/game/camera.h" // import CAMERA_MODE_INSIDE_CANNON
#include "src/game/game_init.h" // import gDisplayListHead, gPlayer1Controller
#include "include/PR/gu.h" // import guScaleF
#include "include/gfx_dimensions.h" // import GFX_DIMENSIONS_FROM_LEFT_EDGE
#include "src/game/ingame_menu.h" // import create_dl_translation_matrix, MENU_MTX_PUSH
#include "src/pc/gfx/gfx_pc.h" // import gfx_current_dimensions
#include "src/audio/external.h" // import play_sound
#include "include/audio_defines.h" // import SOUND_MENU_MESSAGE_DISAPPEAR, SOUND_MENU_MESSAGE_APPEAR
#include "include/sm64.h" // import ACT_CREDITS_CUTSCENE

//------------------------------------------------------------------------------
// State
//------------------------------------------------------------------------------

static u8 flexFovOn = TRUE;
u8 flexFovSky;
s16 flexFovRoll;

u8 flexFovSide;
enum FLEXFOV_CUBE_SIDE {
  FLEXFOV_CUBE_FRONT,
  FLEXFOV_CUBE_LEFT,
  FLEXFOV_CUBE_RIGHT,
  FLEXFOV_CUBE_BACK,
  FLEXFOV_CUBE_UP,
  FLEXFOV_CUBE_DOWN
};

u8 can_be_on(void) {
  extern struct Object *gMarioObject;
  extern struct MarioState *gMarioState;
  extern struct Area *gCurrentArea;
  u8 marioOnScreen = gMarioObject != NULL;
  u8 marioInCannon = marioOnScreen && gCurrentArea != NULL && gCurrentArea->camera != NULL && gCurrentArea->camera->mode == CAMERA_MODE_INSIDE_CANNON;
  return marioOnScreen && !marioInCannon && gMarioState->action != ACT_CREDITS_CUTSCENE;
}

u8 flexfov_is_on(void) {
  return flexFovOn && can_be_on();
}

// shader state
static u8 useRubix = 0;
static u8 useCube = 0;
static float camPitch = 0;
static float fov = 180.0f;
static float mobiusZoom = -1.0f;
static u8 manualMobiusZoom = FALSE;

float getMobiusZoom(void) {
  if (manualMobiusZoom) {
    return mobiusZoom;
  }
  if (180.0f < fov) {
    float t = (fov - 180.0f)/180.0f;
    return -(1.0f - t);
  }
  return -1.0f;
}

//------------------------------------------------------------------------------
// Controls
//------------------------------------------------------------------------------

static u8 controlsOn = FALSE;
static u8 zooming = FALSE;

// button states
static s16 rCount = 0;
static u8 heldA = 0;
static u8 heldB = 0;

// stick state
static u8 waitingForCenter = FALSE;
static u8 waitingForDecenter = FALSE;

static const float fovOffBound = 90.0f;

void flexfov_update_input(void) {
  if (!can_be_on()) {
    return;
  }

  // get button states
  u8 r = (gPlayer1Controller->buttonDown & R_TRIG) > 0;
  u8 z = (gPlayer1Controller->buttonDown & Z_TRIG) > 0;
  u8 a = (gPlayer1Controller->buttonDown & A_BUTTON) > 0;
  u8 b = (gPlayer1Controller->buttonDown & B_BUTTON) > 0;

  if (!z) zooming = FALSE;
  if (!r) controlsOn = FALSE;

  // Hold R for 5 frames to enable flex fov controls
  // Release R to return normal controls
  if (!controlsOn) {
    rCount = r ? rCount+1 : 0;
    if (rCount > 5) {
      controlsOn = TRUE;
      s32 mode = set_cam_angle(0);
      set_cam_angle(mode == 1 ? 2 : 1);

      if (fov == fovOffBound) {
        fov += 0.1f;
      }
    }
    return;
  }

  // Toggles
  if (a && !heldA) useRubix = !useRubix;
  if (b && !heldB) useCube = !useCube;
  heldA = a;
  heldB = b;

  // Knobs
  float stickX = gPlayer1Controller->stickX / 64.0f;
  if (stickX == 0.0f) {
    waitingForCenter = FALSE;
  }
  if (z) {
    if (!zooming) {
      if (getMobiusZoom() == 0.0f) {
        play_sound(SOUND_MENU_PINCH_MARIO_FACE, gDefaultSoundArgs);
        waitingForDecenter = FALSE;
      } else {
        play_sound(SOUND_MENU_LET_GO_MARIO_FACE, gDefaultSoundArgs);
        waitingForDecenter = TRUE;
      }
    }
    if (waitingForDecenter && stickX != 0.0f) {
      waitingForDecenter = FALSE;
      play_sound(SOUND_MENU_PINCH_MARIO_FACE, gDefaultSoundArgs);
    }
    zooming = TRUE;
    mobiusZoom = stickX;
    manualMobiusZoom = TRUE;
    waitingForCenter = TRUE;
  } else if (!waitingForCenter) {

    // Scroll fov with thumbstick up/down (as units per second)
    //   - 64 is max stick magnitude
    //   - 30 is frame rate
    float scrollUPS = gPlayer1Controller->stickY / 64.0f / 30.0f; // per second
    fov += scrollUPS * 90.0; // 90°/s

    // toggle on/off when crossing 90° boundary
    if (flexFovOn) {
      if (fov < fovOffBound) { flexFovOn = FALSE; play_sound(SOUND_MENU_MESSAGE_DISAPPEAR, gDefaultSoundArgs); }
    } else {
      if (fov > fovOffBound) { flexFovOn = TRUE; play_sound(SOUND_MENU_MESSAGE_APPEAR, gDefaultSoundArgs); }
    }

    // clamp fov
    if (fov < fovOffBound) fov = fovOffBound;
    else if (fov > 360.0f) fov = 360.0f;
    else {
      if (scrollUPS != 0) {
        play_sound(SOUND_MOVING_AIM_CANNON, gDefaultSoundArgs);
        manualMobiusZoom = FALSE;
      }
    }
  }

  // disable mario controls
  gPlayer1Controller->buttonDown    = gPlayer3Controller->buttonDown = 0;
  gPlayer1Controller->buttonPressed = gPlayer3Controller->buttonPressed = 0;
  gPlayer1Controller->stickX        = gPlayer3Controller->stickX = 0;
  gPlayer1Controller->stickY        = gPlayer3Controller->stickY = 0;
  gPlayer1Controller->stickMag      = gPlayer3Controller->stickMag = 0;
}

//------------------------------------------------------------------------------
// Camera
//------------------------------------------------------------------------------

void flexfov_set_cam(Vec4f *m) {
#define R0(i) pR[i]
#define U0(i) pU[i]
#define B0(i) pB[i]
#define R(i) m[i][0]
#define U(i) m[i][1]
#define B(i) m[i][2]
#define VSET(a,b) a(0)=b(0); a(1)=b(1); a(2)=b(2); a(3)=b(3)

  // save current camera to “previous”
  Vec4f pR, pU, pB;
  VSET(R0,R);
  VSET(U0,U);
  VSET(B0,B);

  camPitch = asin(-pB[1]);

  // overwrite camera position
  if (flexFovSide == FLEXFOV_CUBE_FRONT) {
    // default
  } else if (flexFovSide == FLEXFOV_CUBE_LEFT) {
    VSET(R,-B0);
    VSET(B, R0);
  } else if (flexFovSide == FLEXFOV_CUBE_RIGHT) {
    VSET(R, B0);
    VSET(B,-R0);
  } else if (flexFovSide == FLEXFOV_CUBE_BACK) {
    VSET(R,-R0);
    VSET(B,-B0);
  } else if (flexFovSide == FLEXFOV_CUBE_DOWN) {
    VSET(B, U0);
    VSET(U,-B0);
  } else if (flexFovSide == FLEXFOV_CUBE_UP) {
    VSET(B,-U0);
    VSET(U, B0);
  }

  // for billboards
  const f32 PI = 32768.0f; // in s16 angle units
  flexFovRoll = atan2s(R(1), sqrtf(R(0)*R(0) + R(2)*R(2))) - PI / 2.0f;
  if (flexFovRoll == 0 && U(1) < 0) { // upside-down case when atan2 should return 180°
    flexFovRoll = -PI;
  }
}

//------------------------------------------------------------------------------
// cubemap setup and rendering
//------------------------------------------------------------------------------

// from gfx_pc.c
extern void gfx_get_dimensions(uint32_t *width, uint32_t *height);
extern void gfx_flush(void);
extern void gfx_sp_reset(void);

#if FOR_WINDOWS || defined(TARGET_OSX)
#define GLEW_STATIC
#include <OpenGL/gl3.h>
#endif

#include <SDL2/SDL.h>

#define GL_GLEXT_PROTOTYPES 1
#ifdef USE_GLES
#include <SDL2/SDL_opengles2.h>
#else
#include <SDL2/SDL_opengl.h>
#endif

// names
static GLuint frameBuffer;
static GLuint cubeTextureColor;
static GLuint cubeTextureDepth;

static GLenum cubesidesGL[6] = {
  // SM64 and OpenGL texture cube map have same coord systems
  /* FLEXFOV_CUBE_FRONT */  GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
  /* FLEXFOV_CUBE_LEFT  */  GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
  /* FLEXFOV_CUBE_RIGHT */  GL_TEXTURE_CUBE_MAP_POSITIVE_X,
  /* FLEXFOV_CUBE_BACK  */  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
  /* FLEXFOV_CUBE_UP    */  GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
  /* FLEXFOV_CUBE_DOWN  */  GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
};

static void set_tex_params(void) {
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void flexfov_resize_cubemap(void) {
  // TEXTURES
  // texture size
  u32 w, h;
  gfx_get_dimensions(&w, &h);

  // Allocate COLOR textures
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTextureColor);
  u8 i;
  for (i=0; i<6; i++) {
    GLenum cubesideGL = cubesidesGL[i];
    glTexImage2D(cubesideGL, 0, GL_RGBA, h, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    set_tex_params();
  }

  // Allocate DEPTH textures
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTextureDepth);
  for (i=0; i<6; i++) {
    GLenum cubesideGL = cubesidesGL[i];
    glTexImage2D(cubesideGL, 0, GL_DEPTH_COMPONENT24, h, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    set_tex_params();
  }
}

static void create_cubemap(void) {
  // OBJECTS
  // Create frame buffer object
  glGenFramebuffers(1, &frameBuffer);

  // Create cube texture objects
  glGenTextures(1, &cubeTextureColor);
  glGenTextures(1, &cubeTextureDepth);
  flexfov_resize_cubemap();

  // OPTIONS
  // enable seamless global option
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

static void restore_viewport(void) {
  u32 w, h;
  gfx_get_dimensions(&w, &h);

  // opengl
  glViewport(0,0,w,h);
  glScissor(0,0,w,h);

  // rdp
  gfx_current_dimensions.width = w;
  gfx_current_dimensions.height = h;
  gfx_current_dimensions.aspect_ratio = (float)w / (float)h;
}

static void set_cubeside_viewport(void) {
  u32 w, h;
  gfx_get_dimensions(&w, &h);

  glViewport(0,0,h,h);

  // rdp
  gfx_current_dimensions.width = h;
  gfx_current_dimensions.height = h;
  gfx_current_dimensions.aspect_ratio = 1.0f;
}

static u8 currSideGl;

static void init_cubeside(u8 side) {
  currSideGl = side;
  set_cubeside_viewport();

  GLenum cubesideGL = cubesidesGL[side];
	GLenum attachments[2] = {GL_COLOR_ATTACHMENT0, GL_NONE};

  glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cubesideGL, cubeTextureColor, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubesideGL, cubeTextureDepth, 0);
  glDrawBuffers(2, attachments);

  glDisable(GL_SCISSOR_TEST);
  glDepthMask(GL_TRUE); // Must be set to clear Z-buffer
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_SCISSOR_TEST);

  // Objects rendered to FBO will not blend correctly with previous contents without this:
  // https://stackoverflow.com/a/18497511/142317
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void flexfov_set_light_direction(Light_t *light) {
  // Light is always constant relative to the camera angle.
  // So we have to force the same light direction for all cubefaces.

  if (!flexfov_is_on()) return;

  s8 x0=light->dir[0], y0=light->dir[1], z0=light->dir[2];
  s8 x,y,z;

  switch (currSideGl) {
    case FLEXFOV_CUBE_FRONT: x =  x0; y =  y0; z =  z0; break;
    case FLEXFOV_CUBE_LEFT:  x = -z0; y =  y0; z =  x0; break;
    case FLEXFOV_CUBE_RIGHT: x =  z0; y =  y0; z = -x0; break;
    case FLEXFOV_CUBE_BACK:  x = -x0; y =  y0; z = -z0; break;
    case FLEXFOV_CUBE_UP:    x =  x0; y =  z0; z = -y0; break;
    case FLEXFOV_CUBE_DOWN:  x =  x0; y = -z0; z =  y0; break;
  }

  light->dir[0] = x;
  light->dir[1] = y;
  light->dir[2] = z;
}

static float near, far;
void flexfov_set_fog_planes(struct GraphNodePerspective *node) {
  if (!flexfov_is_on()) return;
  near = node->near;
  far = node->far;
}

void flexfov_set_fog_scale(float m[4][4], float v[3], float *z, float *w) {
  // Fog is scaled based on z-distance to the screen’s near plane.
  // For consistency across cubefaces, we scale it based on actual distance to the camera.

  if (!flexfov_is_on()) return;

  // get distance to the camera
  float dx = v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m[2][0] + m[3][0];
  float dy = v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m[2][1] + m[3][1];
  float dz = v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m[2][2] + m[3][2];
  float dist = sqrtf(dx*dx + dy*dy + dz*dz);

  // Get the non-linear normalization of this distance (z/w)
  // by passing the point (0,0,-dist) through the projection matrix.
  //
  //   float u[3] = {0.0f, 0.0f, dist};
  //   x = u[0] * p[0][0] + u[1] * p[1][0] + u[2] * p[2][0] + p[3][0];
  //   y = u[0] * p[0][1] + u[1] * p[1][1] + u[2] * p[2][1] + p[3][1];
  //   z = u[0] * p[0][2] + u[1] * p[1][2] + u[2] * p[2][2] + p[3][2];
  //   w = u[0] * p[0][3] + u[1] * p[1][3] + u[2] * p[2][3] + p[3][3];
  //
  //   This simplifies to the following:
  *z = (-dist*(near+far) + 2*near*far) / (near-far);
  *w = dist;
}

//------------------------------------------------------------------------------
// projection setup and rendering
//------------------------------------------------------------------------------

GLuint quadProg;
GLint quadAttrXY;
GLint quadAttrUV;
GLint quadCamPitch;
GLint quadUseRubix;
GLint quadUseCube;
GLint quadFov;
GLint quadMobiusZoom;
GLint quadControlsOn;
GLint quadZooming;

// Z depths:
//  * sky = -0.33
//  * hud = -1
//  * pause = 0

static GLfloat quadVerts[] = {
// x      y     u     v
// (clip space) (aspect-normalized texture space)
  -1.0f, -1.0f, 0.0f, 0.0f, // bottom left
   1.0f, -1.0f, 0.0f, 0.0f, // bottom right
  -1.0f,  1.0f, 0.0f, 0.0f, // top left

  -1.0f,  1.0f, 0.0f, 0.0f, // top left
   1.0f,  1.0f, 0.0f, 0.0f, // top right
   1.0f, -1.0f, 0.0f, 0.0f  // bottom right
};
static const u8 numQuadVerts = 6;
static const u8 quadStride = 4;

static const char *quadFragSrc=
#include "flexfov.frag"
;

static const char *quadVertSrc =
"#version 110\n"
"\n"
"attribute vec2 aXY;\n"
"attribute vec2 aUV;\n"
"varying vec2 vUV;\n"
"\n"
"void main(void)\n"
"{\n"
"  vUV = aUV;\n"
"  gl_Position = vec4(aXY, 1.0, 1.0);\n"
"}\n"
;

static void create_quad(void) {

  // CREATE SHADER PROGRAM

  GLint success;

  // Vertex Shader
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &quadVertSrc, NULL);
  glCompileShader(vertex_shader);
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
      GLint max_length = 0;
      glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &max_length);
      char error_log[1024];
      printf("Quad Vertex shader compilation failed\n");
      glGetShaderInfoLog(vertex_shader, max_length, &max_length, &error_log[0]);
      printf("%s\n", &error_log[0]);
      abort();
  }

  // Fragment Shader
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &quadFragSrc, NULL);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
      GLint max_length = 0;
      glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &max_length);
      char error_log[1024];
      printf("Quad Fragment shader compilation failed\n");
      glGetShaderInfoLog(fragment_shader, max_length, &max_length, &error_log[0]);
      printf("%s\n", &error_log[0]);
      abort();
  }

  // Program
  quadProg = glCreateProgram();
  glAttachShader(quadProg, vertex_shader);
  glAttachShader(quadProg, fragment_shader);
  glLinkProgram(quadProg);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  // Attribs
  quadAttrXY = glGetAttribLocation(quadProg, "aXY");
  quadAttrUV = glGetAttribLocation(quadProg, "aUV");

  // Uniforms
  quadCamPitch = glGetUniformLocation(quadProg, "camPitch");
  quadUseRubix = glGetUniformLocation(quadProg, "useRubix");
  quadUseCube = glGetUniformLocation(quadProg, "useCube");
  quadFov = glGetUniformLocation(quadProg, "fov");
  quadMobiusZoom = glGetUniformLocation(quadProg, "mobiusZoom");
  quadControlsOn = glGetUniformLocation(quadProg, "controlsOn");
  quadZooming = glGetUniformLocation(quadProg, "zooming");
}

// set aspect-normalized uv
static void update_aspect(u32 w, u32 h) {
  float aspect=(float)w/(float)h;
  float defaultAspect=4.0f/3.0f;
  float u,v;
  if (aspect < defaultAspect) {
    // narrow
    v = 1.0f/defaultAspect;
    u = v*aspect;
  } else {
    // wide
    u = aspect/defaultAspect;
    v = 1.0f/defaultAspect;
  }

  // update quadVerts with aspect-normalized uv
  u8 i;
  for (i=0; i<numQuadVerts; i++) {
    u8 j = i*quadStride;
    quadVerts[j+2] = quadVerts[j]*u;
    quadVerts[j+3] = quadVerts[j+1]*v;
  }
}

static void render_quad(void) {
  restore_viewport();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // colors are already premultiplied, see previous usage of glBlendFuncSeparate

  u32 w,h;
  gfx_get_dimensions(&w, &h);
  update_aspect(w,h);

  glViewport(0, 0, w, h);

  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);

  extern void gfx_unload_current_shader(void);
  gfx_unload_current_shader();

  glUseProgram(quadProg);
  glUniform1f(quadCamPitch, camPitch);
  glUniform1i(quadUseRubix, useRubix);
  glUniform1i(quadUseCube, useCube);
  glUniform1f(quadFov, fov);
  glUniform1f(quadMobiusZoom, getMobiusZoom());
  glUniform1i(quadControlsOn, controlsOn);
  glUniform1i(quadZooming, zooming);

  glEnableVertexAttribArray(quadAttrXY); glVertexAttribPointer(quadAttrXY, 2, GL_FLOAT, GL_FALSE, quadStride*sizeof(float), NULL);
  glEnableVertexAttribArray(quadAttrUV); glVertexAttribPointer(quadAttrUV, 2, GL_FLOAT, GL_FALSE, quadStride*sizeof(float), (void*)(2*sizeof(float)));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTextureColor);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, numQuadVerts);
  glDisableVertexAttribArray(quadAttrXY);
  glDisableVertexAttribArray(quadAttrUV);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // restore normal blending
}

//------------------------------------------------------------------------------
// OpenGL command hooks
//------------------------------------------------------------------------------

// create display list index markers for when each of the functions below should be called
static Gfx *prehooksCube[6]; // cubemap framebuffer setpoints
static Gfx *prehookQuad;     // quad projection setpoint

void flexfov_run_prehook(Gfx *cmd) {
       if (cmd == prehooksCube[0]) { gfx_flush(); init_cubeside(0); }
  else if (cmd == prehooksCube[1]) { gfx_flush(); init_cubeside(1); }
  else if (cmd == prehooksCube[2]) { gfx_flush(); init_cubeside(2); }
  else if (cmd == prehooksCube[3]) { gfx_flush(); init_cubeside(3); }
  else if (cmd == prehooksCube[4]) { gfx_flush(); init_cubeside(4); }
  else if (cmd == prehooksCube[5]) { gfx_flush(); init_cubeside(5); }
  else if (cmd == prehookQuad)     { gfx_flush(); render_quad(); }
}

void flexfov_gfx_init(void) {
  create_cubemap();
  create_quad();
}

//------------------------------------------------------------------------------
// RDP rendering (display list additions)
//------------------------------------------------------------------------------

void flexfov_geo_process_root(struct GraphNodeRoot *root, Vp *b, Vp *c, s32 clearColor) {
  if (!flexfov_is_on()) {
    geo_process_root(root, b, c, clearColor);
    return;
  }

  u8 i;
  flexFovSky = TRUE;
  geo_process_root(root, b, c, clearColor);
  flexFovSky = FALSE;
  for (i=0; i<6; i++) {
    flexFovSide = i;
    prehooksCube[i] = gDisplayListHead;
    geo_process_root(root, b, c, clearColor);
  }
  prehookQuad = gDisplayListHead;
}

