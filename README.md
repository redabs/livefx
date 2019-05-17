# livefx
Hot loading OpenGL shader toy

![bla](https://i.imgur.com/dE6PSiV.png)

## Inputs to the fragment shader
* `vec2 FragPosition` Pixel coordinate of the fragment
* `vec2 Resolution` Width and height of the client window in pixels
* `float Time` Floating point seconds elapsed since the start of the program
 
 
## Sample program
```c
out vec4 FragColor;

void main() {
    vec2 UV = FragPosition / Resolution * 2. - 1.;
    UV.x *= Resolution.x / Resolution.y;

    float Radius = 0.3 * (sin(Time) + 1.) * 0.5;
    float MonoChrome = step(length(UV), Radius);

    FragColor = vec4(vec3(MonoChrome), 1);
}
```
## Dependencies
* SDL2
* OpenGL

## TODO
* Win32 support
* Better probing of newest GLSL version available on system
