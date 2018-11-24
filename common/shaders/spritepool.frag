#version 330 core
in VertexData {
	vec2 textureCoordinates;
	flat int image;
} fragment;

out vec4 FragColor;

uniform sampler2DArray u_texture;

void main(void) {
	vec4 tex = texture(u_texture, vec3(fragment.textureCoordinates, fragment.image));
	if (tex.a < 1.0) {
		discard; // No early-z for sprites :'(
	}
    FragColor = tex;
}