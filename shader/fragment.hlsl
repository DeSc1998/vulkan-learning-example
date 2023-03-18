

#pragma fragment main

struct v2p {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 main(v2p input) {
    return input.color;
}