#pragma once
// Shared by terrain_material.cpp and terrain_material_drawings.cpp. Each
// includes it after its using-directives (it has no includes of its own).
// Both must keep calling intersection: an unreferenced internal function
// is C4505, fatal under /WX. Keep the unnamed namespace (one internal copy
// per TU, as before); do not make it inline or extern.
namespace
{
	RectangleF intersection(const RectangleF& a, const RectangleF& b)
	{
		const float left = std::max(a.left(), b.left());
		const float top = std::max(a.top(), b.top());
		const float right = std::min(a.right(), b.right());
		const float bottom = std::min(a.bottom(), b.bottom());
		if (!(right > left && bottom > top)) { return RectangleF::ZERO; }
		return RectangleF(left, top, right - left, bottom - top);
	}
}
