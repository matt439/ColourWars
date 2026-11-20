// Single-pass RGB monitors. The virtual raster describes the deflection of
// the beam across the full display; the game continues to render at native
// resolution. Everything is stationary on the glass, including the mask.
struct CrtMonitor
{
	float spot_radius;
	float dark_beam_width;
	float mask_contrast;
	float diffusion_radius;
	float diffusion_amount;
};

// Approximate a gamma-2 display. Filter premultiplied *light* and coverage
// together so the colour of a transparent neighbour contributes no energy.
float4 crt_linear_signal(float4 sample_colour)
{
	return float4(sample_colour.rgb * sample_colour.rgb / max(sample_colour.a, 0.00001), sample_colour.a);
}

float3 crt_radiance(float4 signal)
{
	return signal.rgb / max(signal.a, 0.00001);
}

float4 crt_spot(float2 uv, float radius_uv)
{
	const float2 side = float2(radius_uv, 0.0);
	const float4 centre = crt_linear_signal(scene_linear(uv));
	const float4 left = crt_linear_signal(scene_linear(uv - side));
	const float4 right = crt_linear_signal(scene_linear(uv + side));
	return 0.6 * centre + 0.2 * (left + right);
}

// Unit-area Gaussian. A bright beam spreads its energy across more of the
// line interval. Integrating the pixel footprint as variance also softens
// the beam peaks when a scanline spans only a couple of output pixels.
float crt_beam(float4 spot, float distance_in_lines, float dark_width, float pixel_variance)
{
	const float luminance = saturate(dot(crt_radiance(spot), float3(0.2126, 0.7152, 0.0722)));
	const float width = lerp(dark_width, 0.48, sqrt(luminance));
	const float variance = width * width + pixel_variance;
	return 0.398942 * rsqrt(variance) * exp2(-0.721348 * distance_in_lines * distance_in_lines / variance);
}

float3 crt_raster(MaterialInput input, float3 native_light, float lines, CrtMonitor tube, float scale)
{
	const float pitch = output_size.y / lines;
	const float raster_y = input.uv.y * lines;
	const float distance = frac(raster_y) - 0.5;
	const float2 row = float2(input.uv.x, (floor(raster_y) + 0.5) / lines);
	const float2 interval = float2(0.0, 1.0 / lines);
	const float radius = tube.spot_radius * scale * output_size.z;
	const float4 above = crt_spot(row - interval, radius);
	const float4 here = crt_spot(row, radius);
	const float4 below = crt_spot(row + interval, radius);
	const float pixel_variance = 1.0 / (12.0 * pitch * pitch);
	const float3 weights = float3(
		crt_beam(above, distance + 1.0, tube.dark_beam_width, pixel_variance),
		crt_beam(here, distance, tube.dark_beam_width, pixel_variance),
		crt_beam(below, distance - 1.0, tube.dark_beam_width, pixel_variance));
	const float4 signal = above * weights.x + here * weights.y + below * weights.z;
	const float energy = weights.x + weights.y + weights.z;
	// Ease out periodic contrast below the sampling limit. Keep a share of
	// the native signal at all resolutions so a fine HUD stroke between two
	// virtual rows still contributes even on the coarse arcade raster.
	const float resolved = smoothstep(1.65, 3.4, pitch);
	return lerp(native_light, crt_radiance(signal), lerp(0.30, 0.72, resolved))
		* lerp(1.0, energy, resolved);
}

float3 crt_diffusion(MaterialInput input, float radius_pixels)
{
	// Four bilinear footprints on the glass spread a little of the adjacent
	// phosphors' light. Keep each RGB channel, even for saturated team paint.
	const float2 radius = radius_pixels * output_size.zw;
	const float2 diagonal = float2(radius.x, -radius.y);
	const float4 light = crt_linear_signal(scene_linear(input.uv + radius))
		+ crt_linear_signal(scene_linear(input.uv - radius))
		+ crt_linear_signal(scene_linear(input.uv + diagonal))
		+ crt_linear_signal(scene_linear(input.uv - diagonal));
	return crt_radiance(light);
}

float3 crt_phosphors(float2 output_pixel, float mode, float scale)
{
	// Integer pixel groups keep the RGB triads phase-locked to the panel.
	// At 2160p each phosphor is two pixels across; at 720p it is one.
	const float cell = max(1.0, floor(scale + 0.5));
	const float2 p = (floor(output_pixel) + 0.5) / cell;
	const float3 phase = (p.x - 0.5) / 3.0 + float3(0.0, -1.0 / 3.0, 1.0 / 3.0);
	const float3 grille = cos(6.2831853 * phase);
	float3 pattern = grille;
	if (mode < 8.5)
	{
		// Shadow mask: the next row of phosphor dots sits half a triad
		// across. The spot narrows between the centres of adjacent rows.
		pattern *= cos(3.14159265 * (p.y - 0.5));
	}
	else if (mode > 9.5)
	{
		// Slot mask: neighbouring triads stagger their dark horizontal
		// ties. The long vertical slots keep the arcade texture coarser.
		const float triad = floor(p.x / 3.0);
		const float slot = cos(6.2831853 * (p.y / 4.0 + triad * 0.5));
		pattern = 0.76 * grille + 0.24 * slot;
	}
	return pattern;
}

float3 crt_monitor(MaterialInput input, float4 picture, float mode, float lines)
{
	// Radius values are output pixels at 1080p. Beam widths are fractions
	// of a virtual scanline. Each tube has its own focus, mask and glass.
	CrtMonitor tube = { 0.65, 0.30, 0.23, 0.9, 0.025 }; // VGA dot mask
	if (mode >= 9.5)
	{
		const CrtMonitor arcade = { 1.55, 0.23, 0.40, 2.8, 0.12 };
		tube = arcade;
	}
	else if (mode >= 8.5)
	{
		const CrtMonitor trinitron = { 0.95, 0.27, 0.34, 1.5, 0.055 };
		tube = trinitron;
	}
	const float scale = output_size.y / 1080.0;
	const float3 original = crt_radiance(crt_linear_signal(picture));
	float3 light = crt_raster(input, original, max(lines, 1.0), tube, scale);
	const float mask = tube.mask_contrast * lerp(0.45, 1.0, smoothstep(540.0, 1440.0, output_size.y));
	light *= 1.0 + mask * crt_phosphors(input.output_pixel, mode, scale);
	light = lerp(light, crt_diffusion(input, tube.diffusion_radius * scale), tube.diffusion_amount);
	return sqrt(saturate(light)) * picture.a;
}
