#version 330

in vec2 v_Uv;

uniform sampler2D u_Scene;
uniform sampler2D u_Bloom;
// Per pixel: glow in red, figure in green.
uniform sampler2D u_Aux;
uniform vec2 u_Texel;

uniform float u_BloomIntensity;

// Figures: an ink line just outside them, and the edge facing the light
// brightened just inside. Offsets are in pixels.
uniform float u_Outline; // 0 off, 1 on
uniform float u_OutlineWidth;
uniform float u_OutlineStrength;
uniform vec3 u_OutlineColor;
uniform vec2 u_RimOffset;
uniform float u_RimStrength;
uniform vec3 u_RimColor;

// The grade: how much colour is kept, how far tones are pushed apart, and a
// tint over everything. 1, 1 and white leave the frame as it was.
uniform float u_Saturation;
uniform float u_Contrast;
uniform vec3 u_Tint;

layout(location = 0) out vec4 FragColor;

// Low on purpose: a translucent effect passing over a body thins its mark
// without breaking the outline.
const float kFigureThreshold = 0.25;

float Figure(vec2 offsetPixels)
{
	return texture(u_Aux, v_Uv + offsetPixels * u_Texel).g;
}

vec3 Outline(vec3 color)
{
	if (Figure(vec2(0.0)) >= kFigureThreshold)
	{
		// Inside: catch the light along the edge that faces it.
		if (Figure(u_RimOffset) < kFigureThreshold)
		{
			color += u_RimColor * u_RimStrength;
		}
		return color;
	}

	// Outside: ink wherever a figure lies within reach.
	float w = u_OutlineWidth;
	float d = u_OutlineWidth * 0.7071;
	float near = Figure(vec2(w, 0.0));
	near = max(near, Figure(vec2(-w, 0.0)));
	near = max(near, Figure(vec2(0.0, w)));
	near = max(near, Figure(vec2(0.0, -w)));
	near = max(near, Figure(vec2(d, d)));
	near = max(near, Figure(vec2(-d, d)));
	near = max(near, Figure(vec2(d, -d)));
	near = max(near, Figure(vec2(-d, -d)));

	if (near >= kFigureThreshold)
	{
		color = mix(color, u_OutlineColor, u_OutlineStrength);
	}
	return color;
}

void main()
{
	vec3 color = texture(u_Scene, v_Uv).rgb;
	if (u_Outline > 0.5)
	{
		color = Outline(color);
	}

	color += texture(u_Bloom, v_Uv).rgb * u_BloomIntensity;

	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	color = mix(vec3(luma), color, u_Saturation);
	color = (color - 0.5) * u_Contrast + 0.5;
	color *= u_Tint;

	FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
