#version 330 core

uniform vec3 u_color;

in vec3 normal;
in vec3 world_pos;
in vec4 posLightSpace1;
in vec4 posLightSpace2;
in vec4 posLightSpace3;
in vec4 posLightSpace4;

out vec4 frag_color;

uniform vec3 light_pos;
uniform float ambient_factor;
uniform sampler2D depthMap1;
uniform sampler2D depthMap2;
uniform sampler2D depthMap3;
uniform sampler2D depthMap4;
uniform float shadowStrength;

bool isInShadowMap(vec4 posLightSpace) {
	vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;

	return projCoords.x >= 0.005 && projCoords.y >= 0.005 && projCoords.x <= 0.995 && projCoords.y <= 0.995;
}

float getShadow(vec4 posLightSpace, sampler2D depthMap) {
	vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;
	float currentDepth = projCoords.z;
	float shadow = 0.0;
	
	vec2 texelSize = 1.0 / textureSize(depthMap, 0);

	if (shadowStrength > 0.99f) {
		for (int i = -1; i <=1; ++i) {
			for (int j = -1; j <=1; ++j) {
				float depth = texture(depthMap, projCoords.xy + vec2(i, j) * texelSize).r;
				shadow += currentDepth - 0.001 > depth ? 1.0 / 9.0 : 0.0;
			}
		}	
	}
	else {
		float depth = texture(depthMap, projCoords.xy).r;
		shadow += currentDepth - 0.001 > depth ? 1.0 : 0.0;
	}

	return shadow;
}

void main() {
    vec3 light_col = vec3(1, 1, 1);
	float diffuse_factor = max(0.0f, dot(normal, normalize(light_pos)));
	float shadow;
	
	if (isInShadowMap(posLightSpace1))
		shadow = getShadow(posLightSpace1, depthMap1);
	else if (isInShadowMap(posLightSpace2))
		shadow = getShadow(posLightSpace2, depthMap2);
	else if (isInShadowMap(posLightSpace3))
		shadow = getShadow(posLightSpace3, depthMap3);
	else
		shadow = getShadow(posLightSpace4, depthMap4);

	vec3 light = light_col * clamp(ambient_factor + diffuse_factor * (1 - shadow) * shadowStrength, 0.0f, 1.0f);

    frag_color = vec4(u_color * light, 1.0f);
}