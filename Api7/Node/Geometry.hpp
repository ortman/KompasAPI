#pragma once

#include "../../Include/Node/Vertex.h"
#include "../ComKompas.h"

#include <algorithm>
#include <optional>

// Общие геометрические помощники для граней и рёбер
namespace Api7Geometry {

inline std::optional<Box3D> CurveBox(K5::ksCurve3DPtr curve) {
	Box3D b;
	if (!curve || !curve->GetGabarit(&b.min.x, &b.min.y, &b.min.z, &b.max.x, &b.max.y, &b.max.z)) return std::nullopt;
	return b;
}

inline void Unite(std::optional<Box3D>& box, const std::optional<Box3D>& other) {
	if (!other) return;
	if (!box) {
		box = other;
		return;
	}
	box->min = {(std::min)(box->min.x, other->min.x), (std::min)(box->min.y, other->min.y), (std::min)(box->min.z, other->min.z)};
	box->max = {(std::max)(box->max.x, other->max.x), (std::max)(box->max.y, other->max.y), (std::max)(box->max.z, other->max.z)};
}

} // namespace Api7Geometry
