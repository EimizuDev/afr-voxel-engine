struct VertexOutput
{
	float4 m_svPosition : SV_Position;
};

static float2 s_vertices[6] = float2[]
(
	float2(-1, -1),
	float2(1, -1),
	float2(-1, 1),
	float2(-1, 1),
	float2(1, -1),
	float2(1, 1)
);

struct CameraData {
    column_major float4x4 m_CTWMatrix;
};

struct Brick {
    uint16_t m_voxels[16][16][16];
};

struct VoxelData {
    Brick m_bricks[3][3][3];
    uint16_t m_bricksPerDim;
};

static float s_py = radians(180);
static float3 s_lightColor = float3(1.f, 0.9f, 0.63f);
static float s_lightIntensity = 15.f;

static float s_dLTheta = 10, s_dLPhi = 65;
static float3 s_directionalLight = float3(
    sin(radians(s_dLTheta)) * cos(radians(s_dLPhi)), 
    cos(radians(s_dLTheta)), 
    sin(radians(s_dLTheta)) * sin(radians(s_dLPhi)));

ConstantBuffer<CameraData, Std430DataLayout> camData;
StructuredBuffer<VoxelData, Std430DataLayout> voxData;

[shader("vertex")]
VertexOutput VertMain(uint index : SV_VertexID) 
{
	VertexOutput output;
	output.m_svPosition = float4(s_vertices[index], 0, 1);
	return output;
}

[shader("fragment")]
float4 FragMain(VertexOutput input) : SV_Target
{
    const uint16_t width = 600, height = 600;
    const float aspectRatio = width / height;
    constexpr float FOV = radians(90.f);

    float pixelCamX = (input.m_svPosition.x / width * 2 - 1) * aspectRatio * tan(FOV / 2);
    float pixelCamY = -(1 - 2 * input.m_svPosition.y / height) * tan(FOV / 2);

    float3 rayOrigin = float3(0, 0, 0);
    float3 rayPosCamSpace = float3(pixelCamX, pixelCamY, -1.f);

    float3 rayPosWorld = mul(camData.m_CTWMatrix, float4(rayPosCamSpace, 1)).xyz;
    float3 rayOrigWorld = mul(camData.m_CTWMatrix, float4(rayOrigin, 1)).xyz;

    float3 rayDir = normalize(rayPosWorld - rayOrigWorld);

    // DDA ray tracing
    float3 deltaDist = 1.f / fabs(rayDir);
    int3 voxelMap = int3(floor(rayPosWorld));
    int3 voxelStep = sign(rayDir);

    float3 sideDist = float3(
        (rayDir.x < 0 ? (rayPosWorld.x - voxelMap.x) : (1 - (rayPosWorld.x - voxelMap.x))) * deltaDist.x,
        (rayDir.y < 0 ? (rayPosWorld.y - voxelMap.y) : (1 - (rayPosWorld.y - voxelMap.y))) * deltaDist.y,
        (rayDir.z < 0 ? (rayPosWorld.z - voxelMap.z) : (1 - (rayPosWorld.z - voxelMap.z))) * deltaDist.z
    );

    float maxDistance = 70.f;
    float currentDistance = 0;
    while (currentDistance < maxDistance)
    {
        bool3 stepTaken = bool3(false);

        if (sideDist.x < sideDist.y)
        {
            if (sideDist.x < sideDist.z)
            {
                voxelMap.x += voxelStep.x;
                currentDistance = sideDist.x;
                sideDist.x += deltaDist.x;

                stepTaken.x = true;
            }
            else
            {
                voxelMap.z += voxelStep.z;
                currentDistance = sideDist.z;
                sideDist.z += deltaDist.z;

                stepTaken.z = true;
            }
        }
        else
        {
            if (sideDist.y < sideDist.z)
            {
                voxelMap.y += voxelStep.y;
                currentDistance = sideDist.y;
                sideDist.y += deltaDist.y;

                stepTaken.y = true;
            }
            else
            {
                voxelMap.z += voxelStep.z;
                currentDistance = sideDist.z;
                sideDist.z += deltaDist.z;

                stepTaken.z = true;
            }
        }

        if (voxelMap.x >= 0 && voxelMap.x < 16 && voxelMap.y >= 0 && voxelMap.y < 16 && voxelMap.z >= 0 && voxelMap.z < 16)
        {
            uint16_t centerIndex = (voxData[0].m_bricksPerDim + 1) / 2 - 1;

            if (voxData[0].m_bricks[centerIndex][centerIndex][centerIndex].m_voxels[voxelMap.z][voxelMap.y][voxelMap.x] > 0)
            {
                const float3 faceNormal = float3(stepTaken.x ? -sign(rayDir.x) : 0, stepTaken.y ? -sign(rayDir.y) : 0, stepTaken.z ? -sign(rayDir.z) : 0);
                const float lightRatio = max(0.f, dot(faceNormal, -s_directionalLight));

                const float3 lightingCalc = 1 / s_py * s_lightColor * s_lightIntensity * lightRatio;

                if (voxData[0].m_bricks[centerIndex][centerIndex][centerIndex].m_voxels[voxelMap.z][voxelMap.y][voxelMap.x] == 1) {
                    return float4(float3(1.f, 0.f, 0.f) * lightingCalc, 1.f);
                }
                else if (voxData[0].m_bricks[centerIndex][centerIndex][centerIndex].m_voxels[voxelMap.z][voxelMap.y][voxelMap.x] == 2) {
                    return float4(float3(0.f, 1.f, 0.f) * lightingCalc, 1.f);
                }
            }
        }
    }

	return float4(0.53f, 0.81f, 0.92f, 1.f);
}