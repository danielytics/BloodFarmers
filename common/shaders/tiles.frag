#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform sampler2DArray texture_albedo;

void main()
{
	FragColor = texture(texture_albedo, TexCoords);
	// FragColor = vec4(1.0f, 1.0f, 1.0f, 0.0f);
}
