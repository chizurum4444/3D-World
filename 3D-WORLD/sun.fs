#version 330 core
/*
 *  Simple fragment sharder for laboratory two
 */

in vec3 normal;
out vec4 colour; 

void main() {
	vec3 N;
	vec3 L = vec3(1.0, 1.0, 0.0);
	colour = vec4(1.0, 1.0, 0.0, 1.0);
	float diffuse;

	N = normalize(normal);
	L = normalize(L);
	diffuse = dot(N,L);
	if(diffuse < 0.0) {
		diffuse = 0.0;
	}

	colour = min(0.3*colour + 0.7*diffuse*colour, vec4(1.0));
	colour.a = 1.0;
}