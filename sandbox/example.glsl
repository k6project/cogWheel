#ifdef _VS_
#define INTERP(n) out n
#endif

#ifdef _FS_
#define INTERP(n) in n
#endif

#ifndef _CS_

INTERP(interpolated)
{
	vec3 world_position;
	vec3 world_normal;
	vec4 vertex_color;
};

#endif

layout(std140) uniform global_params
{
	mat4 projection;
	mat4 view_transform;
} global;

layout(std140) uniform local_params
{
	mat4 model_transform;
	mat4 normal_transform;
} local;

#ifdef _VS_

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec4 color;

void main()
{
	vec4 local_to_world = local.model_transform * vec4(position, 1.0);
	gl_Position = global.projection * global.view_transform * local_to_world;
	world_normal = (local.normal_transform * vec4(normal, 0.0)).xyz;
	world_position = local_to_world.xyz; 
	vertex_color = color;
}

#endif

#ifdef _CS_

void main()
{
	// Compute shader code
}

#endif

#ifdef _FS_

layout(location=0) out vec4 out_color0;

void main()
{
	out_color0 = vertex_color;
}

#endif
