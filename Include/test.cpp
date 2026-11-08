// #include <iostream>
// #include <memory>
// #include <utility>

// // ---------- Abstract Base Class ----------
// class Shape
// {
// public:
//   virtual ~Shape() = default;

//   // Polymorphic clone for deep copying (essential for value semantics)
//   virtual std::unique_ptr<Shape> clone() const = 0;

//   virtual double area() const = 0;

//   // Optional: for debugging
//   virtual void print() const = 0;
// };

// // ---------- Concrete Component: Circle ----------
// class Circle : public Shape
// {
// private:
//   double radius_;

// public:
//   explicit Circle(double r) : radius_(r) {}

//   // Clone implementation
//   std::unique_ptr<Shape> clone() const override
//   {
//     return std::make_unique<Circle>(*this);
//   }

//   double area() const override
//   {
//     return 3.1415926535 * radius_ * radius_;
//   }

//   void print() const override
//   {
//     std::cout << "Circle(radius=" << radius_ << ")";
//   }
// };

// // ---------- Concrete Component: Rectangle ----------
// class Rectangle : public Shape
// {
// private:
//   double width_, height_;

// public:
//   Rectangle(double w, double h) : width_(w), height_(h) {}

//   std::unique_ptr<Shape> clone() const override
//   {
//     return std::make_unique<Rectangle>(*this);
//   }

//   double area() const override
//   {
//     return width_ * height_;
//   }

//   void print() const override
//   {
//     std::cout << "Rectangle(" << width_ << "x" << height_ << ")";
//   }
// };

// // ---------- Base Decorator ----------
// // Holds a pointer to a Shape, forwards all calls.
// class ShapeDecorator : public Shape
// {
// protected:
//   std::unique_ptr<Shape> wrapped_; // owning pointer to the decorated object

// public:
//   // Constructor takes ownership of a Shape (by pointer or by move)
//   explicit ShapeDecorator(std::unique_ptr<Shape> shape) : wrapped_(std::move(shape)) {}

//   // Copy constructor: deep copy of the wrapped shape
//   ShapeDecorator(const ShapeDecorator &other)
//       : wrapped_(other.wrapped_ ? other.wrapped_->clone() : nullptr) {}

//   // Move constructor: transfer ownership
//   ShapeDecorator(ShapeDecorator &&other) noexcept = default;

//   // Copy assignment using copy‑and‑swap (safe and efficient)
//   ShapeDecorator &operator=(ShapeDecorator other) noexcept
//   {
//     swap(*this, other);
//     return *this;
//   }

//   // Swap for copy‑and‑swap idiom
//   friend void swap(ShapeDecorator &lhs, ShapeDecorator &rhs) noexcept
//   {
//     using std::swap;
//     swap(lhs.wrapped_, rhs.wrapped_);
//   }

//   // Clone for the decorator itself (returns a new decorator with a cloned wrapped shape)
//   std::unique_ptr<Shape> clone() const override
//   {
//     return std::make_unique<ShapeDecorator>(wrapped_ ? wrapped_->clone() : nullptr);
//   }

//   // Forward area to wrapped shape
//   double area() const override
//   {
//     return wrapped_ ? wrapped_->area() : 0.0;
//   }

//   void print() const override
//   {
//     if (wrapped_)
//       wrapped_->print();
//     else
//       std::cout << "null";
//   }
// };

// // ---------- Concrete Decorator: ScaleDecorator ----------
// // Multiplies the area of the wrapped shape by a scaling factor.
// class ScaleDecorator : public ShapeDecorator
// {
// private:
//   double factor_;

// public:
//   ScaleDecorator(std::unique_ptr<Shape> shape, double factor)
//       : ShapeDecorator(std::move(shape)), factor_(factor) {}

//   // Copy constructor (must deep copy base and factor)
//   ScaleDecorator(const ScaleDecorator &other)
//       : ShapeDecorator(other), // calls ShapeDecorator copy ctor (deep copy)
//         factor_(other.factor_)
//   {
//   }

//   // Move constructor
//   ScaleDecorator(ScaleDecorator &&other) noexcept = default;

//   // Copy assignment using copy‑and‑swap (reuses base swap and adds factor)
//   ScaleDecorator &operator=(ScaleDecorator other) noexcept
//   {
//     swap(*this, other);
//     return *this;
//   }

//   friend void swap(ScaleDecorator &lhs, ScaleDecorator &rhs) noexcept
//   {
//     using std::swap;
//     swap(static_cast<ShapeDecorator &>(lhs), static_cast<ShapeDecorator &>(rhs));
//     swap(lhs.factor_, rhs.factor_);
//   }

//   std::unique_ptr<Shape> clone() const override
//   {
//     return std::make_unique<ScaleDecorator>(*this);
//   }

//   double area() const override
//   {
//     // Scale the wrapped shape's area
//     return ShapeDecorator::area() * factor_;
//   }

//   void print() const override
//   {
//     std::cout << "ScaleDecorator(factor=" << factor_ << ", ";
//     ShapeDecorator::print();
//     std::cout << ")";
//   }
// };

// // ---------- Another Concrete Decorator: ColorDecorator (adds name, no area change) ----------
// class ColorDecorator : public ShapeDecorator
// {
// private:
//   std::string color_;

// public:
//   ColorDecorator(std::unique_ptr<Shape> shape, std::string color)
//       : ShapeDecorator(std::move(shape)), color_(std::move(color)) {}

//   ColorDecorator(const ColorDecorator &other) = default;
//   ColorDecorator(ColorDecorator &&other) noexcept = default;
//   ColorDecorator &operator=(ColorDecorator other) noexcept
//   {
//     swap(*this, other);
//     return *this;
//   }
//   friend void swap(ColorDecorator &lhs, ColorDecorator &rhs) noexcept
//   {
//     using std::swap;
//     swap(static_cast<ShapeDecorator &>(lhs), static_cast<ShapeDecorator &>(rhs));
//     swap(lhs.color_, rhs.color_);
//   }

//   std::unique_ptr<Shape> clone() const override
//   {
//     return std::make_unique<ColorDecorator>(*this);
//   }

//   double area() const override
//   {
//     return ShapeDecorator::area(); // no change
//   }

//   void print() const override
//   {
//     std::cout << "ColorDecorator(color=" << color_ << ", ";
//     ShapeDecorator::print();
//     std::cout << ")";
//   }
// };

// // ---------- Demonstration ----------
// int main()
// {
//   // Create a simple circle
//   std::unique_ptr<Shape> circle = std::make_unique<Circle>(5.0);
//   std::cout << "Original: ";
//   circle->print();
//   std::cout << " area=" << circle->area() << "\n";

//   // Decorate it with scaling – still a Shape
//   std::unique_ptr<Shape> scaled = std::make_unique<ScaleDecorator>(std::move(circle), 2.0);
//   std::cout << "Scaled: ";
//   scaled->print();
//   std::cout << " area=" << scaled->area() << "\n";

//   // Decorate further with color – still a Shape
//   std::unique_ptr<Shape> colored_scaled = std::make_unique<ColorDecorator>(std::move(scaled), "red");
//   std::cout << "Colored+Scaled: ";
//   colored_scaled->print();
//   std::cout << " area=" << colored_scaled->area() << "\n";

//   // Deep copy the decorated object (clone)
//   auto copy = colored_scaled->clone(); // returns unique_ptr<Shape>
//   std::cout << "Copy of colored+scaled: ";
//   copy->print();
//   std::cout << " area=" << copy->area() << "\n";

//   // Move assignment: assign a different decorated shape to another Shape pointer
//   std::unique_ptr<Shape> another = std::make_unique<ScaleDecorator>(
//       std::make_unique<Rectangle>(3, 4), 1.5);
//   std::cout << "\nBefore move: ";
//   another->print();
//   std::cout << " area=" << another->area() << "\n";

//   // Move assignment – works because both are unique_ptr<Shape>
//   another = std::move(colored_scaled);
//   std::cout << "After move: ";
//   another->print();
//   std::cout << " area=" << another->area() << "\n";

//   return 0;
// }

#include <utility>
#include <cmath>
#include <iostream>

struct Vector3
{
  double x, y, z;

  Vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

  double norm() const
  {
    return std::sqrt(x * x + y * y + z * z);
  }

  Vector3 normalized() const
  {
    double n = norm();
    return Vector3(x / n, y / n, z / n);
  }

  Vector3 cross(const Vector3 &other) const
  {
    return Vector3(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x);
  }

  double dot(const Vector3 &other) const
  {
    return x * other.x + y * other.y + z * other.z;
  }
};

// Rodrigues rotation formula: rotate vector v around axis u by angle theta
Vector3 rotateVector(const Vector3 &v, const Vector3 &u, double theta)
{
  double cosTheta = std::cos(theta);
  double sinTheta = std::sin(theta);

  Vector3 uNorm = u.normalized();

  // v_rot = v*cosθ + (u×v)*sinθ + u(u·v)(1-cosθ)
  Vector3 crossProd = uNorm.cross(v);
  double dotProd = uNorm.dot(v);

  return Vector3(
      v.x * cosTheta + crossProd.x * sinTheta + uNorm.x * dotProd * (1 - cosTheta),
      v.y * cosTheta + crossProd.y * sinTheta + uNorm.y * dotProd * (1 - cosTheta),
      v.z * cosTheta + crossProd.z * sinTheta + uNorm.z * dotProd * (1 - cosTheta));
}

// Get rotation that aligns 'from' vector to 'to' vector
// Returns rotation axis and angle
void getAlignmentRotation(const Vector3 &from, const Vector3 &to,
                          Vector3 &axis, double &angle)
{
  Vector3 fromNorm = from.normalized();
  Vector3 toNorm = to.normalized();

  // Check if vectors are already aligned
  double dot = fromNorm.dot(toNorm);

  if (dot > 0.99999)
  {
    // Already aligned - no rotation
    axis = Vector3(1, 0, 0);
    angle = 0;
    return;
  }

  if (dot < -0.99999)
  {
    // Opposite directions - rotate 180° around any perpendicular axis
    // Find a perpendicular vector
    if (std::abs(fromNorm.x) < 0.707)
    {
      axis = Vector3(1, 0, 0).cross(fromNorm).normalized();
    }
    else
    {
      axis = Vector3(0, 1, 0).cross(fromNorm).normalized();
    }
    angle = M_PI;
    return;
  }

  // Normal case: axis = from × to, angle = acos(dot)
  axis = fromNorm.cross(toNorm);
  angle = std::acos(dot);
}

class ParabolicDish
{
private:
  Vector3 position;
  Vector3 localForward;       // Which local direction points forward (e.g., +Z or +X)
  Vector3 globalAimDirection; // Desired aiming direction in global coordinates

  // Current orientation (as quaternion or rotation matrix)
  double rotationAngle;
  Vector3 rotationAxis;

public:
  // Constructor: dish at position, with local forward direction (e.g., +Z = (0,0,1))
  ParabolicDish(const Vector3 &pos, const Vector3 &localFwd = Vector3(0, 0, 1))
      : position(pos), localForward(localFwd), globalAimDirection(0, 0, 1)
  {

    // Compute required rotation to align local forward with global Z
    getAlignmentRotation(localForward, globalAimDirection, rotationAxis, rotationAngle);
  }

  // Set new dish position (dish moves, but aiming direction remains global Z)
  void setPosition(const Vector3 &newPos)
  {
    position = newPos;
    // Rotation stays the same - we still aim along global Z
  }

  // Get the current aiming direction in global coordinates
  Vector3 getGlobalAimDirection() const
  {
    // Apply the rotation to local forward to get global pointing
    return rotateVector(localForward, rotationAxis, rotationAngle);
  }

  // Get a point along the aim direction (for visualization)
  Vector3 getAimPoint(double distance = 100.0) const
  {
    Vector3 aimDir = getGlobalAimDirection();
    return Vector3(
        position.x + aimDir.x * distance,
        position.y + aimDir.y * distance,
        position.z + aimDir.z * distance);
  }

  // // Get the rotation matrix that transforms local to global coordinates
  // void getRotationMatrix(double R[3][3]) const
  // {
  //   double c = std::cos(rotationAngle);
  //   double s = std::sin(rotationAngle);
  //   double ux = rotationAxis.x;
  //   double uy = rotationAxis.y;
  //   double uz = rotationAxis.z;

  //   // Rodrigues rotation matrix formula
  //   R[0][0] = c + ux * ux * (1 - c);
  //   R[0][1] = ux * uy * (1 - c) - uz * s;
  //   R[0][2] = ux * uz * (1 - c) + uy * s;
  //   R[1][0] = uy * ux * (1 - c) + uz * s;
  //   R[1][1] = c + uy * uy * (1 - c);
  //   R[1][2] = uy * uz * (1 - c) - ux * s;
  //   R[2][0] = uz * ux * (1 - c) - uy * s;
  //   R[2][1] = uz * uy * (1 - c) + ux * s;
  //   R[2][2] = c + uz * uz * (1 - c);
  // }

  void print() const
  {
    Vector3 aimDir = getGlobalAimDirection();
    Vector3 aimPoint = getAimPoint(1.0);

    std::cout << "Dish position: (" << position.x << ", "
              << position.y << ", " << position.z << ")\n";
    std::cout << "Aim direction: (" << aimDir.x << ", "
              << aimDir.y << ", " << aimDir.z << ")\n";
    std::cout << "Aims toward point: (" << aimPoint.x << ", "
              << aimPoint.y << ", " << aimPoint.z << ")\n";
    std::cout << "Rotation angle: " << rotationAngle * 180.0 / M_PI << "°\n\n";
  }
};

int main()
{
  // Dish initially at origin, local forward = +Z
  ParabolicDish dish1(Vector3(0, 0, 0));
  std::cout << "=== Dish at origin ===\n";
  dish1.print();

  // Move dish to new position, but maintain global Z aiming
  ParabolicDish dish2(Vector3(0.979186, 0.128738, 0.15691));
  std::cout << "=== Dish moved to (0.979186,0.128738,0.15691) ===\n";
  dish2.print();

  // Create dish with different local forward direction (e.g., local +X is forward)
  ParabolicDish dish3(Vector3(5, 2, 1), Vector3(1, 0, 0));
  std::cout << "=== Dish with local +X forward ===\n";
  dish3.print();

  return 0;
}

/*

ray num: 233670
Total intersections: 514571
Number of stages in context: 2
=== STAGE MAP DISTRIBUTION ===
stage 1: Hits: 280901
stage 2: Hits: 166300
==============================

ray num: 233670
Total intersections: 458924
Number of stages in context: 2
=== STAGE MAP DISTRIBUTION ===
stage 1: Hits: 225254
stage 2: Hits: 209877
==============================
*/