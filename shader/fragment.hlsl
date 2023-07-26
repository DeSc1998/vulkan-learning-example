

#pragma fragment main

struct v2p {
  float4 pos : SV_POSITION;
  float4 color : COLOR;
};

float4 main(v2p input) {
  return lerp(input.color, float4(0, 0, 0, 1), 1 - input.pos.z);
}