<img width="150px" src="img/LakituSM64.png" align="right">

# flexfov for sm64

Extra camera controls for panoramic play.

🎥 See [demo video](https://youtu.be/EX-GF2JhLaI).

## Controls

Hold R to use extra controls:

* Thumbstick: change FOV (up to 360°)
* A: overlay grid
* B: box projection
* Z + thumbstick: zoom center of the image when fov > 180°

## How it works

<table>
<tr>
<td>1. Render six 90° views</td>
<td>2. Stitch together by projection</td>
</tr>
<tr>
<td><img src="img/rubix-cubenet.jpg"></td>
<td><img src="img/rubix-panini.jpg"></td>
</tr>
</table>

## Construction

We choose the best default projection based on your desired FOV, and let you
scale the center region if you want.  We do this as fluidly as possible using
the following knobs, projections, and procedures.

### Knobs

- fov (manual)
- pitch (auto)
- mobius (auto or manual)

FOV and mobius are coupled when automatic:

```
                       180°            360°
      fov <-------------|--------------->
                        |--------------->   mobius
               0.5     0.5            1.0
```

### Projections

- panini (architecture)
- stereographic (ground/sky)
- mercator (world)

```
             0°     pitch     ±90°
      panini <------------------> stereographic
                      ^
                      |
                      | mobius
                      |
                      v
                   mercator
```

### Procedures

Key:

```
    _
   (_)  >  ---     Forward projection (sphere to plane)
            _
   ---  >  (_)     Inverse projection (plane to sphere)

   ---  >  ---     Scale (plane to plane)
```

Overview:

```
    _                       _
   (_)  >  ---  >  ---  >  (_)  > --- > ---       <--- topologies
    1       2       3       4      5     6

          pitch   mobius                fov       <--- relevant knobs
         ---------------         -----------
             panini/               mercator       <--- projections
          stereographic
```

1. Sphere of pixels collected from cubemap.
2. Forward projection to plane, using pitch-based panini-stereographic.
3. Scale using the mobius factor.
4. Inverse projection to sphere, using original panini-stereographic (non-scaled).
5. Forward projection to plane using mercator.
6. Scale such that the desired fov range touches screen bounds.

### Why Mobius?

A “Mobius” factor is used to push the pixels on the sphere *toward* or *away*
from the center of the screen.  The scaling is done on a projected plane, then
projected back to the sphere.  The spherical image is now warped by the
projection.  Why?  Two reasons:

1. Feathering the transition to mercator after 180°.
  - It looks more natural (than ray interpolation) to  “bake in”
    the panini/stereographic projection into the
    image that mercator is projecting.
2. Allowing custom magnification when approaching 360°.

See the following for more details:

- Spherical video editing with Möbius transformations
  - [video](https://youtu.be/oVwmF_vrZh0)
  - [blog](https://web.archive.org/web/20190627155016/http://elevr.com/spherical-video-editing-effects-with-mobius-transformations/)
- [The Mercator Redemption](http://archive.bridgesmathart.org/2013/bridges2013-217.pdf)

_Thanks to Jai for finding and sharing this transformation method. And thanks
to Richard for asking if cropping a panini-zoomed mercator would be similar to
panini alone, leading to our use of it to feather the transition._

## Outstanding Issues

- fog is non-uniform across cubefaces
- window in portrait mode clips cubefaces

## References

As seen in [quake] and [minecraft].

[quake]:https://github.com/shaunlebron/blinky
[minecraft]:https://github.com/shaunlebron/flex-fov
