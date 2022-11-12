#version 460

#define SIZE 1024

layout(local_size_x = SIZE, local_size_y = 1) in;

layout(std430, binding = 0) buffer Data {
	vec4 Position;
	vec4 Gravity;
	float Resistance;
	float dt;
	int LiveCount;

	float MinAngle;
	float MaxAngle;
	float Speed;
	float SpeedVariancePc;

	float LifeSpan;
	float LifeSpanVariancePc;

	float BirthRate;
	float RemainingBirthRate;
	float SpawnCount;
	mat4 Initial;
};

struct Particle {
	vec4 Position;
	vec4 Velocity;
	float Life;
	float MaxLife;
	float CamDist, pad;
};

layout(std430, binding = 1) buffer ParticlesData { 
	Particle Particles[];
};

shared int s[SIZE];

float rand(vec2 co) {
	return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
	uint i = gl_GlobalInvocationID.x;
	if (i<LiveCount) {
		Particles[i].Life -= dt;
		Particles[i].Velocity *= (1.0f - (1.0f-Resistance) * dt);
		Particles[i].Velocity += Gravity * dt;
		Particles[i].Position += Particles[i].Velocity * dt;
	}
	//random = fract(random + dt);
}