#version 150

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

in vec3 position;
out vec3 Position;

void main()
{
	gl_Position = proj * view * model * vec4( position, 1.0 );     //output position with projection
	Position = gl_Position.xyz;
}
