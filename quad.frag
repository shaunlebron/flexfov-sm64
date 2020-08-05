#version 110

uniform samplerCube cubeTexture;
varying vec2 vUV;
uniform float camPitch;
uniform bool useRubix;
uniform bool useCube;

float pi = 3.14159;

vec3 latlon_to_ray(float lat, float lon) {
  //      +Y ^
  //         |
  //         |
  //
  //        * *
  //     *       *
  //    *   _+Z   *   --->
  //    *   /|    *     +X
  //     * /     *
  //        * *
  float x = sin(lon)*cos(lat);
  float y = sin(lat);
  float z = cos(lon)*cos(lat);
  return vec3(x,y,z);
}

// cubemap texture lookup vector
// (accounting for upside-down textures)
vec3 cuberay(vec3 ray) {
  float x = ray.x;
  float y = ray.y;
  float z = ray.z;
  float ax = abs(x);
  float ay = abs(y);
  float az = abs(z);
  bool upOrDownFace = ay >= ax && ay >= az;
  return upOrDownFace ? vec3(x,y,-z) : vec3(x,-y,z);
}

bool on_normal_fov_border(vec3 ray) {
  float x = ray.x;
  float y = ray.y;
  float z = ray.z;
  bool isFront = z > 0.0 && abs(z) > abs(x) && abs(z) > abs(y);
  if (!isFront) return false;

  // project ray to front face (at z=1)
  x = x/z;
  y = y/z;

  float vfov = radians(45.0);
  float hfov = vfov/3.0*4.0;

  // bounds
  float xb = tan(hfov/2.0);
  float yb = tan(vfov/2.0);

  // .---------------.
  // | .-----------. |
  // | |           | |
  // | |     o     | |
  // | |           | |
  // | .-----------. |
  // .---------------.

  float thick = 0.0125/2.0;
  bool insideOuter = (
    -xb-thick < x && x < xb+thick &&
    -yb-thick < y && y < yb+thick
  );
  bool insideInner = (
    -xb+thick < x && x < xb-thick &&
    -yb+thick < y && y < yb-thick
  );
  bool insideBorder = insideOuter && !insideInner;
  return insideBorder;
}

vec3 blankRay = vec3(-1.0,-1.0,-1.0);


vec4 clear = vec4(0.0, 0.0, 0.0, 0.0);
vec4 red = vec4(1.0, 0.0, 0.0, 1.0);
vec4 green = vec4(0.0, 1.0, 0.0, 1.0);
vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);
vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);
vec4 magenta = vec4(1.0, 0.0, 1.0, 1.0);
vec4 cyan = vec4(0.0, 1.0, 1.0, 1.0);
vec4 white = vec4(1.0, 1.0, 1.0, 1.0);
vec4 black = vec4(0.0, 0.0, 0.0, 1.0);

vec4 rubix(vec3 ray) {
  float x = ray.x;
  float y = ray.y;
  float z = ray.z;
  float ax = abs(x);
  float ay = abs(y);
  float az = abs(z);

  float u, v;
  vec4 color;
  if (ax >= ay && ax >= az) { // left or right
    color = x > 0.0 ? red : blue;
    u = y/ax;
    v = z/ax;
  } else if (ay >= ax && ay >= az) { // up or down
    color = y > 0.0 ? magenta : cyan;
    u = x/ay;
    v = z/ay;
  } else if (az >= ax && az >= ay) { // front or back
    color = z > 0.0 ? white : black;
    u = x/az;
    v = y/az;
  }

  // normalize uv to 0 to 1
  u = (u + 1.0) / 2.0;
  v = (v + 1.0) / 2.0;

  float numCells = 10.0;
  float cellSize = 4.0;
  float padSize = 1.0;
  float blockSize = padSize + cellSize;
  float numUnits = numCells * blockSize + padSize;

  bool onGrid =
    mod(u*numUnits, blockSize) < padSize ||
    mod(v*numUnits, blockSize) < padSize;

  if (onGrid) return clear;
  return color;
}

vec4 cubecolor(vec3 ray) {
  vec4 color = blankRay == ray ? mix(black, clear, 0.5) : textureCube(cubeTexture, cuberay(ray));
  if (useRubix) {
    vec4 rubixColor = rubix(ray);
    if (rubixColor != clear) {
      color = mix(color, rubixColor, 0.3);
    }
  }
  if (on_normal_fov_border(ray)) {
    color = mix(color, white, 0.5);
  }
  return color;
}

//------------------------------------------------------------------------------
// Stereographic
//------------------------------------------------------------------------------

vec3 stereographic_inverse(vec2 uv) {
  float x = uv.x;
  float y = uv.y;
  float r = sqrt(x*x+y*y);
  float theta = atan(r)/0.5;
  float s = sin(theta);
  return vec3(x/r*s, y/r*s, cos(theta));
}

vec2 stereographic_forward(vec3 ray) {
  float x = ray.x;
  float y = ray.y;
  float z = ray.z;
  float theta = acos(z);
  float r = tan(theta*0.5);
  float c = r/sqrt(x*x+y*y);
  return vec2(x*c, y*c);
}

vec3 stereographic(vec2 uv) {
  float fov = radians(180.0);
  vec3 ray = latlon_to_ray(0.0, fov/2.0);
  float scale = stereographic_forward(ray).x;
  return stereographic_inverse(uv * scale);
}

//------------------------------------------------------------------------------
// Panini
//------------------------------------------------------------------------------

vec3 panini_inverse(vec2 uv) {
  float x = uv.x;
  float y = uv.y;
  float d = 1.0;
  float k = x*x/((d+1.0)*(d+1.0));
  float dscr = k*k*d*d - (k+1.0)*(k*d*d-1.0);
  float clon = (-k*d+sqrt(dscr))/(k+1.0);
  float S = (d+1.0)/(d+clon);
  float lon = atan(x,S*clon);
  float lat = atan(y,S);
  return latlon_to_ray(lat,lon);
}

vec2 panini_forward(float lat, float lon) {
  float d = 1.0;
  float S = (d+1.0)/(d+cos(lon));
  float x = S*sin(lon);
  float y = S*tan(lat);
  return vec2(x,y);
}

vec3 panini(vec2 uv) {
  float fov = radians(180.0);
  float scale = panini_forward(0.0, fov/2.0).x;
  return panini_inverse(uv * scale);
}

//------------------------------------------------------------------------------
// Flex between Panini and Stereographic depending on pitch
//------------------------------------------------------------------------------

vec3 flex(vec2 uv) {
  float k = abs(camPitch)/(3.14159/2.0);
  return mix(panini(uv),stereographic(uv),k);
}

//------------------------------------------------------------------------------
// Debugging projections
//------------------------------------------------------------------------------

vec3 cubenet(vec2 uv) {
  float u = uv.x;   // -1.0 < u < 1.0
  float i = u*2.0+2.0;   //  0.0 < i < 4.0

  float v = uv.y;   // -0.75 < v < 0.75
  float j = v*2.0+1.5; //  0.0  < j < 3.0

  //     *---*          j=3   v=3/4
  //    .|   |.
  // *---*---*---*---*  j=2   v=1/4
  // |  -|-  |   |   |                 v=0
  // *---*---*---*---*  j=1   v=-1/4
  //    .|   |.
  //     *---*          j=0   v=-3/4
  //
  // i=0 i=1 i=2 i=3 i=4
  // u=-1    u=0     u=1

  int col = int(floor(i));
  int row = int(floor(j));
  u = mix(-1.0, 1.0, mod(i,1.0));
  v = mix(-1.0, 1.0, mod(j,1.0));

  float x,y,z;
  if (col == 1 && row == 1)      { x = u; y = v; z = 1.0; } // front
  else if (col == 0 && row == 1) { x = -1.0; y = v; z = u; } // left
  else if (col == 2 && row == 1) { x = 1.0; y = v; z = -u; } // right
  else if (col == 3 && row == 1) { x = -u; y = v; z = -1.0; } // back
  else if (col == 1 && row == 2) { x = u; y = 1.0; z = -v; } // up
  else if (col == 1 && row == 0) { x = u; y = -1.0; z = v; } // down
  else return blankRay;

  return vec3(x,y,z);
}



void main(void)
{
  vec3 ray = useCube ? cubenet(vUV) : flex(vUV);
  gl_FragColor = cubecolor(ray);
}

/* DEBUGGING aspect box
if (-1.0 < u && u < 1.0 &&
    -0.75 < v && v < 0.75) {
  gl_FragColor = vec4(1.0, 1.0, 1.0, 0.5);
} else {
  gl_FragColor = vec4(0.0, 0.0, 0.0, 0.5);
}
*/
