#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2DArray texture_albedo;

void main()
{
	FragColor = texture(texture_albedo, vec3(TexCoords, 0));
}
