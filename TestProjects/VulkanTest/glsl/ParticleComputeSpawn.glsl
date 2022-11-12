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

float rand(vec2 co, inout float rnd) {
	return rnd = fract(sin(dot(co.xy, vec2(12.9898, 78.233 + rnd))) * 43758.5453) * 2.0 - 1.0;
}

void main() {
	float rnd = 0;
	uint i = gl_GlobalInvocationID.x;
	uint n = i + LiveCount;
	float sc = BirthRate * dt + RemainingBirthRate;
	if (i + 1 < sc) {
		float Pitch = MinAngle + (MaxAngle - MinAngle) * 
			rand(vec2(i, dt), rnd);
		float Yaw = rand(vec2(i, dt), rnd) * 100.0f;

		float cy = cos(Yaw);
		float sy = sin(Yaw);
		float cp = cos(Pitch);
		float sp = sin(Pitch);

		Particles[n].Position = Position;
		Particles[n].Velocity = vec4(sp*sy, cp, cy*sp, 1) * Initial *
			Speed * (1.0 + rand(vec2(i, dt), rnd) * SpeedVariancePc);

		Particles[n].Life = Particles[n].MaxLife = LifeSpan * 
			(1.0 + LifeSpanVariancePc * rand(vec2(i, dt), rnd));
	}
	//random = fract(random + dt);
}