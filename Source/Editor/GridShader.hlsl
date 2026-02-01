struct VS_INPUT {
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

cbuffer SceneBuffer : register(b0) {
    matrix viewProj;
};

PS_INPUT VSMain(VS_INPUT input) {
    PS_INPUT output;
    output.pos = mul(float4(input.pos, 1.0f), viewProj);
    output.color = input.color;
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET {
    return input.color;
}
