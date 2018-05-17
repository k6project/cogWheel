/******************************************************************************/

layout(std140) uniform globalParams
{
    mat4 projection;
    mat4 viewTransform;
}

layout(std140) uniform localParams
{
    mat4 modelTransform;
    mat4 normalTransform;
}

/******************************************************************************/

#ifdef _VS_

out stageOut
{
    vec3 worldPposition;
    vec3 worldNormal;
    vec4 vertexColor;
};

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec4 color;

void vsMain()
{
    vec4 localToWorld = modelTransform * vec4(position, 1.0);
    gl_Position = projection * viewTransform * localToWorld;
    worldNormal = (normalTransform * vec4(normal, 0.0)).xyz;
    worldPosition = localToWorld.xyz; 
    vertexColor = color;
}

#endif

/******************************************************************************/

#ifdef _PS_

in stageIn
{
    vec3 worldPposition;
    vec3 worldNormal;
    vec4 vertexColor;
};

layout(location=0) out vec4 outColor0;

void psMain()
{
    outColor0 = vertexColor;
}

#endif

/******************************************************************************/
