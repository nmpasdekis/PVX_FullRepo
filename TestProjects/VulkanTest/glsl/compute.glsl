#version 440

#define SIZE 1024

layout(local_size_x = SIZE, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer bA { float a[]; };

shared uint s[SIZE];

bool PredFnc(uint i1, uint i2) {
    return ((a[s[i1]]>0 || a[s[i2]]>0) && a[s[i1]] > a[s[i2]]);
}

void bitonic_sort_step(uint i, uint j, uint k) {
    uint ixj = i^j;

    if ((ixj)>i) {
        if ((i&k)==0) {
            if (!PredFnc(i,ixj)) {
                uint temp = s[i];
                s[i] = s[ixj];
                s[ixj] = temp;
            }
        }
        else {
            if (PredFnc(i, ixj)) {
                uint temp = s[i];
                s[i] = s[ixj];
                s[ixj] = temp;
            }
        }
    }
}

void main() {
    uint i = gl_GlobalInvocationID.x;
    s[i] = i;
    barrier();
    uint j, k;
    for (k = 2; k<=700; k <<= 1) {
        for (j = k>>1; j > 0; j >>= 1) {
            bitonic_sort_step(i, j, k);
            barrier();
        }
    }
    float tmp = a[s[i]];
    barrier();
    a[i] = tmp;
}
