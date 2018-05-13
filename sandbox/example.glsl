#if defined _VS_
#define INTERP(n) out n
#elif defined _FS_
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

in vertex_attr
{
	vec3 position;
	vec3 normal;
	vec4 color;
} in_vertex;

void main()
{
	world_position = local.model_transform * vec4(in_vertex.position, 1.0); 
	gl_Position = global.projection * global.view_transform * world_position;
	world_normal = (local.normal_transform * vec4(in_vertex.normal, 1.0)).xyz;
	vertex_color = in_vertex.color;
}

#endif

#ifdef _CS_

void main()
{
	// Compute shader code
}

#endif

#ifdef _FS_

in vertex_interp
{
	vec3 world_position;
	vec4 color;
} in_vertex;

out vec4 out_color0;

void main()
{
	out_color0 = vertex_color;
}

#endif
