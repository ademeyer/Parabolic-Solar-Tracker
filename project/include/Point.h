#pragma once

struct Point2f
{
  double X;
  double Y;
};

struct Point3f
{
  double X;
  double Y;
  double Z;
  Point3f() : X(0.0), Y(0.0), Z(0.0) {}
  Point3f(const double &x, const double &y, const double &z) : X(x), Y(y), Z(z) {}
};
