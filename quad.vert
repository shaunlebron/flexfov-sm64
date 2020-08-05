#version 110

attribute vec2 aXY;
attribute vec2 aUV;
varying vec2 vUV;
//uniform float fov;

void main(void)
{
  vUV = aUV;
  gl_Position = vec4(aXY, 1.0, 1.0);
}
