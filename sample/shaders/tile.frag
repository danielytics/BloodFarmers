#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform sampler2DArray texture_albedo;

void main()
{
	FragColor = texture(texture_albedo, TexCoords);
}
