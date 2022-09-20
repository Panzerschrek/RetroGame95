#include "GamesCommon.hpp"

bool MakeCollisionBetweenObjectAndBox(
	const fixed16vec2_t& box_min,
	const fixed16vec2_t& box_max,
	const fixed16vec2_t& object_half_size,
	fixed16vec2_t& object_position,
	fixed16vec2_t& object_velocity)
{
	// Replace box<-> box collision with extended box<->point collision.

	const fixed16vec2_t borders_min =
	{
		box_min[0] - object_half_size[0],
		box_min[1] - object_half_size[1],
	};
	const fixed16vec2_t borders_max =
	{
		box_max[0] + object_half_size[0],
		box_max[1] + object_half_size[1],
	};

	if( object_position[0] <= borders_min[0] || object_position[0] >= borders_max[0] ||
		object_position[1] <= borders_min[1] || object_position[1] >= borders_max[1])
	{
		return false;
	}

	// Object intersectss with this box. Try to push it.
	// Find closest intersection of negative velocity vector and extended box side in order to do this.

	int64_t closest_square_dist = 0x7FFFFFFFFFFFFFFF;
	fixed16vec2_t closest_position = object_position;
	std::array<int32_t, 2> bounce_vec = {0, 0};
	const fixed16_t vel_div_clamp = g_fixed16_one / 256; // Avoid overflow in division.
	if(object_velocity[0] > 0)
	{
		const fixed16vec2_t intersection_pos
		{
			borders_min[0],
			object_position[1] -
				Fixed16MulDiv(
					object_position[0] - borders_min[0],
					object_velocity[1],
					std::max(object_velocity[0], vel_div_clamp)),
		};
		const fixed16vec2_t vec_to_intersection_pos
		{
			object_position[0] - intersection_pos[0],
			object_position[1] - intersection_pos[1]
		};
		const int64_t square_dist = Fixed16VecSquareLenScaled(vec_to_intersection_pos);
		if(square_dist < closest_square_dist)
		{
			closest_square_dist = square_dist;
			closest_position = intersection_pos;
			bounce_vec = {1, 0};
		}
	}
	else if(object_velocity[0] < 0)
	{
		const fixed16vec2_t intersection_pos
		{
			borders_max[0],
			object_position[1] +
				Fixed16MulDiv(
					borders_max[0] - object_position[0],
					object_velocity[1],
					std::min(object_velocity[0], -vel_div_clamp)),
		};
		const fixed16vec2_t vec_to_intersection_pos
		{
			object_position[0] - intersection_pos[0],
			object_position[1] - intersection_pos[1]
		};
		const int64_t square_dist = Fixed16VecSquareLenScaled(vec_to_intersection_pos);
		if(square_dist < closest_square_dist)
		{
			closest_square_dist = square_dist;
			closest_position = intersection_pos;
			bounce_vec = {1, 0};
		}
	}

	if(object_velocity[1] > 0)
	{
		const fixed16vec2_t intersection_pos
		{
			object_position[0] -
				Fixed16MulDiv(
					object_position[1] - borders_min[1],
					object_velocity[0],
					std::max(object_velocity[1], vel_div_clamp)),
			borders_min[1],
		};
		const fixed16vec2_t vec_to_intersection_pos
		{
			object_position[0] - intersection_pos[0],
			object_position[1] - intersection_pos[1]
		};
		const int64_t square_dist = Fixed16VecSquareLenScaled(vec_to_intersection_pos);
		if(square_dist < closest_square_dist)
		{
			closest_square_dist = square_dist;
			closest_position = intersection_pos;
			bounce_vec = {0, 1};
		}
	}
	else if(object_velocity[1] < 0)
	{
		const fixed16vec2_t intersection_pos
		{
			object_position[0] +
				Fixed16MulDiv(
					borders_max[1] - object_position[1],
					object_velocity[0],
					std::min(object_velocity[1], -vel_div_clamp)),
			borders_max[1],
		};
		const fixed16vec2_t vec_to_intersection_pos
		{
			object_position[0] - intersection_pos[0],
			object_position[1] - intersection_pos[1]
		};
		const int64_t square_dist = Fixed16VecSquareLenScaled(vec_to_intersection_pos);
		if(square_dist < closest_square_dist)
		{
			closest_square_dist = square_dist;
			closest_position = intersection_pos;
			bounce_vec = {0, 1};
		}
	}

	if(bounce_vec[0] != 0)
	{
		object_position[0] = 2 * closest_position[0] - object_position[0];
		object_velocity[0] = -object_velocity[0];
	}
	if(bounce_vec[1] != 0)
	{
		object_position[1] = 2 * closest_position[1] - object_position[1];
		object_velocity[1] = -object_velocity[1];
	}

	return true;
}