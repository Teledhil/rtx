struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 transmittance;
  vec3 emission;
  float shininess;
  float refraction;  // index
  float dissolve;    // 1 == opaque; 0 == fully transparent
  int illumination;
  int texture_id;
};

vec3 compute_diffuse(Material material, vec3 light_direction, vec3 normal) {
  // Lambertian

  if (material.illumination < 1) {
    return vec3(0);
  }

  float dot_normal_light = max(dot(normal, light_direction), 0.0);
  vec3 c = material.diffuse * dot_normal_light;

  return c + material.ambient;
}

vec3 compute_diffuse_lol(Material material, vec3 light_direction, vec3 normal) {
  float dot_normal_light = max(dot(normal, light_direction), 0.0);

  vec3 diffuse = vec3(0.8);

  vec3 c = diffuse * dot_normal_light;

  vec3 ambient = vec3(1);

  return c + ambient;
}

vec3 compute_specular(Material material, vec3 view_direction,
                      vec3 light_direction, vec3 normal) {
  if (material.illumination < 2) {
    return vec3(0);
  }

  const float pi = 3.14159265;
  const float shininess = max(material.shininess, 4.0);

  const float energy_conservation = (2.0 + shininess) / (2.0 * pi);

  vec3 V = normalize(-view_direction);
  vec3 R = reflect(-light_direction, normal);
  float specular = energy_conservation * pow(max(dot(V, R), 0.0), shininess);

  return vec3(material.specular * specular);
}

vec3 compute_specular_lol(Material material, vec3 view_direction,
                          vec3 light_direction, vec3 normal) {
  const float pi = 3.14159265;
  const float shininess = 0.5;

  const float energy_conservation = (2.0 + shininess) / (2.0 * pi);

  vec3 V = normalize(-view_direction);
  vec3 R = reflect(-light_direction, normal);
  float specular = energy_conservation * pow(max(dot(V, R), 0.0), shininess);

  return vec3(0.5 * specular);
}
