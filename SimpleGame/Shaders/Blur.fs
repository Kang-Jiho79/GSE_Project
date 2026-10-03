#version 330

in vec2 v_Uv;

uniform sampler2D u_Source;
// The distance between taps along the one direction this pass blurs in.
uniform vec2 u_Step;

layout(location = 0) out vec4 FragColor;

// A 9-tap Gaussian, centre first. Run once across and once down for a round
// blur at a fraction of the cost of doing both at once.
const float kWeights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
	vec3 sum = texture(u_Source, v_Uv).rgb * kWeights[0];
	for (int i = 1; i < 5; i++)
	{
		vec2 offset = u_Step * float(i);
		sum += texture(u_Source, v_Uv + offset).rgb * kWeights[i];
		sum += texture(u_Source, v_Uv - offset).rgb * kWeights[i];
	}
	FragColor = vec4(sum, 1.0);
}
