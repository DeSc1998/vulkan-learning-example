
#pragma vertex main

struct info {
  float4 pos : POSITION;
  float4 color : COLOR;
  float time : PSIZE;
};

struct v2p {
  float4 pos : SV_POSITION;
  float4 color : COLOR;
};

float4 rotate_z(float4 pos, float4 axes, float angle) {
  float4x4 rotate = {cos(angle), sin(angle), 0.0, 0.0, -sin(angle), cos(angle),
                     0.0,        0.0,        0.0, 0.0, 1.0,         0.0,
                     0.0,        0.0,        0.0, 1.0};

  return rotate * (pos - axes) + axes;
}

float4 rotate_y(float4 pos, float4 axes, float angle) {
  float4x4 rotate = {cos(angle),  0, sin(angle), 0, 0, 1, 0, 0,
                     -sin(angle), 0, cos(angle), 0, 0, 0, 0, 1};

  return rotate * (pos - axes) + axes;
}

v2p main(info input) {
  v2p output;
  const float pi = 3.14159265358979323846;
  output.pos = rotate_y(input.pos, float4(0.0, 0.0, 0.5, 0.0), pi * input.time);
  output.color = input.color;
  return output;
}
