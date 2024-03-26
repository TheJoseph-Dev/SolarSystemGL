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

uniform vec3 camPos;
uniform vec2 iMouse;
uniform float iTime;

uniform sampler2D textureAtlas;

in vec2 TexCoords;

vec3 rotateZ(vec3 p, float angle) {
    mat3 rotMat = mat3(0.0);
    rotMat[0][0] = cos(angle);
    rotMat[0][1] = -sin(angle);
    rotMat[1][0] = sin(angle);
    rotMat[1][1] = cos(angle);
    rotMat[2][2] = 1.0;
    
    return rotMat*p;
}

vec3 rotateX(vec3 p, float angle) {
    mat3 rotMat = mat3(0.0);
    rotMat[2][2] = cos(angle);
    rotMat[1][2] = -sin(angle);
    rotMat[2][1] = sin(angle);
    rotMat[1][1] = cos(angle);
    rotMat[0][0] = 1.0;
    
    return rotMat*p;
}

vec3 rotateY(vec3 p, float angle) {
    mat3 rotMat = mat3(0.0);
    rotMat[0][0] = cos(angle);
    rotMat[2][0] = -sin(angle);
    rotMat[0][2] = sin(angle);
    rotMat[1][1] = 1.0;
    rotMat[2][2] = cos(angle);
    
    return rotMat*p;
}

float sphereSDF(vec3 p, float r) { return length(p)-r; }

struct Sphere {
	vec3 position;
	float radius;
	// sampler2D texture
};

float map(vec3 rayPoint) {
	const float maxDist = 0.15;
	float minDist = maxDist;

	Sphere[] planets = Sphere[] (
		Sphere(vec3(0.0, sin(iTime)*0.5 + 0.5, 0.0), 0.4)
	);

	for(int i = 0; i < planets.length(); i++) {
		vec3 tRay = rayPoint - planets[i].position;//rotateX(rotateZ(planets[i].position, iTime), iTime);
		float sSDF = sphereSDF(tRay, planets[i].radius);
		minDist = min(minDist, sSDF);
	}

	//float ground = rayPoint.y + 0.1;
	//minDist = min(ground, minDist);

	return minDist;
}

vec3 GetNormal( in vec3 p ) // for function f(p)
{
    const float eps = 0.0001; // or some other value
    const vec2 h = vec2(eps,0);
    return normalize( vec3( map(p+h.xyy) - map(p-h.xyy),
                            map(p+h.yxy) - map(p-h.yxy),
                            map(p+h.yyx) - map(p-h.yyx) ) );
}

#define PI 3.141592653

vec4 raymarching(vec2 uv, out float sceneDepth) {
	const int maxIterDepth = 60;

	float fov = 10.0/10.0;
	vec3 rayOrigin = camPos;
	vec3 rayDir = normalize(vec3(uv, 1.0)*fov);
	rayOrigin = rotateY(rayOrigin, iTime/2.0);
	rayDir = rotateY(rayDir, iTime/2.0);


	//float sceneDepth = 0.0;
	vec4 scene = vec4(0.0);
	for (int i = 0; i < maxIterDepth; i++) {
		vec3 march = rayOrigin + (rayDir * sceneDepth);
		float minMapDist = map(march);
		sceneDepth += minMapDist;

		vec3 normal = GetNormal(march); 
		//vec3 texXY = texture(textureAtlas, march.xy*.5+.5).rgb;
		//vec3 texXZ = texture(textureAtlas, march.xz*.5+.5).rgb;
		//vec3 texYZ = texture(textureAtlas, march.yz*.5+.5).rgb;
		
		vec3 absNormal = abs(normal);//pow(abs(normal), vec3(50));
		//absNormal /= absNormal.x + absNormal.y + absNormal.z;
		//vec4 triplanarProj = vec4(texXY*absNormal.z + texXZ*absNormal.y + texYZ*absNormal.x, 1.0);

		vec2 polarCoord = (vec2((atan(normal.x, normal.z))/PI, normal.y)+1)*0.5;
		vec3 tex = texture(textureAtlas, 1.0-(polarCoord)).rgb;
		vec4 skybox = vec4(0.01, 0.025, 0.1, 1.0);

		scene = vec4(tex, 1.0); //mix(skybox, vec4(tex, 1.0), step(sceneDepth, 1.0));
	}

	return scene;
}

void main() {
	vec2 fragCoord = gl_FragCoord.xy;
	vec2 uv = fragCoord / iResolution * 2.0 - 1.0;
	float aspect = iResolution.x / iResolution.y; 
	uv.x *= aspect;

	float zDepth = 1.0;
	float sceneDepth = 0.0;
	vec4 depth = raymarching(uv, sceneDepth);
	if (step(sceneDepth, 2.0) < 1.0) { discard; }
	fragColor = vec4(depth/zDepth);
}