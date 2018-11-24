#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec3 TexCoords;

uniform mat4 projection_view;

void main()
{
	TexCoords = aTexCoords;
	gl_Position = projection_view * vec4(aPos, 1.0);
}