// https://drewcassidy.me/2020/06/26/sdf-antialiasing/
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform vec2 iResolution;
uniform float uroundness;
uniform vec4 colDiffuse;

// xy: half size
float sd_rectangle(vec2 p, vec2 xy) {
    vec2 d = abs(p) - max(xy, 0.0);
    float outer = length(max(d, 0.0));
    float inner = min(max(d.x, d.y), 0.0);
    return outer + inner;
}

// xy: half size
// r: radius [top left, top right, bottom left, bottom right]
float sd_rounded_rectangle(vec2 p, vec2 xy, vec4 r) {
    float s = r[int(p[0] > 0.0) + int(p[1] < 0.0) * 2];
    // shrink when `radius < min(w,h)/2`
    s = min(s, min(xy.x, xy.y));
    return sd_rectangle(p, xy - s) - s;
}

// xy: half size
// border: border width [top, right, bottom, left]
float sd_border(vec2 p, vec2 xy, float r, vec4 border) {
    vec2 dp = vec2(border[1] - border[3], border[0] - border[2]) / 2.;
    vec2 inner_p = p + dp;
    vec2 dxy = vec2(border[1] + border[3], border[0] + border[2]) / 2.;
    vec2 inner_xy = xy - dxy;
    float d2 = sd_rounded_rectangle(p, xy, vec4(r));
    if(d2 > 0.0 || inner_xy.x < 0.0 || inner_xy.y < 0.0) {
        return d2;
    }
    // FIXME: when border widths are different, inner corner should be ellipse
    // FIXME: when inner corner radius shrinks, it will not match outer corner
    float r0 = max(r - max(border[3], border[0]), 0.0);
    float r1 = max(r - max(border[0], border[1]), 0.0);
    float r2 = max(r - max(border[2], border[3]), 0.0);
    float r3 = max(r - max(border[1], border[2]), 0.0);
    vec4 inner_r = vec4(r0, r1, r2, r3);
    float d1 = sd_rounded_rectangle(inner_p, inner_xy, inner_r);
    return d1 < 0.0 ? -d1 : max(-d1, d2);
}

void main()
{
    float roundness = clamp(uroundness, 0.,1.);
    roundness*=2.;

    float w = min(iResolution.x, iResolution.y) / 2.0;
    vec2 p = (fragTexCoord * 2.0 - iResolution.xy) / w;

    vec4 border = vec4(3);
    float d = sd_border(p, (iResolution.xy)/w, 4., border);

    vec2 ddist = vec2(dFdx(d),dFdy(d));
    float pd = d / length(ddist);
    float a= clamp(0.5-pd,0.,1.);
    vec4 color = fragColor;
    color.a=a;
    finalColor = color;
}
