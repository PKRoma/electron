// Copyright (c) 2024 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/renderer/electron_smooth_round_rect.h"

#include <numbers>
#include "base/check.h"

namespace electron {

namespace {

constexpr SkPoint QuarterRotate(const SkPoint& p,
                                unsigned int quarter_rotations) {
  switch (quarter_rotations % 4) {
    case 0:
      return p;
    case 1:
      return {-p.y(), p.x()};
    case 2:
      return {-p.x(), -p.y()};
    // case 3:
    default:
      return {p.y(), -p.x()};
  }
}

// Geometric measurements for constructing the curves of smooth round corners on
// a rectangle.
//
// Each measurement's value is relative to the rectangle's natural corner point.
// An "offset" measurement is a one-dimensional length and a "vector"
// measurement is a two-dimensional pair of lengths.
//
// Each measurement's direction is relative to the direction of an edge towards
// the corner. Offsets are in the same direction as the edge toward the corner.
// For vectors, the X direction is parallel and the Y direction is
// perpendicular.
struct CurveGeometry {
  CurveGeometry(float radius, float smoothness);

  inline constexpr SkVector edge_connecting_vector() const {
    return {edge_connecting_offset, 0.0f};
  }
  inline constexpr SkVector edge_curve_vector() const {
    return {edge_curve_offset, 0.0f};
  }
  inline constexpr SkVector arc_curve_vector() const {
    return {arc_curve_offset, 0.0f};
  }
  inline constexpr SkVector arc_connecting_vector_transposed() const {
    return {arc_connecting_vector.y(), arc_connecting_vector.x()};
  }

  // The point where the edge connects to the curve.
  float edge_connecting_offset;

  // The control point for the curvature where the edge connects to the curve.
  float edge_curve_offset;

  // The control point for the curvature where the arc connects to the curve.
  float arc_curve_offset;

  // The point where the arc connects to the curve.
  SkVector arc_connecting_vector;
};

CurveGeometry::CurveGeometry(float radius, float smoothness) {
  edge_connecting_offset = (1.0f + smoothness) * radius;

  float arc_angle = (std::numbers::pi / 4.0f) * smoothness;

  arc_connecting_vector =
      SkVector::Make(1.0f - std::sin(arc_angle), 1.0f - std::cos(arc_angle)) *
      radius;

  arc_curve_offset = (1.0f - std::tan(arc_angle / 2.0f)) * radius;

  constexpr float EDGE_CURVE_POINT_RATIO = 2.0f / 3.0f;
  edge_curve_offset =
      edge_connecting_offset -
      ((edge_connecting_offset - arc_curve_offset) * EDGE_CURVE_POINT_RATIO);
}

void DrawCorner(SkPath& path,
                float radius,
                const CurveGeometry& curve,
                const SkPoint& corner,
                unsigned int quarter_rotations) {
  // Move/Line to the edge connecting point
  {
    SkPoint edge_connecting_point =
        corner +
        QuarterRotate(curve.edge_connecting_vector(), quarter_rotations + 1);

    if (quarter_rotations == 0) {
      path.moveTo(edge_connecting_point);
    } else {
      path.lineTo(edge_connecting_point);
    }
  }

  // Draw the first smoothing curve
  {
    SkPoint edge_curve_point = corner + QuarterRotate(curve.edge_curve_vector(),
                                                      quarter_rotations + 1);
    SkPoint arc_curve_point =
        corner + QuarterRotate(curve.arc_curve_vector(), quarter_rotations + 1);
    SkPoint arc_connecting_point =
        corner +
        QuarterRotate(curve.arc_connecting_vector_transposed(),
                      quarter_rotations);  // This transpose is equivalent to 3
                                           // rotations, and 3 + 1 quarter
                                           // rotations = 0 quarter rotations
    path.cubicTo(edge_curve_point, arc_curve_point, arc_connecting_point);
  }

  // Draw the arc
  {
    SkPoint arc_connecting_point =
        corner + QuarterRotate(curve.arc_connecting_vector, quarter_rotations);
    path.arcTo(SkPoint::Make(radius, radius), 0.0f, SkPath::kSmall_ArcSize,
               SkPathDirection::kCW, arc_connecting_point);
  }

  // Draw the second smoothing curve
  {
    SkPoint arc_curve_point =
        corner + QuarterRotate(curve.arc_curve_vector(), quarter_rotations);
    SkPoint edge_curve_point =
        corner + QuarterRotate(curve.edge_curve_vector(), quarter_rotations);
    SkPoint edge_connecting_point =
        corner +
        QuarterRotate(curve.edge_connecting_vector(), quarter_rotations);
    path.cubicTo(arc_curve_point, edge_curve_point, edge_connecting_point);
  }
}

}  // namespace

SkPath DrawSmoothRoundRect(float x,
                           float y,
                           float width,
                           float height,
                           float smoothness,
                           float top_left_radius,
                           float top_right_radius,
                           float bottom_right_radius,
                           float bottom_left_radius) {
  if (width <= 0.0f || height <= 0.0f) {
    return SkPath();
  }

  // We assume the radii are already constrained within the rectangle size
  DCHECK(top_left_radius + top_right_radius <= width);
  DCHECK(bottom_left_radius + bottom_right_radius <= width);
  DCHECK(top_left_radius + bottom_left_radius <= height);
  DCHECK(top_right_radius + bottom_right_radius <= height);

  // Constrain each corner curve's smoothness
  // TODO

  SkPath path;

  // Top left corner
  DrawCorner(path, top_left_radius, CurveGeometry(top_left_radius, smoothness),
             SkPoint::Make(x, y), 0);

  // Top right corner
  DrawCorner(path, top_right_radius,
             CurveGeometry(top_right_radius, smoothness),
             SkPoint::Make(x + width, y), 1);

  // Bottom right corner
  DrawCorner(path, bottom_right_radius,
             CurveGeometry(bottom_right_radius, smoothness),
             SkPoint::Make(x + width, y + height), 2);

  // Bottom left corner
  DrawCorner(path, bottom_left_radius,
             CurveGeometry(bottom_left_radius, smoothness),
             SkPoint::Make(x, y + height), 3);

  path.close();
  return path;
}

}  // namespace electron
