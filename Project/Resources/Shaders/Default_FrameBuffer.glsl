#VERTEX_SHADER
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoords;

uniform mat4 MVP;

out vec2 TexCoords;

void main() {
	TexCoords = texCoords;
	TexCoords.y = 1.0-texCoords.y;
	gl_Position = MVP*vec4(position.xy, 0.0, 1.0);
}

#FRAGMENT_SHADER
#version 330 core

layout(location = 0) out vec4 fragColor;
uniform vec2 iResolution;

uniform sampler2D screenTexture;
uniform int shouldTonemap;

in vec2 TexCoords;

// ACES fitted
// from https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

const mat3 ACESInputMat = mat3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat = mat3(
     1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec4 ACESFitted(vec3 color)
{
    color = color * ACESInputMat;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = color * ACESOutputMat;

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return vec4(color, 1.0);
}

vec4 reinhard(vec4 color) {
	const float gamma = 2.2;
	vec3 hdrColor = color.rgb;
  
	// reinhard tone mapping
	vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
	// gamma correction 
	mapped = pow(mapped, vec3(1.0 / gamma));
  
	return vec4(mapped, 1.0);
}

vec4 contrast(vec4 color, float c) {
    float rC = c/2.0;
    return vec4(smoothstep(rC, 1.0-rC, color.rgb), 1.0);
}

void main() {
	vec4 tColor = texture(screenTexture, TexCoords);
    //if (tColor.r < 1.0 && tColor.g < 1.0 && tColor.b < 1.0) discard; HDR Test
	//fragColor = tColor;//vec4(tColor.xyz, 1.0);

	fragColor = shouldTonemap == 1 ? ACESFitted(tColor.rgb) : tColor;

}