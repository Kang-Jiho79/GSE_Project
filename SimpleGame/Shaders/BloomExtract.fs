#version 330

in vec2 v_Uv;

uniform sampler2D u_Scene;
uniform sampler2D u_Aux;

layout(location = 0) out vec4 FragColor;

void main()
{
	// Only what was drawn as light is lifted out - however bright anything
	// else on screen happens to be.
	vec3 color = texture(u_Scene, v_Uv).rgb;
	float glow = texture(u_Aux, v_Uv).r;
	FragColor = vec4(color * glow, 1.0);
}
