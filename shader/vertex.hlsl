
#pragma vertex main

struct camera {
  float4 position;
  float4 direction;
};

uniform camera cam;

struct info {
  float3 pos : POSITION;
  float4 color : COLOR;
  float time : PSIZE;
};

struct v2p {
  float4 pos : SV_POSITION;
  float4 color : COLOR;
};

float4 rotate_y(float4 pos, float4 axes, float angle) {
  float4x4 rotate =
      float4x4(float4(cos(angle), 0, sin(angle), 0), float4(0, 1, 0, 0),
               float4(-sin(angle), 0, cos(angle), 0), float4(0, 0, 0, 1));

  return rotate * (pos - axes) + axes;
}

float2 cosines_to_default(float4 input) {
  // const float4 default_ = float4(0, 0, 1, 0);
  float len_in_xz = length(input.xz);
  float len_in_yz = length(input.yz);
  float cx = input.z / len_in_yz;
  float cy = input.z / len_in_xz;
  return float2(cx, cy);
}

float4x4 rotation_matrix(float2 cosines) {
  float sx = sin(acos(cosines.x));
  float sy = sin(acos(cosines.y));
  float cx = cosines.x;
  float cy = cosines.y;

  float4x4 rot_x = {float4(1, 0, 0, 0), float4(0, cx, sx, 0),
                    float4(0, -sx, cx, 0), float4(0, 0, 0, 1)};
  float4x4 rot_y = {float4(cy, 0, sy, 0), float4(0, 1, 0, 0),
                    float4(-sy, 0, cy, 0), float4(0, 0, 0, 1)};

  return rot_y * rot_x;
}

float4x4 world_to_view(float FOVx, float FOVy, float near, float far) {
  return float4x4(float4(atan(FOVx / 2), 0, 0, 0),
                  float4(0, atan(FOVy / 2), 0, 0),
                  float4(0, 0, -2 / (far - near), -(far + near) / (far - near)),
                  float4(0, 0, 0, 1));
}

v2p main(info input) {
  v2p output;

  float pi = 3.14159265358979323846;
  float4 p = float4(input.pos, 1);
  float4 rot = rotate_y(p, float4(0, 0, 0, 0), pi / 2 * input.time);

  float2 cos = cosines_to_default(cam.direction);
  float4x4 view = rotation_matrix(cos);
  float4x4 proj = world_to_view(pi / 3, pi / 3, 0.3, 4);

  output.pos = view * proj * (rot - cam.position);
  output.color = input.color;
  return output;
}
