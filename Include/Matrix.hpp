// #ifndef __MATRIX_HPP__
// #define __MATRIX_HPP__
// #include <cmath>
// #include "Point.h"

// struct Mat4
// {
//   double m[4][4];
//   Mat4()
//   {
//     for (int i = 0; i < 4; ++i)
//     {
//       for (int j = 0; j < 4; ++j)
//         m[i][j] = (i == j) ? 1.0 : 0.0; // Identity matrix initialization
//     }
//   }

//   /* Matrix multiplication */
//   Mat4 operator*(const Mat4 &rhs) const
//   {
//     Mat4 result;
//     for (int i = 0; i < 4; ++i)
//     {
//       for (int j = 0; j < 4; ++j)
//       {
//         result.m[i][j] = 0.0;
//         for (int k = 0; k < 4; ++k)
//         {
//           result.m[i][j] += m[i][k] * rhs.m[k][j];
//         }
//       }
//     }
//     return result;
//   }

//   /* Apply the transformation to a 3D point (assuming w=1 for homogeneous coordinates) */
//   Point3f TransformPoint(const Point3f &p) const
//   {
//     double x = m[0][0] * p.X + m[0][1] * p.Y + m[0][2] * p.Z + m[0][3];
//     double y = m[1][0] * p.X + m[1][1] * p.Y + m[1][2] * p.Z + m[1][3];
//     double z = m[2][0] * p.X + m[2][1] * p.Y + m[2][2] * p.Z + m[2][3];
//     double w = m[3][0] * p.X + m[3][1] * p.Y + m[3][2] * p.Z + m[3][3];
//     if (w != 0.0)
//     {
//       x /= w;
//       y /= w;
//       z /= w;
//     }
//     return Point3f(x, y, z);
//   }
// };

// /* Example rotation around X-axis */
// Mat4 RotationX(const double &angle)
// {
//   Mat4 R;
//   double c = cos(angle), s = sin(angle);
//   R.m[1][1] = c;
//   R.m[1][2] = -s;
//   R.m[2][1] = s;
//   R.m[2][2] = c;
//   return R;
// }

// /* Example rotation around Y-axis */
// Mat4 RotationY(const double &angle)
// {
//   Mat4 R;
//   double c = cos(angle), s = sin(angle);
//   R.m[0][0] = c;
//   R.m[0][2] = s;
//   R.m[2][0] = -s;
//   R.m[2][2] = c;
//   return R;
// }

// /* Example rotation around Z-axis */
// Mat4 RotationZ(const double &angle)
// {
//   Mat4 R;
//   double c = cos(angle), s = sin(angle);
//   R.m[0][0] = c;
//   R.m[0][1] = -s;
//   R.m[1][0] = s;
//   R.m[1][1] = c;
//   return R;
// }

// /* Translation matrix (dx, dy, dz) */
// Mat4 Translation(double dx, double dy, double dz)
// {
//   Mat4 T;
//   T.m[0][3] = dx;
//   T.m[1][3] = dy;
//   T.m[2][3] = dz;
//   return T;
// }

// #endif // __MATRIX_HPP__