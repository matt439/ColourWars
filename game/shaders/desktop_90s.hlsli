// Optional desktop-display treatments for ColourWars' Paint.exe setting.
// All patterns sit on the output pixels. They never move a sampled feature,
// so the game's mouse aim and split-pane geometry keep their original places.

float3 desktop_crt(MaterialInput input, float3 colour, float detail)
{
	const float pitch = max(4.0, detail);
	const float row = frac(input.output_pixel.y / pitch);
	// A broad stationary beam, with no rolling bar or alternating-frame term.
	const float scanline = 0.9 + 0.1 * cos(row * 6.28318530718);
	const float column = frac(input.output_pixel.x / 3.0) * 3.0;
	const float3 phosphor = float3(column < 1.0, column >= 1.0 && column < 2.0,
		column >= 2.0);
	const float3 grille = 0.96 + 0.04 * phosphor;
	const float2 centred = input.uv * 2.0 - 1.0;
	const float vignette = saturate(1.0 - 0.045 * dot(centred, centred));
	// Every multiplier is at most one: retain premultiplication without
	// clipping bright team paint or adding a white glow over dark keylines.
	return colour * scanline * grille * vignette;
}

float3 desktop_highcolour(float4 picture)
{
	// The display's RGB565 precision, not a replacement set of team swatches.
	const float3 colour = saturate(picture.rgb / max(picture.a, 0.00001));
	const float3 steps = float3(31.0, 63.0, 31.0);
	return (floor(colour * steps + 0.5) / steps) * picture.a;
}

float desktop_bayer_threshold(float2 output_pixel)
{
	const uint2 pixel = (uint2)floor(output_pixel);
	const uint2 low = pixel & 1;
	const uint2 high = (pixel >> 1) & 1;
	// The 2x2 Bayer order is [0, 2; 3, 1]. Expand it to the familiar 4x4
	// ordered pattern without an indexed table or a time-dependent noise tap.
	const uint low_order = 2 * low.x + 3 * low.y - 4 * low.x * low.y;
	const uint high_order = 2 * high.x + 3 * high.y - 4 * high.x * high.y;
	return (float(4 * low_order + high_order) + 0.5) / 16.0;
}

float3 desktop_dither(MaterialInput input, float4 picture, float detail)
{
	const float3 colour = saturate(picture.rgb / max(picture.a, 0.00001));
	const float steps = max(2.0, floor(detail + 0.5)) - 1.0;
	const float threshold = desktop_bayer_threshold(input.output_pixel);
	// Thresholds lie strictly between zero and one, preserving fully black
	// and white endpoints while distributing the intermediate rounding error.
	return (floor(colour * steps + threshold) / steps) * picture.a;
}

float3 desktop_paint90(MaterialInput input, float4 picture, float detail)
{
	return desktop_crt(input, desktop_highcolour(picture), detail);
}
