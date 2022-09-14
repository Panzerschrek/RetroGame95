#include "Matrix.hpp"

namespace
{

fixed16vec3_t Vec3Scale(const fixed16vec3_t& v, const fixed16_t scale)
{
	return
	{
		Fixed16Mul(v[0], scale),
		Fixed16Mul(v[1], scale),
		Fixed16Mul(v[2], scale),
	};
}

fixed16vec3_t Vec3Cross(const fixed16vec3_t& l, const fixed16vec3_t& r)
{
	return
	{
		fixed16_t(((int64_t(l[1]) * int64_t(r[2])) - (int64_t(l[2]) * int64_t(r[1]))) >> g_fixed16_base),
		fixed16_t(((int64_t(l[2]) * int64_t(r[0])) - (int64_t(l[0]) * int64_t(r[2]))) >> g_fixed16_base),
		fixed16_t(((int64_t(l[0]) * int64_t(r[1])) - (int64_t(l[1]) * int64_t(r[0]))) >> g_fixed16_base),
	};
}

} // namespace

Matrix3::Matrix3()
{
	x = {g_fixed16_one, 0, 0};
	y = {0, g_fixed16_one, 0};
	z = {0, 0, g_fixed16_one};
}

Matrix3::Matrix3(const fixed16vec3_t& in_x, const fixed16vec3_t& in_y, const fixed16vec3_t& in_z)
	: x(in_x), y(in_y), z(in_z)
{
}

fixed16_t Matrix3::GetDeterminant() const
{
	const int64_t res =
		  (int64_t(x[0]) *
				((int64_t(y[1]) * int64_t(z[2]) - int64_t(z[1]) * int64_t(y[2])) >> g_fixed16_base))
		- (int64_t(y[0]) *
				((int64_t(x[1]) * int64_t(z[2]) - int64_t(z[1]) * int64_t(x[2])) >> g_fixed16_base))
		+ (int64_t(z[0]) *
				((int64_t(x[1]) * int64_t(y[2]) - int64_t(y[1]) * int64_t(x[2])) >> g_fixed16_base));
	return fixed16_t(res >> g_fixed16_base);
}

Matrix3 Matrix3::GetTranspose() const
{
	return Matrix3(
		{x[0], y[0], z[0]},
		{x[1], y[1], z[1]},
		{x[2], y[2], z[2]});
}

Matrix3 Matrix3::GetInverse() const
{
	const fixed16_t det = GetDeterminant();
	const fixed16_t inv_det = Fixed16Invert(det);

	return Matrix3(
		Vec3Scale(Vec3Cross(y, z), inv_det),
		Vec3Scale(Vec3Cross(z, x), inv_det),
		Vec3Scale(Vec3Cross(x, y), inv_det)).GetTranspose();
}
