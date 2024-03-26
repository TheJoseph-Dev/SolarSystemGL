#VERTEX_SHADER
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoords;
layout(location = 2) in vec3 normals;

uniform mat4 MVP;
uniform mat4 model;

out vec2 TexCoords;
out vec3 Normals;
out vec3 lightPos; // World Space Position
out vec3 FragPos; // World Space Position

void main() {
	TexCoords = vec2(texCoords.x, 1.0-texCoords.y);
	Normals = transpose(inverse(mat3(model))) * normals;
	lightPos = vec3(0.0);
	FragPos = vec3( model * vec4(position, 1.0) );

	gl_Position = MVP*vec4(position, 1.0);
}

#FRAGMENT_SHADER
#version 330 core

layout(location = 0) out vec4 fragColor;

in vec2 TexCoords;
in vec3 Normals;
in vec3 FragPos;
in vec3 lightPos;

uniform vec2 iResolution;
uniform float iTime;
uniform sampler2DArray mainTex;
uniform int texLayer;
uniform int isEmissive;

#define PI 3.141592653

void main() {
	
	if (isEmissive == 0) {
		vec3 lightDir = normalize( lightPos - FragPos );
		const float ambientL = 0.1;
		float objLight = min(max(dot(normalize(Normals), lightDir), 0.0)/4.0 + ambientL, 1.0);
		vec3 color = vec3(texture(mainTex, vec3(TexCoords.xy, texLayer) )); //vec3(1.0);

		fragColor = vec4( objLight * color, 1.0 );
	}
	else {
		vec3 color = vec3(texture(mainTex, vec3(TexCoords.xy, texLayer) ));
		fragColor = vec4(color * 20.0, 1.0);
	}

}