#version 440

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

shared int scatter[SIZE];

void main() {
	uint i = gl_LocalInvocationID.x;
	uint i2 = i<<1;

	int pred = scatter[i] = int(step(0, Particles[i].Life));
	Particle myParticle = Particles[i];
	uint offset = 1;
	uint n = SIZE;
	barrier();

	for (uint d = n >> 1; d > 0; d >>= 1) {
		if (i < d) {
			uint ai = offset*(i2+1)-1;
			uint bi = offset*(i2+2)-1;
			scatter[bi] += scatter[ai];
		}
		offset <<= 1;
		barrier();
	}

	if (i == 0) {
		LiveCount = scatter[n - 1];
		scatter[n - 1] = 0;
	}

	for (int d = 1; d < n; d <<= 1) {
		offset >>= 1;
		barrier();
		if (i < d) {
			uint ai = offset*(i2+1)-1;
			uint bi = offset*(i2+2)-1;
			int t = scatter[ai];
			scatter[ai] = scatter[bi];
			scatter[bi] += t;
		}
	}
	barrier();

	if (pred>0) {
		Particles[scatter[i]] = myParticle;
	}
}