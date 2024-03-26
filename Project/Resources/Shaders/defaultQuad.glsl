#VERTEX_SHADER
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoords;

uniform mat4 MVP;

out vec2 TexCoords;

void main() {
	TexCoords = texCoords;
	gl_Position = MVP*vec4(position, 1.0);
}

#FRAGMENT_SHADER
#version 330 core

layout(location = 0) out vec4 fragColor;
uniform vec2 iResolution;

uniform sampler2DArray mainTex;
uniform int texLayer;

in vec2 TexCoords;

void main() {
	vec2 fragCoord = gl_FragCoord.xy;
	vec2 uv = fragCoord / iResolution * 2.0 - 1.0;
	float aspect = iResolution.x / iResolution.y; 
	uv.x *= aspect;

	vec4 tColor = texture(mainTex, vec3(TexCoords.xy, texLayer));

	if (tColor.a < 0.05) discard;
	fragColor = tColor*0.5; //+ vec4(0,0,1.0,0.0);
}