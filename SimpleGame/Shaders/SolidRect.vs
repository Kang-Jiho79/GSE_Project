#version 330

// Per vertex: the shared unit quad, used only as a pair of 0..1 weights.
in vec3 a_Position;

// Per instance: the four corners this quad actually occupies, already in
// clip space. Giving corners outright rather than a 2x2 transform is what
// lets a face taper instead of always being a parallelogram.
in vec4 a_CornerLo; // xy = bottom-left, zw = bottom-right
in vec4 a_CornerHi; // xy = top-left,    zw = top-right
in vec4 a_Color;
// x = how much it glows, y = whether it belongs to a figure (for outlines).
in vec2 a_Material;

out vec4 v_Color;
out vec2 v_Material;

void main()
{
	vec2 weight = a_Position.xy + 0.5;

	vec2 bottom = mix(a_CornerLo.xy, a_CornerLo.zw, weight.x);
	vec2 top = mix(a_CornerHi.xy, a_CornerHi.zw, weight.x);

	gl_Position = vec4(mix(bottom, top, weight.y), 0.0, 1.0);
	v_Color = a_Color;
	v_Material = a_Material;
}
