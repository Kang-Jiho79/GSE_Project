#version 330

in vec3 a_Position;
uniform vec2 u_Trans;
uniform mat2 u_Transform;

void main()
{
	vec4 newPosition;
	newPosition.xy = u_Transform * a_Position.xy + u_Trans;
	newPosition.z = 0;
	newPosition.w= 1;
	gl_Position = newPosition;
}
