#pragma once
#include "cloud_visibility_msl.hpp"
#include <string>
namespace storm_metal {
inline const std::string volumetricSkyShaderStorage=std::string(cloudVisibilityMSL)+R"MSL(
/*
// Cloud shader and Godot code

Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.
Copyright (c) 2014-2021 Godot Engine contributors.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Atmosphere shader (https://www.shadertoy.com/view/msXXDS)

Copyright (c) 2023 Fernando García Liñán

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


Source: clayjohn/godot-volumetric-cloud-demo-v2 @ 6b5772821dbb42ee1f278722810e5dee936e9309
Metal port: Corsairs. Cloud lighting moments and tiled-cache integration adapted here.
*/
#include <metal_stdlib>
using namespace metal;

// Matches the backend's canonical converted fixed-function vertex. Keeping the
// sky shader on that representation preserves indexed-span validation and the
// frame upload cache instead of decoding raw D3D9 memory a second time.
struct SkyVertex { packed_float4 p; packed_float4 tint; packed_float4 uv01; packed_float4 uv23;
                   packed_float4 specular; packed_float4 cameraNormal; packed_float4 cameraPosition; };
struct SkyDraw { float4x4 mvp; uint hasCurrentTexture; uint hasNextTexture; float textureBlend; uint padding; };
struct SkyWeather { float4 sunDirection_hour; float4 moonDirection_time; float4 wind_coverage_density; float4 options; float4 horizonFog; };
struct SkyOut { float4 position [[position]]; float3 direction; float2 uv; float4 tint; };

vertex SkyOut dynamic_sky_vs(uint id [[vertex_id]], const device SkyVertex *vertices [[buffer(0)]],
                             constant SkyDraw &draw [[buffer(1)]]) {
    SkyVertex v = vertices[id];
    SkyOut o;
    float3 p = v.p.xyz;
    o.uv = v.uv01.xy;
    o.tint = v.tint;
    o.direction = normalize(p);
    o.position = draw.mvp * float4(p, 1.f);
    return o;
}

constexpr sampler atmoSampler(coord::normalized,address::clamp_to_edge,filter::linear);
constant float PI = 3.14159265358979323846;
constant float INV_PI = 0.31830988618379067154;
constant float INV_4PI = 0.25 * INV_PI;
constant float PHASE_ISOTROPIC = INV_4PI;
constant float RAYLEIGH_PHASE_SCALE = (3.0 / 16.0) * INV_PI;
constant float g = 0.8;
constant float gg = g*g;

// Ray marching steps. More steps mean better accuracy but worse performance
constant int IN_SCATTERING_STEPS = 30;

// All parameters that depend on wavelength (float4) are sampled at
// 630, 560, 490, 430 nanometers

constant float EARTH_RADIUS = 6371.0; // km
constant float ATMOSPHERE_THICKNESS = 100.0; // km
constant float ATMOSPHERE_RADIUS = EARTH_RADIUS + ATMOSPHERE_THICKNESS;
constant float EYE_ALTITUDE          = 0.5;
constant float EYE_DISTANCE_TO_EARTH_CENTER = EARTH_RADIUS + EYE_ALTITUDE;
constant float4  GROUND_ALBEDO = float4(0.3);

// Extraterrestial Solar Irradiance Spectra, units W * m^-2 * nm^-1
// https://www.nrel.gov/grid/solar-resource/spectra.html
constant float4 sun_spectral_irradiance = float4(1.679, 1.828, 1.986, 1.307);
// Rayleigh scattering coefficient at sea level, units km^-1
// "Rayleigh-scattering calculations for the terrestrial atmosphere"
// by Anthony Bucholtz (1995).
constant float4 molecular_scattering_coefficient_base = float4(6.605e-3, 1.067e-2, 1.842e-2, 3.156e-2);
// Ozone absorption cross section, units m^2 / molecules
// "High spectral resolution ozone absorption cross-sections"
// by V. Gorshelev et al. (2014).
constant float4 ozone_absorption_cross_section = float4(3.472e-21, 3.914e-21, 1.349e-21, 11.03e-23) * 1e-4f;

// Mean ozone concentration in Dobson for each month of the year.
constant float ozone_mean_monthly_dobson = 350.0;

/*
 * This model for aerosols and their corresponding parameters come from
 * "A Physically-Based Spatio-Temporal Sky Model"
 * by Guimera et al. (2018).
 */
constant float4 aerosol_absorption_cross_section = float4(2.8722e-24, 4.6168e-24, 7.9706e-24, 1.3578e-23);
constant float4 aerosol_scattering_cross_section = float4(1.5908e-22, 1.7711e-22, 2.0942e-22, 2.4033e-22);
constant float aerosol_base_density = 1.3681e20;
constant float aerosol_background_density = 2e6;
constant float aerosol_height_scale = 0.73;

constant float aerosol_background_divided_by_base_density = aerosol_background_density / aerosol_base_density;

//-----------------------------------------------------------------------------

/*
 * Returns the distance between ro and the first intersection with the sphere
 * or -1.0 if there is no intersection. The sphere's origin is (0,0,0).
 * -1.0 is also returned if the ray is pointing away from the sphere.
 */
float ray_sphere_intersection(float3 ro, float3 rd, float radius)
{
    float b = dot(ro, rd);
    float c = dot(ro, ro) - radius*radius;
    if (c > 0.0 && b > 0.0) return -1.0;
    float d = b*b - c;
    if (d < 0.0) return -1.0;
    if (d > b*b) return (-b+sqrt(d));
    return (-b-sqrt(d));
}

/*
 * Rayleigh phase function.
 */
float molecular_phase_function(float cos_theta)
{
    return RAYLEIGH_PHASE_SCALE * (1.0 + cos_theta*cos_theta);
}

/*
 * Henyey-Greenstrein phase function.
 */
float aerosol_phase_function(float cos_theta)
{
    float den = 1.0 + gg + 2.0 * g * cos_theta;
    return INV_4PI * (1.0 - gg) / (den * sqrt(den));
}

/*
 * Return the molecular volume scattering coefficient (km^-1) for a given altitude
 * in kilometers.
 */
float4 get_molecular_scattering_coefficient(float h)
{
    return molecular_scattering_coefficient_base * exp(-0.07771971 * pow(h, 1.16364243));
}

float4 transmittance_from_lut(texture2d<float> lut, float cos_theta, float normalized_altitude)
{
    float u = clamp(cos_theta * 0.5 + 0.5, 0.0, 1.0);
    float v = clamp(normalized_altitude, 0.0, 1.0);
    return lut.sample(atmoSampler, float2(u,v), level(0));
}

float4 get_multiple_scattering(texture2d<float> transmittance_lut, float cos_theta, float normalized_height, float d)
{
    // Solid angle subtended by the planet from a point at d distance
    // from the planet center.
    float omega = 2.0 * PI * (1.0 - sqrt(d*d - EARTH_RADIUS*EARTH_RADIUS) / d);

    float4 T_to_ground = transmittance_from_lut(transmittance_lut, cos_theta, 0.0);

    float4 T_ground_to_sample =
        transmittance_from_lut(transmittance_lut, 1.0, 0.0) /
        transmittance_from_lut(transmittance_lut, 1.0, normalized_height);

    // 2nd order scattering from the ground
    float4 L_ground = PHASE_ISOTROPIC * omega * (GROUND_ALBEDO / PI) * T_to_ground * T_ground_to_sample * cos_theta;

    // Fit of Earth's multiple scattering coming from other points in the atmosphere
    float4 L_ms = 0.02 * float4(0.217, 0.347, 0.594, 1.0) * (1.0 / (1.0 + 5.0 * exp(-17.92 * cos_theta)));

    return L_ms + L_ground;

}

/*
 * Return the molecular volume absorption coefficient (km^-1) for a given altitude
 * in kilometers.
 */
float4 get_molecular_absorption_coefficient(float h)
{
    h += 1e-4; // Avoid division by 0
    float t = log(h) - 3.22261;
    float density = 3.78547397e20 * (1.0 / h) * exp(-t * t * 5.55555555);
    return ozone_absorption_cross_section * ozone_mean_monthly_dobson * density;
}

float get_aerosol_density(float h)
{
    return aerosol_base_density * (exp(-h / aerosol_height_scale)
        + aerosol_background_divided_by_base_density);
}

/*
 * Get the collision coefficients (scattering and absorption) of the
 * atmospheric medium for a given point at an altitude h.
 */
void get_atmosphere_collision_coefficients(float h,
                                           thread float4 & aerosol_absorption,
                                           thread float4 & aerosol_scattering,
                                           thread float4 & molecular_absorption,
                                           thread float4 & molecular_scattering,
                                           thread float4 & extinction)
{
    h = max(h, 0.0); // In case height is negative
    float aerosol_density = get_aerosol_density(h);
    aerosol_absorption = aerosol_absorption_cross_section * aerosol_density;
    aerosol_scattering = aerosol_scattering_cross_section * aerosol_density;
    molecular_absorption = get_molecular_absorption_coefficient(h);
    molecular_scattering = get_molecular_scattering_coefficient(h);
    extinction = aerosol_absorption + aerosol_scattering + molecular_absorption + molecular_scattering;
}

//-----------------------------------------------------------------------------
// Spectral rendering stuff

constant float4x3 M = float4x3(float3(137.672389239975f,-8.632904716299537f,-1.7181567391931372f),float3(32.549094028629234f,91.29801417199785f,-12.005406444382531f),float3(-38.91428392614275f,34.31665471469816f,29.89044807197628f),float3(8.572844237945445f,-11.103384660054624f,117.47585277566478f));

float3 linear_srgb_from_spectral_samples(float4 L)
{
    return M * L;
}

float4 compute_inscattering(float3 ray_origin, float3 ray_dir, float t_d, thread float4 & transmittance, texture2d<float> transmittance_lut, constant SkyWeather &weather)
{
    float3 sun_dir = weather.sunDirection_hour.xyz.xzy;
    float cos_theta = dot(-ray_dir, sun_dir);

    float molecular_phase = molecular_phase_function(cos_theta);
    float aerosol_phase = aerosol_phase_function(cos_theta);

    float dt = t_d / float(IN_SCATTERING_STEPS);

    float4 L_inscattering = float4(0.0);
    transmittance = float4(1.0);

    for (int i = 0; i < IN_SCATTERING_STEPS; ++i) {
        float t = (float(i) + 0.5) * dt;
        float3 x_t = ray_origin + ray_dir * t;

        float distance_to_earth_center = length(x_t);
        float3 zenith_dir = x_t / distance_to_earth_center;
        float altitude = distance_to_earth_center - EARTH_RADIUS;
        float normalized_altitude = altitude / ATMOSPHERE_THICKNESS;

        float sample_cos_theta = dot(zenith_dir, sun_dir);

        float4 aerosol_absorption, aerosol_scattering;
        float4 molecular_absorption, molecular_scattering;
        float4 extinction;
        get_atmosphere_collision_coefficients(
            altitude,
            aerosol_absorption, aerosol_scattering,
            molecular_absorption, molecular_scattering,
            extinction);

        float4 transmittance_to_sun = transmittance_from_lut(
            transmittance_lut, sample_cos_theta, normalized_altitude);

        float4 ms = get_multiple_scattering(
            transmittance_lut, sample_cos_theta, normalized_altitude,
            distance_to_earth_center);

        float4 S = sun_spectral_irradiance *
            (molecular_scattering * (molecular_phase * transmittance_to_sun + ms) +
             aerosol_scattering   * (aerosol_phase   * transmittance_to_sun + ms));

        float4 step_transmittance = exp(-dt * extinction);

        // Energy-conserving analytical integration
        // "Physically Based Sky, Atmosphere and Cloud Rendering in Frostbite"
        // by Sébastien Hillaire
        float4 S_int = (S - S * step_transmittance) / max(extinction, 1e-7);
        L_inscattering += transmittance * S_int;
        transmittance *= step_transmittance;
    }

    return L_inscattering;
}


kernel void sky_transmittance(texture2d<float,access::write> output [[texture(0)]],uint2 pos [[thread_position_in_grid]]) {
    if(any(pos>=uint2(output.get_width(),output.get_height())))return;
    float2 uv=(float2(pos)+.5f)/float2(output.get_width(),output.get_height());
    float sun_cos_theta = uv.x * 2.0 - 1.0;
    float3 sun_dir = float3(-sqrt(1.0 - sun_cos_theta*sun_cos_theta), 0.0, sun_cos_theta);

    float distance_to_earth_center = mix(EARTH_RADIUS, ATMOSPHERE_RADIUS, uv.y);
    float3 ray_origin = float3(0.0, 0.0, distance_to_earth_center);

    float t_d = ray_sphere_intersection(ray_origin, sun_dir, ATMOSPHERE_RADIUS);
    float dt = t_d / 40.f;

    float4 result = float4(0.0);

    for (int i = 0; i < 40; ++i) {
        float t = (float(i) + 0.5) * dt;
        float3 x_t = ray_origin + sun_dir * t;

        float altitude = length(x_t) - EARTH_RADIUS;

        float4 aerosol_absorption, aerosol_scattering;
        float4 molecular_absorption, molecular_scattering;
        float4 extinction;
        get_atmosphere_collision_coefficients(
            altitude,
            aerosol_absorption, aerosol_scattering,
            molecular_absorption, molecular_scattering,
            extinction);

        result += extinction * dt;
    }

    float4 transmittance = exp(-result);

	    output.write(transmittance,pos);
}
kernel void sky_atmosphere(texture2d<float,access::write> output [[texture(0)]], texture2d<float> transmittance_lut [[texture(1)]], constant SkyWeather &weather [[buffer(0)]], uint2 pos [[thread_position_in_grid]]) {
    if(any(pos>=uint2(output.get_width(),output.get_height())))return;
    float2 uv=(float2(pos)+.5f)/float2(output.get_width(),output.get_height());
    float azimuth=2.f*PI*(uv.x-.5f),l=uv.y*2.f-1.f,elev=l*l*sign(l)*PI*.5f;
    float3 ray_dir=float3(cos(elev)*cos(azimuth),cos(elev)*sin(azimuth),sin(elev));
    float3 ray_origin=float3(0,0,EYE_DISTANCE_TO_EARTH_CENTER);
    float atmos_dist=ray_sphere_intersection(ray_origin,ray_dir,ATMOSPHERE_RADIUS),ground_dist=ray_sphere_intersection(ray_origin,ray_dir,EARTH_RADIUS);
    float4 transmittance;
    float4 L=compute_inscattering(ray_origin,ray_dir,ground_dist<0.f?atmos_dist:ground_dist,transmittance,transmittance_lut,weather);
    output.write(float4(max(linear_srgb_from_spectral_samples(L),0.f)/50.f,1.f),pos);
}
float3 sampleAtmosphere(float3 ray,texture2d<float> atmosphere) {
    float theta=asin(clamp(ray.y,-1.f,1.f));
    float2 uv=float2(atan2(ray.z,ray.x)/(2.f*PI)+.5f,sqrt(abs(theta)/(PI*.5f))*sign(theta)*.5f+.5f);
    constexpr sampler s(coord::normalized,s_address::repeat,t_address::clamp_to_edge,filter::linear);
    return atmosphere.sample(s,uv,level(0)).rgb;
}
constant float g_radius=6000000.f, sky_b_radius=6001500.f, sky_t_radius=6004000.f;
constexpr sampler ns(coord::normalized,address::repeat,filter::linear,mip_filter::linear);
// From: https://www.shadertoy.com/view/4sfGzS credit to iq
float hash(float3 p) {
	p  = fract( p * 0.3183099 + 0.1 );
	p *= 17.0;
	return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Utility function that maps a value from one range to another.
float remap(float originalValue,  float originalMin,  float originalMax,  float newMin,  float newMax) {
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

// Phase function
float henyey_greenstein(float cos_theta, float g) {
	const float k = 0.0795774715459;
	return k * (1.0 - g * g) / (pow(1.0 + g * g - 2.0 * g * cos_theta, 1.5));
}

float GetHeightFractionForPoint(float inPosition) {
	float height_fraction = (inPosition -  sky_b_radius) / (sky_t_radius - sky_b_radius);
	return clamp(height_fraction, 0.0, 1.0);
}

float4 mixGradients(float cloudType){
	const float4 STRATUS_GRADIENT = float4(0.02f, 0.05f, 0.09f, 0.11f);
	const float4 STRATOCUMULUS_GRADIENT = float4(0.02f, 0.2f, 0.48f, 0.625f);
	const float4 CUMULUS_GRADIENT = float4(0.01f, 0.0625f, 0.78f, 1.0f);
	float stratus = 1.0f - clamp(cloudType * 2.0f, 0.0, 1.0);
	float stratocumulus = 1.0f - abs(cloudType - 0.5f) * 2.0f;
	float cumulus = clamp(cloudType - 0.5f, 0.0, 1.0) * 2.0f;
	return STRATUS_GRADIENT * stratus + STRATOCUMULUS_GRADIENT * stratocumulus + CUMULUS_GRADIENT * cumulus;
}

float densityHeightGradient(float heightFrac, float cloudType) {
	float4 cloudGradient = mixGradients(cloudType);
	return smoothstep(cloudGradient.x, cloudGradient.y, heightFrac) - smoothstep(cloudGradient.z, cloudGradient.w, heightFrac);
}

float intersectSphere(float3 pos, float3 dir,float r) {
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, pos);
    float c = dot(pos, pos) - (r * r);
	float d = sqrt((b*b) - 4.0*a*c);
	float p = -b - d;
	float p2 = -b + d;
    return max(p, p2) / (2.0 * a);
}

// Returns density at a given point
// Heavily based on method from Schneider
float density(float3 pip, float3 weather, float mip, texture3d<float> large_scale_noise, texture3d<float> small_scale_noise, texture2d<float> weather_noise, constant SkyWeather &w) {
	float3 p = pip;
	float height_fraction = GetHeightFractionForPoint(length(p));
	// A much larger, independently scaled weather domain bends the repeating
	// source volume and varies cloud towers/coverage without swapping textures.
	float3 macro=weather_noise.sample(ns,p.xz*.0000123f+float2(.371f,.619f)+w.options.yz*.003f,level(0)).rgb;
	p.xz+=(macro.rb-float2(.748f,.577f))*8500.f;

	// Base wind.
	p.xz += 20.0 * (w.options.yz*2400.f) * 0.6;

	// Define the base of the cloud.
	float4 n = large_scale_noise.sample(ns, p.xyz * 0.00008f, level(max(0.f,mip-2.f)));
	float fbm = n.g * 0.625 + n.b * 0.25 + n.a * 0.125;

	// Remap based on weather, coverage, and cloud shape gradient.
	float type=mix(.62f+.38f*smoothstep(.68f,.83f,weather.r),.18f+.40f*macro.r,smoothstep(.5f,.9f,w.wind_coverage_density.z));
	float g = densityHeightGradient(height_fraction, type);
	float base_cloud = remap(n.r, -(1.0 - fbm), 1.0, 0.0, 1.0);
	float weather_coverage = clamp(w.wind_coverage_density.z * weather.b * (.65f+.60f*macro.b),.001f,1.f);
	base_cloud = remap(base_cloud * g, 1.0 - (weather_coverage), 1.0, 0.0, 1.0);
	base_cloud *= weather_coverage;

	// Detailed wind.
	p.xz -= (w.options.yz*4800.f) * 40.;
	p.y -= (w.moonDirection_time.w*.025f) * 40.;

	// Detailed texture.
	float3 hn = small_scale_noise.sample(ns, p * 0.001f, level(max(0.f,mip))).rgb;
	float hfbm = hn.r * 0.625 + hn.g * 0.25 + hn.b * 0.125;
	hfbm = mix(hfbm, 1.0 - hfbm, clamp(height_fraction * 4.0, 0.0, 1.0));
	base_cloud = remap(base_cloud, hfbm * 0.4 * height_fraction, 1.0, 0.0, 1.0);
	return pow(clamp(base_cloud, 0.0, 1.0), (1.0 - height_fraction) * 0.8 + 0.5);
}

float4 march(float3 pos, float3 end, float3 dir, int depth, texture3d<float> large_scale_noise, texture3d<float> small_scale_noise, texture2d<float> weather_noise, constant SkyWeather &w) {
	const float3 RANDOM_VECTORS[6] = {float3( 0.38051305f,  0.92453449f, -0.02111345f),float3(-0.50625799f, -0.03590792f, -0.86163418f),float3(-0.32509218f, -0.94557439f,  0.01428793f),float3( 0.09026238f, -0.27376545f,  0.95755165f),float3( 0.28128598f,  0.42443639f, -0.86065785f),float3(-0.16852403f,  0.14748697f,  0.97460106f)};

	// Initialize ray length, direction, and position.
	float ss = length(dir);
	dir = normalize(dir);
	const float jitter = hash(pos * 10.0);
	float3 p = pos;

	// Initialize light ray.
	const float t_dist = sky_t_radius - sky_b_radius;
	float lss = (t_dist / 64.0);
	float3 ldir = normalize(w.sunDirection_hour.xyz);

	float t = 1.0;
	float T = 1.0;
	float alpha = 0.0;
	float3 L = float3(0.0);


	float costheta = dot(ldir, dir);
	// Stack multiple phase functions to emulate some backscattering
	float phase = max(max(henyey_greenstein(costheta, 0.6), henyey_greenstein(costheta, (0.4 - 1.4 * ldir.y))), henyey_greenstein(costheta, -0.2));

	const float weather_scale = 0.00006;
	float2 weather_pos = (w.options.yz*.06f);

	for (int i = 0; i < depth; i++) {
		if(T<.005f)break;
		// Planet-space Y is about six million metres: repeatedly adding small
		// steps rounds the same error into every sample and bands the whole ray.
		p = pos + dir * (ss * (float(i) + 1.f + jitter));
		float3 weather_sample = weather_noise.sample(ns, p.xz * weather_scale + .5f + weather_pos).xyz;
		float height_fraction = GetHeightFractionForPoint(length(p));

		t = density(p, weather_sample, max(0.f,log2(ss*.001f)), large_scale_noise, small_scale_noise, weather_noise, w);
		float dt = exp(-(.025f+.055f*w.wind_coverage_density.w) * t * ss);

		float3 lp = p;
		float lt = 1.0;
		float cd = 0.0;

		if (t > 0.0) { //calculate lighting, but only when we are in the cloud
			float lheight_fraction = 0.0;
			for (int j = 0; j < 6; j++) {
				lp +=  (ldir + RANDOM_VECTORS[j] * float(j)) * lss;
				lheight_fraction = GetHeightFractionForPoint(length(lp));
				float3 lweather = weather_noise.sample(ns, lp.xz * weather_scale + .5f + weather_pos).xyz;
				lt = density(lp, lweather, float(j), large_scale_noise, small_scale_noise, weather_noise, w);
				cd += lt;
			}

			// Take a single distant sample
			lp = p + ldir * 18.0 * lss;
			lheight_fraction = GetHeightFractionForPoint(length(lp));
			float3 lweather = weather_noise.sample(ns, lp.xz * weather_scale + .5f + weather_pos).xyz;
			lt = pow(density(lp, lweather, 5.0, large_scale_noise, small_scale_noise, weather_noise, w), (1.0 - lheight_fraction) * 0.8 + 0.5);
			cd += lt;

			// captures the direct lighting from the sun
			float beers = exp(-(.025f+.055f*w.wind_coverage_density.w) * cd * lss * 3.0);
			float powder_sugar_effect = 1.0 - exp(-(.025f+.055f*w.wind_coverage_density.w) * cd * lss * 3.0 * 2.0);
			float beers_total = 2 * beers * powder_sugar_effect;

            float upper=smoothstep(0.f,1.f,height_fraction);
            alpha += (1.f-dt)*(1.f-alpha);
            // Alpha already supplies the lower ambient weight (alpha-upper).
            // Use the spare moment for optical distance, so aerial perspective
            // reduces distant contrast without filtering cloud edges.
            float distance=length(p-float3(0,g_radius,0))*.001f;
            L += T*(1.f-dt)*float3(upper,distance,beers_total);
			T *= dt;
		}
	}
	alpha = clamp(alpha, 0.0, 1.0);
	return float4(L, alpha);
}

// Take a direction as input and draw the sky.
float4 cloudSky(float3 dir, texture3d<float> large_scale_noise, texture3d<float> small_scale_noise, texture2d<float> weather_noise, constant SkyWeather &w) {
	float4 col = float4(0.0);

	if (dir.y > 0.0) {
		// Only draw clouds above the horizon.
		float3 camPos = float3(0.0, g_radius, 0.0);
		float3 start = camPos + dir * intersectSphere(camPos, dir, sky_b_radius);
		float3 end = camPos + dir * intersectSphere(camPos, dir, sky_t_radius);
		float shelldist = (length(end - start));
		// Grazing rays traverse a much longer shell. Increase their bounded
		// sampling budget instead of hiding distant detail in an opacity fade.
		float steps = 128.f + 512.f*(1.f-smoothstep(.03f,.30f,dir.y));

		float3 raystep = dir * shelldist / steps;
		col = march(start, end, raystep, int(steps), large_scale_noise, small_scale_noise, weather_noise, w);
	} else {
		col = float4(0.0);
	}

    return col;
}



float3 skyOctDirection(float2 uv) {
    float x=uv.x-uv.y,y=uv.x+uv.y-1.f;
    return normalize(float3(x,1.f-abs(x)-abs(y),y));
}
kernel void sky_clouds(texture2d<float,access::write> output [[texture(0)]], texture3d<float> shape [[texture(1)]],texture3d<float> detail [[texture(2)]],texture2d<float> coverage [[texture(3)]],constant SkyWeather&w [[buffer(0)]],constant uint4&region [[buffer(1)]],uint2 id [[thread_position_in_grid]]) {
    if(any(id>=uint2(region.w)))return;
    uint2 pixel=id+region.yz;float3 d=skyOctDirection((float2(pixel)+.5f)/float(region.x));
    float4 cloud=d.y>0.f?cloudSky(d,shape,detail,coverage,w):float4(0);
    output.write(cloud,pixel);
}
float3 skyDisplay(float3 color){return pow(1.f-exp(-max(color,0.f)*1.2f),float3(1.f/2.2f));}
struct SkyStar {packed_float3 center;float size,angle;uint color,subtexture;};
struct SkyStarDraw {float4x4 view,projection;float4 camera,scale_blend;};
struct SkyStarOut {float4 position [[position]];float4 color;float2 uv;float3 direction;};
vertex SkyStarOut dynamic_stars_vs(uint id [[vertex_id]],const device SkyStar*stars [[buffer(0)]],constant SkyStarDraw&draw [[buffer(1)]]) {
    constexpr float2 corners[6]={float2(-1,-1),float2(-1,1),float2(1,-1),float2(1,-1),float2(-1,1),float2(1,1)};
    SkyStar s=stars[id/6];float2 q=corners[id%6];float sn=sin(s.angle),cs=cos(s.angle);
    float4 center=draw.view*float4(s.center,1);center.xy+=float2(q.x*cs+q.y*sn,-q.x*sn+q.y*cs)*s.size*draw.scale_blend.xy;
    SkyStarOut o;o.position=draw.projection*center;o.color=float4(float((s.color>>16)&255),float((s.color>>8)&255),float(s.color&255),float((s.color>>24)&255))/255.f;
    o.uv=float2(q.x*.5f+.5f,.5f-q.y*.5f);o.direction=normalize(float3(s.center)-draw.camera.xyz);return o;
}
fragment float4 dynamic_stars_fs(SkyStarOut in [[stage_in]],constant SkyStarDraw&draw [[buffer(1)]],texture2d<float>sprite [[texture(0)]],texture2d<float>cloudFrom [[texture(2)]],texture2d<float>cloudTo [[texture(3)]]) {
    constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::linear);
    float4 color=sprite.sample(s,in.uv)*in.color;
    float opacity=skyCloudMoments(normalize(in.direction),cloudFrom,cloudTo,draw.scale_blend.z).a;
    color.a*=1.f-opacity;return color;
}
fragment float4 dynamic_sky_fs(SkyOut in [[stage_in]],constant SkyDraw&draw [[buffer(1)]],constant SkyWeather&weather [[buffer(2)]],texture2d<float>current [[texture(0)]],texture2d<float>next [[texture(1)]],texture2d<float>cloudFrom [[texture(2)]],texture2d<float>cloudTo [[texture(3)]],texture2d<float>atmosphere [[texture(4)]],texture2d<float>transmittance [[texture(5)]],sampler skySampler [[sampler(0)]]) {
    float3 d=normalize(in.direction),sun=normalize(weather.sunDirection_hour.xyz);
    float solarElevation=1.3780972f*sin((weather.sunDirection_hour.w-5.5f)*.232710567f)-.2f;
    float daylight=smoothstep(-.13f,.10f,solarElevation),twilight=1.f-smoothstep(.02f,.28f,abs(solarElevation));
    float horizon=1.f-smoothstep(-.02f,.42f,max(d.y,0.f));
    float3 fog=weather.horizonFog.w>.5f?weather.horizonFog.rgb:mix(float3(.035f,.045f,.075f),float3(.64f,.78f,.92f),daylight);
    float3 color=mix(fog*float3(.22f,.42f,.72f),fog,horizon);
    color+=float3(1.f,.25f,.055f)*twilight*horizon*max(0.f,dot(d,float3(sun.x,0,sun.z)))*.55f;
    float3 physical=skyDisplay(sampleAtmosphere(d,atmosphere));
    color=mix(color,physical,twilight*.72f);
    float4 moments=skyCloudMoments(d,cloudFrom,cloudTo,weather.options.w);
    float3 upper=float3(.48f,.62f,.82f);upper=mix(upper,float3(length(upper)),.5f);
    float3 lower=float3(.18f,.22f,.26f);lower=mix(lower,float3(.4f,.55f,.65f)*length(lower),.5f);
    float3 transmission=transmittance_from_lut(transmittance,sun.y,0.f).rgb;
    float3 noonTransmission=transmittance_from_lut(transmittance,1.f,0.f).rgb;
    float3 sunColor=float3(12.f,12.4f,13.2f)*transmission/max(noonTransmission,.001f);
    float cosTheta=dot(d,sun),phase=max(max(henyey_greenstein(cosTheta,.6f),henyey_greenstein(cosTheta,clamp(.4f-1.4f*sun.y,-.7f,.7f))),henyey_greenstein(cosTheta,-.2f));
    float lowerWeight=max(0.f,moments.a-moments.x);
    float3 dayCloud=moments.x*upper+lowerWeight*lower+moments.z*sunColor*phase;
    // The cloud moments contain HDR radiance but Storm's scene target is LDR.
    // Compress only bright highlights before that target loses their detail;
    // unpremultiply first so thin cloud edges keep their proper coverage.
    float3 radiance=dayCloud/max(moments.a,.00001f);
    float peak=max(max(radiance.r,radiance.g),radiance.b);
    float displayPeak=peak<=.75f?peak:.75f+.25f*(1.f-exp(-(peak-.75f)/.25f));
    float3 displayCloud=radiance*(displayPeak/max(peak,.00001f))*moments.a;
    float sunsetExposure=1.f-smoothstep(.12f,.42f,solarElevation);
    dayCloud=mix(displayCloud,1.f-exp(-dayCloud*.85f),sunsetExposure);
    float overcast=smoothstep(.5f,.96f,weather.wind_coverage_density.z);
    dayCloud*=mix(1.f,.64f,overcast);
    float3 nightCloud=moments.x*float3(.055f,.071f,.105f)+lowerWeight*float3(.022f,.027f,.044f)+moments.z*float3(.16f,.20f,.29f)*phase;
    float distance=moments.y/max(moments.a,.001f);
    float3 airExtinction=.5f*(molecular_scattering_coefficient_base.xyz+aerosol_scattering_cross_section.xyz*aerosol_base_density*.25f);
    airExtinction+=overcast*.045f;
    float3 aerial=exp(-max(0.f,distance-1.5f)*airExtinction);
    color=mix(color,color*(1.f-moments.a)+mix(nightCloud,dayCloud,daylight),aerial);
    float seam=(1.f-smoothstep(0.f,.0015f,max(d.y,0.f)))*weather.horizonFog.w;
    color=mix(color,fog,seam);
    return float4(max(color,0.f),1.f);
}
)MSL";
inline const char *volumetricSkyShaderSource=volumetricSkyShaderStorage.c_str();
}
