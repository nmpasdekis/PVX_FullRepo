#version 460

#define SIZE 1024

layout(local_size_x = SIZE, local_size_y = 1) in;

layout(std430, binding = 0) buffer Data {
	vec4 Position;
	vec4 Gravity;
	float Resistance;
	float dt;
	uint LiveCount;

	float MinAngle;
	float MaxAngle;

	float MinSpeed;
	float MaxSpeed;

	float MinLifeSpan;
	float MaxLifeSpan;

	float BirthRate;
	float RemainingBirthRate;
	float Random;

	vec2 MinScale;
	vec2 MaxScale;
	float camX, camY, camZ;
	float MaxAngularVelocity;
	mat4 Initial;
};

struct Particle {
	vec4 Position;
	vec4 Velocity;
	float Rotation;
	float AngularVelocity;
	float Life;
	float MaxLife;
	float CamDist2, pad;
	vec2 Scale;
};

layout(std430, binding = 1) buffer ParticlesData {
	Particle Particles[];
};

shared uint scatter[SIZE];
shared float dist2[SIZE];


bool PredFnc(uint i1, uint i2) {
	return dist2[i1] > dist2[i2];
	//return dist2[scatter[i1]] > dist2[scatter[i2]];
}

void bitonic_sort_step(uint i, uint j, uint k) {
	uint ixj = i^j;

	if ((ixj)>i) {
		if ((i&k)==0) {
			if (!PredFnc(i, ixj)) {
				uint temp = scatter[i];
				scatter[i] = scatter[ixj];
				scatter[ixj] = temp;

				float temp2 = dist2[i];
				dist2[i] = dist2[ixj];
				dist2[ixj] = temp2;
			}
		} else {
			if (PredFnc(i, ixj)) {
				uint temp = scatter[i];
				scatter[i] = scatter[ixj];
				scatter[ixj] = temp;

				float temp2 = dist2[i];
				dist2[i] = dist2[ixj];
				dist2[ixj] = temp2;
			}
		}
	}
}

float rndSeed;
vec2 co;

float rand() {
	return rndSeed = sin(dot(co.xy, vec2(12.9898, 78.233 + rndSeed)) * 43758.5453)*0.5+0.5;
}

float length2(vec3 x) {
	return dot(x, x);
}

void main() {
	uint i = gl_GlobalInvocationID.x;
	uint i2 = i<<1;
	//float rnd = Random;
	vec3 CamPos = vec3(camX, camY, camZ);
	float SpawnDist2 = length2(CamPos - Position.xyz);

	co = vec2(i, dt);
	rndSeed = Random;

///////////////// Begin Update ////////////////////

	if (i<LiveCount) {
		Particles[i].Life -= dt;
		Particles[i].Velocity *= (1.0f - (1.0f-Resistance) * dt);
		Particles[i].Velocity += Gravity * dt;
		Particles[i].Position += Particles[i].Velocity * dt;
		Particles[i].Rotation += Particles[i].AngularVelocity * dt;
		Particles[i].CamDist2 = length2(CamPos - Particles[i].Position.xyz);
	}

///////////////// End Update ////////////////////

///////////////// Begin Compact /////////////////

	uint pred = scatter[i] = int(1.0 - step(0, -Particles[i].Life));
	Particle myParticle = Particles[i];
	uint offset = 1;
	uint n = SIZE;
	//uint n = LiveCount;
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
			uint t = scatter[ai];
			scatter[ai] = scatter[bi];
			scatter[bi] += t;
		}
	}
	barrier();
	Particles[i].Life = 0;
	if (pred>0) 
		Particles[scatter[i]] = myParticle;
	
	barrier();
///////////////// End Compact /////////////////

///////////////// Begin Spawn /////////////////

	n = i + LiveCount;
	float sc = BirthRate * dt + RemainingBirthRate;
	if (i + 1 < sc && n < SIZE) {
		float Pitch = mix(MinAngle, MaxAngle, rand());
		float Yaw = rand() * 100.0f;

		float cy = cos(Yaw);
		float sy = sin(Yaw);
		float cp = cos(Pitch);
		float sp = sin(Pitch);

		Particles[n].Position = Position;
		Particles[n].Velocity = vec4(sp*sy, cp, cy*sp, 1) * Initial *
			mix(MinSpeed, MaxSpeed, rand());

		Particles[n].Life = Particles[n].MaxLife = mix(MinLifeSpan, MaxLifeSpan, rand());

		Particles[n].Rotation = 0;
		Particles[n].AngularVelocity = MaxAngularVelocity * (rand() * 2.0 - 1.0);
		Particles[n].CamDist2 = SpawnDist2;
		Particles[n].Scale = mix(MinScale, MaxScale, rand());
	}
	if (i==0) 	{
		LiveCount = min(LiveCount + int(sc), SIZE);
		RemainingBirthRate = fract(sc);
	}

///////////////// End Spawn /////////////////

///////////////// Begin Sort ////////////////

	if (i>=LiveCount)
		Particles[i].CamDist2 = 0;
	//Particles[i].CamDist2 *= step(0, int(LiveCount) - i);
	barrier();
	scatter[i] = i;
	dist2[i] = Particles[i].CamDist2;
	barrier();

	for (uint k = 2; k <= SIZE; k <<= 1) {
		for (uint j = k >> 1; j > 0; j >>= 1) {
			bitonic_sort_step(i, j, k);
			barrier();
		}
	}
	myParticle = Particles[scatter[i]];
	barrier();
	Particles[i] = myParticle;

///////////////// End Sort ////////////////
}