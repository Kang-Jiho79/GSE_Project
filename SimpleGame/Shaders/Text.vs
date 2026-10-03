#version 330

// Per vertex: the shared unit quad, used only as a pair of 0..1 weights.
in vec3 a_Position;

// Per instance: one glyph. Its box in clip space and the matching corners of
// its spot in the font texture, both bottom-left then top-right.
in vec4 a_Rect;
in vec4 a_Uv;
in vec4 a_Color;

out vec2 v_Uv;
out vec4 v_Color;

void main()
{
	vec2 weight = a_Position.xy + 0.5;

	gl_Position = vec4(mix(a_Rect.xy, a_Rect.zw, weight), 0.0, 1.0);
	v_Uv = mix(a_Uv.xy, a_Uv.zw, weight);
	v_Color = a_Color;
}
