#VERTEX_SHADER
#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 MVP;

out vec3 TexCoords;

void main() {

	TexCoords = position;
	vec4 pos = MVP*vec4(position, 1.0);
	gl_Position = pos.xyww;
}

#FRAGMENT_SHADER
#version 330 core

layout(location = 0) out vec4 fragColor;

in vec3 TexCoords;

uniform float iTime;
uniform samplerCube skybox;

vec3 lightPos = vec3(3, 3, -8);

#define PI 3.141592653

void main() {
	vec3 color = vec3(texture(skybox, vec3(TexCoords.x, TexCoords.y, TexCoords.z) ));

	fragColor = vec4(color, 1.0);
}