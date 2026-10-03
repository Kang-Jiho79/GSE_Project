#version 330

in vec2 v_Uv;
in vec4 v_Color;

// The glyph's coverage, in red.
uniform sampler2D u_Atlas;

layout(location = 0) out vec4 FragColor;

void main()
{
	float coverage = texture(u_Atlas, v_Uv).r;
	FragColor = vec4(v_Color.rgb, v_Color.a * coverage);
}
