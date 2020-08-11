#version 110

// This is a shader that draws the wide angle projection
// and the extra UI elements for controlling it.


varying vec2 vUV;
// SM64 originally plays at 4:3 with 60° of view.
// The port extends the field of view for wider windows.
// Our fullscreen quad follows this with a “4:3 aspect-normalized” UV:
//
//
// u<-1     u=-1          u=0           u=1      u>1
//  *--------|-------------|-------------|--------*  v=3/4
//  |        :                           :        |
//  |        :         inner 4:3         :        |
//  |        :                           :        |
//  |        :                           :        |
//  |        :             o             :        -  v=0
//  |        :          (origin)         :        |
//  |        :                           :        |
//  |        :                           :        |
//  |        :                           :        |
//  *--------|---------------------------|--------*  v=-3/4
//            <------- desired fov ----->

// Environment map
uniform samplerCube cubeTexture;

// Toggles
uniform bool useRubix;    // show rubix grid overlay
uniform bool useCube;     // use cubenet projection
uniform bool controlsOn;  // controls are enabled (show normal fov box for reference)
uniform bool zooming;     // z-trig held (currently changing `mobiusZoom`)

// Knobs
uniform float fov;        // horizontal FOV from u=-1 to u=1
uniform float camPitch;   // pitch (up/down) angle used to transition panini <-> stereographic
uniform float mobiusZoom; // zoom center of image when fov > 180°

float pi = 3.14159;

//------------------------------------------------------------------------------
// Rays
//------------------------------------------------------------------------------

vec3 latlon_to_ray(vec2 latlon) {
  float lat = latlon.x;
  float lon = latlon.y;
  float x = sin(lon)*cos(lat);
  float y = sin(lat);
  float z = cos(lon)*cos(lat);
  return vec3(x,y,z);
}

vec2 ray_to_latlon(vec3 ray) {
  float x = ray.x;
  float y = ray.y;
  float z = ray.z;

  float lat = asin(y);
  float lon = atan(x,z);
  return vec2(lat,lon);
}

// projections are scaled by fov
// (by aligning u=1,v=0 with lat=0,lon=fov/2)
vec3 scaleray() {
  return latlon_to_ray(vec2(0.0, radians(fov)/2.0));
}

// inverse projections return this when uv is out of bounds
vec3 blankRay = vec3(-1.0,-1.0,-1.0);

//------------------------------------------------------------------------------
// Colored Annotation Overlays
//------------------------------------------------------------------------------

vec4 clear = vec4(0.0, 0.0, 0.0, 0.0);
vec4 red = vec4(1.0, 0.0, 0.0, 1.0);
vec4 green = vec4(0.0, 1.0, 0.0, 1.0);
vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);
vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);
vec4 magenta = vec4(1.0, 0.0, 1.0, 1.0);
vec4 cyan = vec4(0.0, 1.0, 1.0, 1.0);
vec4 white = vec4(1.0, 1.0, 1.0, 1.0);
vec4 black = vec4(0.0, 0.0, 0.0, 1.0);


// Show the original 4:3 window at 60° fov.
// (as a landmark reference for “home”)
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


// Show a 10x10 “rubix” grid on each cubeface
// (serves as an “indicatrix” for assessing distortion)
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

bool on_fov_overlay(vec2 uv) {
  // track
  float h = 0.5;
  vec2 center = vec2(0.8, 0.0);
  float y0 = center.y - h/2.0;
  float y1 = center.y + h/2.0;

  // knob position on track
  vec2 knob = vec2(center.x, mix(y0, y1, (fov - 90.0) / (360.0 - 90.0)));
  float knobR = 1.0/40.0;

  // fov display
  float fovR = 1.0/10.0;
  vec2 fovPos = center + vec2(fovR*2.0, 0.0);

  float thick = 0.0125/2.0;

  // draw knob
  if (distance(uv, knob) < knobR) { // inside knob
    return true;
  }

  // draw fov
  float angle = acos(dot(vec2(0,1), normalize(uv-knob)));
  float dist = distance(uv, knob);
  if (angle < radians(fov)/2.0 &&  fovR - thick*8.0 < dist && dist < fovR) {
    return true;
  }

  // draw track
  if (abs(uv.x-center.x) < thick/2.0 && y0 < uv.y && uv.y < y1) {
    return true;
  }
  return false;
}

vec4 fov_overlay(vec2 uv) {
  float thick = 0.0125/2.0;
  vec2 shadowUV = uv + vec2(-thick, thick);
  if (on_fov_overlay(uv)) {
    return white;
  } else if (on_fov_overlay(shadowUV)) {
    return black;
  }
  return clear;
}

bool on_zoom_overlay(vec2 uv) {
  // track
  float w = 0.5;
  vec2 center = vec2(0.0, -0.75 + 0.2);
  float x0 = center.x - w/2.0;
  float x1 = center.x + w/2.0;

  // knob position on track
  float m = (mobiusZoom+1.0)/2.0;
  vec2 knob = vec2(mix(x0, x1, m), center.y);
  float knobR = 1.0/40.0;

  float thick = 0.0125/2.0;

  // draw knob
  if (distance(uv, knob) < knobR) { // inside knob
    return true;
  }

  bool insideX = x0-2.0*knobR < uv.x && uv.x < x1+2.0*knobR;
  float distY = abs(uv.y-center.y);

  // draw track
  if (distY < thick/2.0 && insideX) {
    return true;
  }

  // draw hills
  if (insideX) {
    float dh = thick * 3.0;
    float h0 = dh*2.0;
    float h1 = dh*5.0;

    float outH = mix(h1, h0, m);
    float inH = mix(h0, h1, m);

    float curH = uv.x < center.x ?
      mix(outH, inH, (uv.x - x0) / (w/2.0)) :
      mix(inH, outH, (uv.x - center.x) / (w/2.0));

    if (curH - thick < distY && distY < curH) {
      return true;
    }
  }

  return false;
}

vec4 zoom_overlay(vec2 uv) {
  float thick = 0.0125/2.0;
  vec2 shadowUV = uv + vec2(-thick, thick);
  if (on_zoom_overlay(uv)) {
    return white;
  } else if (on_zoom_overlay(shadowUV)) {
    return black;
  }
  return clear;
}

//------------------------------------------------------------------------------
// Cube lookup
//------------------------------------------------------------------------------

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

// lookup color in cubemap
// (accounting for colored overlays)
vec4 cubecolor(vec3 ray) {
  // translucent black if blank ray
  vec4 color;

  if (blankRay == ray) {
    color = vec4(0.0, 0.0, 0.0, 0.5);
  } else {

    // get cube color
    color = textureCube(cubeTexture, cuberay(ray));

    // add rubix overlay
    if (useRubix) {
      vec4 rubixColor = rubix(ray);
      if (rubixColor != clear) {
        color = mix(color, rubixColor, 0.3);
      }
    }

    // add normal fov border
    if (controlsOn && on_normal_fov_border(ray)) {
      color = mix(color, white, 0.5);
    }

  }

  // control overlays
  if (controlsOn) {

    vec4 fovColor = fov_overlay(vUV);
    if (fovColor != clear) {
      color = mix(color, fovColor, zooming ? 0.2 : 0.7);
    }

    if (fov > 180.0) {
      vec4 zoomColor = zoom_overlay(vUV);
      if (zoomColor != clear) {
        color = mix(color, zoomColor, zooming ? 0.7 : 0.2);
      }
    }
  }

  return color;
}

//------------------------------------------------------------------------------
// Stereographic projection
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
  float scale = stereographic_forward(scaleray()).x;
  return stereographic_inverse(uv * scale);
}

//------------------------------------------------------------------------------
// Panini projection
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
  return latlon_to_ray(vec2(lat,lon));
}

vec2 panini_forward(vec3 ray) {
  vec2 latlon = ray_to_latlon(ray);
  float lat = latlon.x;
  float lon = latlon.y;
  float d = 1.0;
  float S = (d+1.0)/(d+cos(lon));
  float x = S*sin(lon);
  float y = S*tan(lat);
  return vec2(x,y);
}

vec3 panini(vec2 uv) {
  float scale = panini_forward(scaleray()).x;
  return panini_inverse(uv * scale);
}

//------------------------------------------------------------------------------
// “Flex” projection between Panini and Stereographic (using camPitch)
//------------------------------------------------------------------------------

vec3 flex_inverse(vec2 uv) {
  float k = abs(camPitch)/(pi/2.0);
  return mix(panini_inverse(uv),stereographic_inverse(uv),k);
}

vec2 flex_forward(vec3 ray) {
  float k = abs(camPitch)/(pi/2.0);
  return mix(panini_forward(ray),stereographic_forward(ray),k);
}

vec3 flex(vec2 uv) {
  float k = abs(camPitch)/(pi/2.0);
  return mix(panini(uv),stereographic(uv),k);
}

//------------------------------------------------------------------------------
// Mobius scale (used for zooming Mercator and Equirect)
//------------------------------------------------------------------------------

float getMobiusScale() {
  float k = mobiusZoom;
  return k >= 0.0 ? mix(1.0, 0.5, k) : mix(1.0, 2.0, -k);
}

//------------------------------------------------------------------------------
// Mercator projection
//------------------------------------------------------------------------------

float sinh(float t) {
  return (exp(t) - exp(-t)) / 2.0;
}

vec3 mercator_inverse(vec2 uv) {
  float x = uv.x;
  float y = uv.y;
  if (abs(x) > pi) return blankRay;
  float lon = x;
  float lat = atan(sinh(y));
  return latlon_to_ray(vec2(lat,lon));
}

vec2 mercator_forward(vec3 ray) {
  vec2 latlon = ray_to_latlon(ray);
  float lat = latlon.x;
  float lon = latlon.y;
  float x = lon;
  float y = log(tan(pi*0.25+lat*0.5));
  return vec2(x,y);
}

vec3 mercator(vec2 uv) {
  float m = getMobiusScale();

  vec3 scaleRay = scaleray();
  if (mobiusZoom != 0.0) {
    scaleRay = stereographic_inverse(stereographic_forward(scaleRay)/m);
  }
  float scale = mercator_forward(scaleRay).x;
  vec3 ray = mercator_inverse(uv * scale);
  if (ray != blankRay && mobiusZoom != 0.0) {
    ray = flex_inverse(flex_forward(ray)*m);
  }
  return ray;
}

//------------------------------------------------------------------------------
// Equirectangular projection
//------------------------------------------------------------------------------

vec3 equirect_inverse(vec2 uv) {
  float x = uv.x;
  float y = uv.y;
  if (abs(x) > pi || abs(y) > pi/2.0) return blankRay;
  float lon = x;
  float lat = y;
  return latlon_to_ray(vec2(lat,lon));
}

vec2 equirect_forward(vec3 ray) {
  vec2 latlon = ray_to_latlon(ray);
  float lat = latlon.x;
  float lon = latlon.y;
  float x = lon;
  float y = lon;
  return vec2(x,y);
}

vec3 equirect(vec2 uv) {
  float m = getMobiusScale();
  float scale = equirect_forward(scaleray()).x;
  vec3 ray = equirect_inverse(uv * scale);
  if (ray != blankRay && mobiusZoom != 0.0) {
    ray = flex_inverse(flex_forward(ray)*m);
  }
  return ray;
}

//------------------------------------------------------------------------------
// Cube net projection
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

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

void main(void)
{
  vec2 uv = vUV;
  vec3 ray;
  if (useCube)           { ray = cubenet(uv); }
  //else if (fov <= 180.0) { ray = flex(uv); }
  else if (fov < 360.0)  { ray = mercator(uv); }
  else if (fov == 360.0) { ray = equirect(uv); }
  gl_FragColor = cubecolor(ray);
}

