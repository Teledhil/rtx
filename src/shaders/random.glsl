// Generate a random unsigned int from 2 other values using 16 iterations of
// the Tiny Encription Algorithm. See nVidia ray tracing tutorial. See Zafar,
// Olano and Curtis, "GPU Random Numbers via the Tiny Encription Algorithm".
//
uint tea(uint v, uint k) {
  uint v0 = v;
  uint v1 = k;
  uint sum = 0;

  [[unroll]] for (uint i = 0; i < 16; i++) {
    sum += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + sum) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7e95761e);
  }

  return v0;
}

// Generate a random unsigned int in [0, 2^24) using Linear Congruential
// Generator.
//
uint random_uint(inout uint seed) {
  uint accumulator = 1664525;
  uint increment = 1013904223;

  seed = accumulator * seed + increment;
  return seed & 0x00FFFFFF;
}

// Generate a random float in [0, 1) using LCG.
//
float random_float(inout uint seed) {
  return float(random_uint(seed)) / float(0x01000000);
}

// Generate a random unit vector.
//
vec3 random_unit_vector(inout uint seed) {
  return vec3(random_float(seed), random_float(seed), random_float(seed));
}
