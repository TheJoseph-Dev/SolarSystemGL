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
uniform float scale;

in vec2 TexCoords;

void main() {
	
	vec2 uv = TexCoords;
	uv *= 2.0;
	uv -= 1.0;

    float offset = 0.01 - (0.001/scale);
    float radius = 1.0 - offset;
    float t1 = smoothstep(radius-offset, radius, length(uv));
    float t2 = smoothstep(radius, radius+offset, length(uv));

	float outline = t1-t2;
	if (outline < 0.9) discard;
	vec4 white =  vec4(1.0);
	vec4 color = outline * white;
	fragColor = mix(color, vec4(0.0), 0.5);
}