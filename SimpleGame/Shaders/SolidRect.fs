#version 330

in vec4 v_Color;
in vec2 v_Material;

layout(location=0) out vec4 FragColor;
// What the post-process needs to know about each pixel. Blended with the same
// alpha as the colour, so whatever is drawn over a glow also covers its mark,
// and a faint shape leaves a faint mark. Ignored when drawing straight to the
// window, which has nowhere to put it.
layout(location=1) out vec4 Aux;

void main()
{
	FragColor = v_Color;
	Aux = vec4(v_Material.x, v_Material.y, 0.0, v_Color.a);
}
