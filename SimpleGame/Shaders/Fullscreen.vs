#version 330

// One triangle large enough to cover the whole screen; the part outside is
// clipped away. uv runs 0..1 across the visible part.
layout(location = 0) in vec2 a_Position;

out vec2 v_Uv;

void main()
{
	v_Uv = a_Position * 0.5 + 0.5;
	gl_Position = vec4(a_Position, 0.0, 1.0);
}
