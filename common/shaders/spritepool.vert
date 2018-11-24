#version 330 core
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_UV;

// layout (std140) uniform Matrices
// {
// 	mat4 projection;
// 	mat4 view;
// };

uniform mat4 projection;
uniform mat4 view;

uniform bool billboarding;
uniform bool spherical_billboarding;

out VertexData {
	vec2 textureCoordinates;
	flat int image;
} vertex;

uniform samplerBuffer u_tbo_tex;

void main() {
	int offset = gl_InstanceID * 4;
    vec3 instance_pos = vec3(texelFetch(u_tbo_tex, offset).r,
						     texelFetch(u_tbo_tex, offset + 1).r,
					  	     texelFetch(u_tbo_tex, offset + 2).r);
	vertex.image = int(texelFetch(u_tbo_tex, offset + 3).r);
	vec3 position = in_Position + instance_pos;

	mat4 model = mat4(1.0);
	model[3][0] = instance_pos.x;
	model[3][1] = instance_pos.y;
	model[3][2] = instance_pos.z;
	mat4 modelView = view * model;

	if (billboarding) {
		modelView[0][0] = 1.0; 
		modelView[0][1] = 0.0; 
		modelView[0][2] = 0.0; 

		if (spherical_billboarding) {
			// Second colunm.
			modelView[1][0] = 0.0; 
			modelView[1][1] = 1.0; 
			modelView[1][2] = 0.0; 
		}

		// Thrid colunm.
		modelView[2][0] = 0.0; 
		modelView[2][1] = 0.0; 
		modelView[2][2] = 1.0; 
	}


	gl_Position = projection * modelView * vec4(in_Position, 1.0);
	vertex.textureCoordinates = in_UV;
}