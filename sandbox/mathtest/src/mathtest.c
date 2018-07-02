#include <core/math.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct _rt_context_t_
{
	vec3f_t eye;
	vec2f_t plane;
	vec2u_t res;
	vec2u_t tile;
} rt_context_t;

void cast_rays(rt_context_t* ctx)
{
	int32_t xMax = (ctx->res[0] >> 1) & INT32_MAX, xMin = -xMax;
	int32_t yMax = (ctx->res[1] >> 1) & INT32_MAX, yMin = -yMax;
	float xNorm = ctx->plane[0] / ((float)ctx->res[0]);
	float yNorm = ctx->plane[1] / ((float)ctx->res[1]);
	for (int32_t y = yMin,  r = 0; y < yMax; y++, r++)
	{
		float fy = (y + 0.5f) * yNorm;
		uint32_t tile_y = r / ctx->tile[1];
		for (int32_t x = xMin; x < xMax; x++)
		{
			float fx = (x + 0.5f) * xNorm;
			vec3f_t pos = { fx, fy, 0.f };
			vec3f_sub(pos, pos, ctx->eye);
			float norm = 1.f / vec3f_len(pos);
			vec3f_muls(pos, pos, norm);
		}
	}
}

int main(int argc, const char** argv)
{
	rt_context_t ctx =
	{
		.eye = { 0.f, 0.f, -2.f },
		.plane = { 2.f, 2.f },
		.res = { 16, 16 },
		.tile = { 8, 8 }
	};
	cast_rays(&ctx);
	return 0;
}
