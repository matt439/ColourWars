// Debug playground: material_value(0) = mode, strength, detail, unused.
// Modes: 0 identity, 1 posterise, 2 ink edges, 3 halftone dots,
// 4 CRT, 5 RGB565 high colour, 6 ordered dither, 7 Paint.exe desktop.
// The engine supplies the frame contract and sampling helpers.

#include "desktop_90s.hlsli"

float largest_channel(float3 colour)
{
	return max(colour.r, max(colour.g, colour.b));
}

float3 posterised(float3 colour, float detail)
{
	const float steps = max(2.0, floor(detail + 0.5)) - 1.0;
	return floor(saturate(colour) * steps + 0.5) / steps;
}

float3 ink_edges(MaterialInput input, float3 colour, float detail)
{
	// Detail is an output-pixel radius. The helper clamps samples at the
	// picture boundary; the effect also sees text and split-pane boundaries.
	const float2 radius = max(1.0, detail) * output_size.zw;
	float3 difference = abs(scene_point(input.uv + float2(radius.x, 0.0)).rgb - colour);
	difference = max(difference, abs(scene_point(input.uv - float2(radius.x, 0.0)).rgb - colour));
	difference = max(difference, abs(scene_point(input.uv + float2(0.0, radius.y)).rgb - colour));
	difference = max(difference, abs(scene_point(input.uv - float2(0.0, radius.y)).rgb - colour));
	// Channel differences retain boundaries between colours of equal luma.
	const float edge = smoothstep(0.06, 0.3, largest_channel(difference));
	return colour * (1.0 - edge);
}

float3 halftone(MaterialInput input, float3 colour, float detail)
{
	const float cell_size = max(3.0, detail);
	const float2 cell = floor(input.output_pixel / cell_size);
	const float2 centre = (cell + 0.5) * cell_size;
	const float4 representative = scene_point(centre * output_size.zw);
	const float3 representative_colour = representative.rgb / max(representative.a, 0.00001);
	const float luminance = saturate(dot(representative_colour, float3(0.299, 0.587, 0.114)));
	// Dark tones use larger dots. A one-pixel transition softens each circle
	// without relying on derivatives or a time-dependent noise pattern.
	const float radius = sqrt(1.0 - luminance) * 0.65;
	const float distance_from_centre = length((input.output_pixel - centre) / cell_size);
	const float softness = 0.5 / cell_size;
	const float dot_ink = (1.0 - smoothstep(radius - softness, radius + softness, distance_from_centre))
		* saturate(radius * cell_size);
	// Multiply ink into the existing colour, keeping silhouettes and hue.
	return colour * (1.0 - 0.85 * dot_ink);
}

float4 material(MaterialInput input)
{
	const float4 picture = scene_point(input.uv);
	const float4 settings = material_value(0);
	float4 result = picture;
	// Neutral settings return the original sample without colour arithmetic.
	if (settings.x >= 0.5 && settings.y > 0.0)
	{
		float3 processed = picture.rgb;
		if (settings.x < 1.5)
		{
			const float3 colour = picture.rgb / max(picture.a, 0.00001);
			processed = posterised(colour, settings.z) * picture.a;
		}
		else if (settings.x < 2.5)
		{
			processed = ink_edges(input, picture.rgb, settings.z);
		}
		else if (settings.x < 3.5)
		{
			processed = halftone(input, picture.rgb, settings.z);
		}
		else if (settings.x < 4.5)
		{
			processed = desktop_crt(input, picture.rgb, settings.z);
		}
		else if (settings.x < 5.5)
		{
			processed = desktop_highcolour(picture);
		}
		else if (settings.x < 6.5)
		{
			processed = desktop_dither(input, picture, settings.z);
		}
		else
		{
			processed = desktop_paint90(input, picture, settings.z);
		}
		result = float4(lerp(picture.rgb, processed, saturate(settings.y)), picture.a);
	}
	return result;
}
