#include "Math.hpp"

namespace IW3SR
{
	vec2 GMath::WorldToScreen(const vec3& position)
	{
		const vec2 center = vec2(cgs->refdef.width, cgs->refdef.height) / 2.0f;
		const vec2 tanHalfFov = vec2(cgs->refdef.tanHalfFovX, cgs->refdef.tanHalfFovY);
		const mat3 viewAxis = cgs->refdef.viewaxis;
		const vec3 origin = cgs->refdef.vieworg;

		const vec3 local = position - origin;
		const vec3 forward = viewAxis[0];
		const vec3 right = viewAxis[1];
		const vec3 up = viewAxis[2];

		const vec3 transform = { local.Dot(right), local.Dot(up), local.Dot(forward) };

		if (transform[2] < 0.01f)
			return vec2::Zero;

		return { center.x * (1.0f - (transform[0] / tanHalfFov.x / transform[2])),
			center.y * (1.0f - (transform[1] / tanHalfFov.y / transform[2])) };
	}
}
