﻿/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/
// gl_postprocess.c -- screen polygons and effects
#include <float.h>
#include "gl_local.h"

GLuint gl_compositeprog = 0;
GLuint u_compositeTexScale = 0;
GLuint u_compositeMode = 0;
GLuint u_compositeHDRParam = 0;
GLuint u_compositeBrightParam = 0;

GLuint gl_postprog = 0;
GLuint u_postsurfcolor = 0;
GLuint u_postTexScale = 0;
GLuint u_postwaterwarpparam = 0;
GLuint u_postwaterwarp = 0;

float r_warpmaxs;
float r_warpmaxt;

typedef struct wwvert_s
{
	float position[2];
	float texcoord[2];
} wwvert_t;

GLuint r_postvbo = 0;
GLuint r_postvao = 0;

GLuint r_warpGradientImage;
GLuint r_brightPassRenderImage;
GLuint r_bloomRenderImage[MAX_BLOOM_BUFFERS];
GLuint r_currentRenderHDRImage64;
GLuint r_currentRenderHDRImage;
GLuint r_currentDepthRenderImage;
GLuint r_currentAORenderImage;

cvar_t *r_hdrAutoExposure;
cvar_t *r_hdrKey;
cvar_t *r_hdrMinLuminance;
cvar_t *r_hdrMaxLuminance;
cvar_t *r_hdrClampLuminance;
cvar_t *r_exposure;
cvar_t *r_hdrContrastThreshold;
cvar_t *r_hdrContrastOffset;
cvar_t *r_debugHdrAdaptation;
cvar_t *r_debugHdrHistogram;

void RPostProcess_CreatePrograms(void)
{
	wwvert_t wwverts[4];
	byte *data = NULL;
	int width, height;

	// create shaders
	gl_compositeprog = GL_CreateShaderFromName("glsl/composite.glsl", "CompositeVS", "CompositeFS");
	gl_postprog = GL_CreateShaderFromName("glsl/post.glsl", "PostVS", "PostFS");

	// create texture for underwater warp gradient
	LoadTGAFile("env/warpgradient.tga", &data, &width, &height);
	if (data)
		r_warpGradientImage = GL_UploadTexture (data, width, height, false, 32);

	// create textures for use by the HDR framebuffer objects
	glDeleteTextures(1, &r_currentRenderHDRImage);
	glGenTextures(1, &r_currentRenderHDRImage);
	glBindTexture(GL_TEXTURE_2D, r_currentRenderHDRImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, vid.width, vid.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDeleteTextures(1, &r_currentRenderHDRImage64);
	glGenTextures(1, &r_currentRenderHDRImage64);
	glBindTexture(GL_TEXTURE_2D, r_currentRenderHDRImage64);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDeleteTextures(1, &r_currentDepthRenderImage);
	glGenTextures(1, &r_currentDepthRenderImage);
	glBindTexture(GL_TEXTURE_2D, r_currentDepthRenderImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, vid.width, vid.height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDeleteTextures(1, &r_currentAORenderImage);
	glGenTextures(1, &r_currentAORenderImage);
	glBindTexture(GL_TEXTURE_2D, r_currentAORenderImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, vid.width, vid.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDeleteTextures(1, &r_bloomRenderImage[0]);
	glGenTextures(1, &r_bloomRenderImage[0]);
	glBindTexture(GL_TEXTURE_2D, r_bloomRenderImage[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, vid.width, vid.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDeleteTextures(1, &r_bloomRenderImage[1]);
	glGenTextures(1, &r_bloomRenderImage[1]);
	glBindTexture(GL_TEXTURE_2D, r_bloomRenderImage[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, vid.width, vid.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDeleteTextures(1, &r_brightPassRenderImage);
	glGenTextures(1, &r_brightPassRenderImage);
	glBindTexture(GL_TEXTURE_2D, r_brightPassRenderImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, vid.width * 0.25f, vid.height * 0.25f, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	// set up the screen vbo and vao
	r_warpmaxs = 400;
	r_warpmaxt = (r_warpmaxs * vid.height) / vid.width;

	wwverts[0].position[0] = 0;
	wwverts[0].position[1] = 0;
	wwverts[0].texcoord[0] = 0;
	wwverts[0].texcoord[1] = r_warpmaxt;

	wwverts[1].position[0] = vid.width;
	wwverts[1].position[1] = 0;
	wwverts[1].texcoord[0] = r_warpmaxs;
	wwverts[1].texcoord[1] = r_warpmaxt;

	wwverts[2].position[0] = vid.width;
	wwverts[2].position[1] = vid.height;
	wwverts[2].texcoord[0] = r_warpmaxs;
	wwverts[2].texcoord[1] = 0;

	wwverts[3].position[0] = 0;
	wwverts[3].position[1] = vid.height;
	wwverts[3].texcoord[0] = 0;
	wwverts[3].texcoord[1] = 0;

	glGenBuffers (1, &r_postvbo);
	glNamedBufferDataEXT (r_postvbo, sizeof (wwvert_t) * 4, wwverts, GL_STATIC_DRAW);

	glGenVertexArrays (1, &r_postvao);

	// 0 and 2 so that it can share layout with 2d drawing shaders
	glEnableVertexArrayAttribEXT (r_postvao, 0);
	glVertexArrayVertexAttribOffsetEXT (r_postvao, r_postvbo, 0, 2, GL_FLOAT, GL_FALSE, sizeof (wwvert_t), 0);

	glEnableVertexArrayAttribEXT (r_postvao, 2);
	glVertexArrayVertexAttribOffsetEXT (r_postvao, r_postvbo, 2, 2, GL_FLOAT, GL_FALSE, sizeof (wwvert_t), 8);

	// set up shader uniforms
	u_compositeMode = glGetUniformLocation(gl_compositeprog, "compositeMode");
	u_compositeTexScale = glGetUniformLocation(gl_compositeprog, "texScale");
	u_compositeHDRParam = glGetUniformLocation(gl_compositeprog, "hdrParam");
	u_compositeBrightParam = glGetUniformLocation(gl_compositeprog, "brightParam");

	u_postsurfcolor = glGetUniformLocation (gl_postprog, "surfcolor");
	u_postTexScale = glGetUniformLocation (gl_postprog, "texScale");
	u_postwaterwarpparam = glGetUniformLocation (gl_postprog, "waterwarpParam");
	u_postwaterwarp = glGetUniformLocation (gl_postprog, "waterwarppost");

	glProgramUniform1i (gl_compositeprog, glGetUniformLocation (gl_compositeprog, "diffuse"), 0);
	glProgramUniform2f (gl_compositeprog, glGetUniformLocation (gl_compositeprog, "rescale"), 1.0 / r_warpmaxs, 1.0 / r_warpmaxt);
	glProgramUniformMatrix4fv (gl_compositeprog, glGetUniformLocation (gl_compositeprog, "orthomatrix"), 1, GL_FALSE, r_drawmatrix.m[0]);

	glProgramUniform1i (gl_postprog, glGetUniformLocation (gl_postprog, "diffuse"), 0);
	glProgramUniform1i (gl_postprog, glGetUniformLocation (gl_postprog, "precomposite"), 1);
	glProgramUniform1i (gl_postprog, glGetUniformLocation (gl_postprog, "warpgradient"), 2);
	glProgramUniform2f (gl_postprog, glGetUniformLocation (gl_postprog, "rescale"), 1.0 / r_warpmaxs, 1.0 / r_warpmaxt);
	glProgramUniformMatrix4fv (gl_postprog, glGetUniformLocation (gl_postprog, "orthomatrix"), 1, GL_FALSE, r_drawmatrix.m[0]);
}

void RPostProcess_Init(void)
{
	r_hdrAutoExposure = Cvar_Get("r_hdrAutoExposure", "0", 0);
	r_hdrKey = Cvar_Get("r_hdrKey", "0.2", 0);
	r_hdrMinLuminance = Cvar_Get("r_hdrMinLuminance", "0.18", 0);
	r_hdrMaxLuminance = Cvar_Get("r_hdrMaxLuminance", "300", 0);
	r_hdrClampLuminance = Cvar_Get("r_hdrClampLuminance", "1", 0);
	r_exposure = Cvar_Get("r_exposure", "1.0", 0);
	r_hdrContrastThreshold = Cvar_Get("r_hdrContrastThreshold", "0.4", 0);
	r_hdrContrastOffset = Cvar_Get("r_hdrContrastOffset", "100", 0);
	r_debugHdrAdaptation = Cvar_Get("r_debugHdrAdaptation", "0", 0);
	r_debugHdrHistogram = Cvar_Get("r_debugHdrHistogram", "0", 0);
}

qboolean r_dopostprocessing = false;
qboolean r_dowaterwarppost = false;

void RPostProcess_Begin(void)
{
	mleaf_t *leaf;

	r_dopostprocessing = false;
	r_dowaterwarppost = false;

	if (!r_worldmodel) return;
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL) return;
	if (!hdrRenderFBO) return;

	// bind HDR framebuffer object
	R_BindFBO(hdrRenderFBO);

	r_dopostprocessing = true;

	// see if we are underwater 
	leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
	if ((leaf->contents & CONTENTS_WATER) || (leaf->contents & CONTENTS_LAVA) || (leaf->contents & CONTENTS_SLIME))
	{
		r_dowaterwarppost = true;
	}

	// clear out color in framebuffer object before we start drawing to it
	GL_Clear(GL_COLOR_BUFFER_BIT);
}

#define HISTOGRAM_SIZE 256
float rp_histogramValues[HISTOGRAM_SIZE];

float rp_hdrKey = 0;
float rp_hdrAverageLuminance = 0;
float rp_hdrMaxLuminance = 0;
float rp_hdrTime = 0;

static float weight(float val)
{
	if(val <= 0.5)
		return val * 2.0;
	else
		return (1.0 - val) * 2.0;
}

static void RPostProcess_CalculateAdaptation(void)
{
	static float	image[64 * 64 * 4];
	float           curTime;
	float			deltaTime;
	float           luminance;
	float			avgLuminance;
	float			maxLuminance;
	const vec3_t    LUMINANCE_SRGB = { 0.2125f, 0.7154f, 0.0721f }; // be careful wether this should be linear RGB or sRGB
	float			newAdaptation;
	float			newMaximum;

	if (!r_hdrAutoExposure->value)
	{
		// no dynamic exposure
		rp_hdrKey = r_hdrKey->value;
		rp_hdrAverageLuminance = r_hdrMinLuminance->value;
		rp_hdrMaxLuminance = 1;
	}
	else
	{
		curTime = Sys_Milliseconds() / 1000.0f;

		// calculate the average scene luminance
		R_BindFBO(hdrDownscale64);

		// read back the contents
		glReadPixels(0, 0, 64, 64, GL_RGBA, GL_FLOAT, image);

#if 0
		vec4_t color;
		double sum = 0.0f;
		maxLuminance = 0.0f;
		for (int i = 0; i < (64 * 64); i += 4)
		{
			color[0] = image[i * 4 + 0];
			color[1] = image[i * 4 + 1];
			color[2] = image[i * 4 + 2];
			color[3] = image[i * 4 + 3];

			luminance = weight( DotProduct(color, LUMINANCE_SRGB) );
			if (luminance > maxLuminance)
				maxLuminance = luminance;

			sum += log(luminance) + 1e-6;
		}
		sum /= (64.0f * 64.0f);
		avgLuminance = exp(sum);
#else
		// find min max
		float min = FLT_MAX;
		float max = -FLT_MAX;
		int size = 64 * 64;
		for (int i = 0; i < size; ++i)
		{
			vec3_t color;
			color[0] = image[4 * i + 0];
			color[1] = image[4 * i + 1];
			color[2] = image[4 * i + 2];

			// grab weighted luminance values
			luminance = weight(DotProduct(color, LUMINANCE_SRGB));

			// set min and max luminance values
			if (luminance < min)
				min = luminance;
			if (luminance > max)
				max = luminance;

			image[4 * i + 0] = luminance; // write back for later use
		}

		// clear the histogram
		memset(rp_histogramValues, 0, HISTOGRAM_SIZE * sizeof(float));

		// build the histogram
		float hmin = 0.0f;
		float hmax = 5.0f;
		float diff = hmax - hmin;
		float step = diff / HISTOGRAM_SIZE;
		float increment = 1.0f / (float)size;
		for (int i = 0; i < size; ++i)
		{
			float l = image[4 * i + 0];
			int bin = (l - hmin) / step;
			if (bin < 0)
				bin = 0;
			if (bin >= HISTOGRAM_SIZE)
				bin = HISTOGRAM_SIZE - 1;
			rp_histogramValues[bin] += increment;
		}

		// capture average log luminance value and max luminance value from bin
		float binmax = 0.0f;
		float avgLogLum = 0.0f;
		for (int i = 0; i<HISTOGRAM_SIZE; ++i)
		{
			float v = rp_histogramValues[i];

			// set max luminance value
			if (v > binmax)
				binmax = v;
	
			// add log luminance and add 1e-6, so we don't get any odd rounding issues
			avgLogLum += log(luminance) + 1e-6;
		}
		avgLogLum /= (float)size;
		avgLuminance = exp(avgLogLum);

		maxLuminance = binmax;

		if (r_debugHdrHistogram->value)
		{
			VID_Printf(PRINT_ALL, "Darkest pixel = %5.3f, Brightest pixel = %5.3f\n", min, max);
			VID_Printf(PRINT_ALL, " %i%% of pixels, UpperBound = %1.3f\n", (int)(100.0 * binmax), hmax);
		}
#endif
		// clamp average and max luminance values
		if (r_hdrClampLuminance->value)
		{
			rp_hdrAverageLuminance = clamp(r_hdrMinLuminance->value, r_hdrMaxLuminance->value, rp_hdrAverageLuminance);
			avgLuminance = clamp(r_hdrMinLuminance->value, r_hdrMaxLuminance->value, avgLuminance);

			rp_hdrMaxLuminance = clamp(r_hdrMinLuminance->value, r_hdrMaxLuminance->value, rp_hdrMaxLuminance);
			maxLuminance = clamp(r_hdrMinLuminance->value, r_hdrMaxLuminance->value, maxLuminance);
		}

		// the adapted luminance level is simulated by closing the gap between
		// adapted luminance and current luminance by 2% every frame, based on a
		// 30 fps rate. This is not an accurate model of human adaptation, which can
		// take longer than half an hour.
		if (rp_hdrTime > curTime)
			rp_hdrTime = curTime;
		deltaTime = curTime - rp_hdrTime;

		newAdaptation = rp_hdrAverageLuminance + (avgLuminance - rp_hdrAverageLuminance) * (1.0f - powf(0.98f, 30.0f * deltaTime));
		newMaximum = rp_hdrMaxLuminance + (maxLuminance - rp_hdrMaxLuminance) * (1.0f - powf(0.98f, 30.0f * deltaTime));
		if (!IsNAN(newAdaptation) && !IsNAN(newMaximum))
		{
			rp_hdrAverageLuminance = newAdaptation;
			rp_hdrMaxLuminance = newMaximum;
		}

		rp_hdrTime = curTime;

		// calculate HDR image key
		if (r_hdrAutoExposure->value)
		{
			// calculation from: Perceptual Effects in Real-time Tone Mapping - Krawczyk et al.
			rp_hdrKey = 1.03 - (2.0 / (2.0 + (rp_hdrAverageLuminance + 1.0f)));
		}
		else
		{
			rp_hdrKey = r_hdrKey->value;
		}
	}

	if (r_debugHdrAdaptation->value)
	{
		VID_Printf(PRINT_ALL, "HDR luminance avg = %f, max = %f, key = %f\n", rp_hdrAverageLuminance, rp_hdrMaxLuminance, rp_hdrKey);
	}
}

static void RPostProcess_DoTonemap(void)
{
	int flip = 0;
	vec2_t texScale;
	vec4_t hdrParam;

	R_BindNullFBO();

	GL_Enable(BLEND_BIT);

	texScale[0] = 1.0f / vid.width;
	texScale[1] = 1.0f / vid.height;

	GL_UseProgram(gl_compositeprog);

	if (r_hdrAutoExposure->value)
	{
		float exposureOffset = lerp(-0.01f, 0.02f, clamp(0.0, 1.0, r_exposure->value));
		hdrParam[0] = rp_hdrKey + exposureOffset;
		hdrParam[1] = rp_hdrAverageLuminance;
		hdrParam[2] = rp_hdrMaxLuminance;
		hdrParam[3] = exposureOffset;
	}
	else
	{
		float exposureOffset = lerp(-0.01f, 0.01f, clamp(0.0, 1.0, r_exposure->value));
		hdrParam[0] = 0.015f + exposureOffset;
		hdrParam[1] = 0.005f;
		hdrParam[2] = 1;
		hdrParam[3] = 1;
	}
	glProgramUniform1i(gl_compositeprog, u_compositeMode, 2);
	glProgramUniform2f(gl_compositeprog, u_compositeTexScale, texScale[0], texScale[1]);
	glProgramUniform4f(gl_compositeprog, u_compositeHDRParam, hdrParam[0], hdrParam[1], hdrParam[2], hdrParam[3]);

	GL_BindTexture(GL_TEXTURE0, GL_TEXTURE_2D, r_drawclampsampler, r_currentRenderHDRImage);
	
	GL_BindVertexArray(r_postvao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

static void RPostProcess_DoBloom(void)
{
	int i, flip = 0;
	vec2_t texScale;
	vec4_t hdrParam, brightParam, waterwarpParam;

	// set screen scale
	texScale[0] = 1.0f / vid.width;
	texScale[1] = 1.0f / vid.height;

	GL_Enable(BLEND_BIT);

	// do downscaled brightpass and tonemap
	GL_UseProgram(gl_compositeprog);

	hdrParam[0] = rp_hdrKey;
	hdrParam[1] = rp_hdrAverageLuminance;
	hdrParam[2] = rp_hdrMaxLuminance;
	hdrParam[3] = 1.0f;
	glProgramUniform4f(gl_compositeprog, u_compositeHDRParam, hdrParam[0], hdrParam[1], hdrParam[2], hdrParam[3]);

	brightParam[0] = r_hdrContrastThreshold->value;
	brightParam[1] = r_hdrContrastOffset->value;
	brightParam[2] = 0;
	brightParam[3] = 0;
	glProgramUniform4f(gl_compositeprog, u_compositeBrightParam, brightParam[0], brightParam[1], brightParam[2], brightParam[3]);
	glProgramUniform2f(gl_compositeprog, u_compositeTexScale, texScale[0], texScale[1]);
	glProgramUniform1i(gl_compositeprog, u_compositeMode, 1);
	GL_BindTexture(GL_TEXTURE0, GL_TEXTURE_2D, r_drawclampsampler, r_currentRenderHDRImage);

	R_BindFBO(brightpassRenderFBO);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GL_Clear(GL_COLOR_BUFFER_BIT);

	GL_BindVertexArray(r_postvao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// hdr chromatic glare
	GL_BindTexture(GL_TEXTURE0, GL_TEXTURE_2D, r_drawclampsampler, r_brightPassRenderImage);

	for (i = 0; i < 8; i++)
	{
		texScale[0] = 1.0f / bloomRenderFBO[flip]->width;
		texScale[1] = 1.0f / bloomRenderFBO[flip]->height;

		R_BindFBO(bloomRenderFBO[flip]);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		GL_Clear(GL_COLOR_BUFFER_BIT);

		GL_Enable(BLEND_BIT);

		GL_UseProgram(gl_compositeprog);
		glProgramUniform1i(gl_compositeprog, u_compositeMode, 3);
		glProgramUniform2f(gl_compositeprog, u_compositeTexScale, texScale[0], texScale[1]);

		GL_BindVertexArray(r_postvao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		GL_BindTexture(GL_TEXTURE0, GL_TEXTURE_2D, r_drawclampsampler, r_bloomRenderImage[flip]);
		flip ^= 1;
	}

	R_BindNullFBO();

	// then we can do a final post-process pass
	if (v_blend[3])
		glProgramUniform4f(gl_postprog, u_postsurfcolor, v_blend[0], v_blend[1], v_blend[2], v_blend[3] * 0.5);
	else
		glProgramUniform4f(gl_postprog, u_postsurfcolor, 0, 0, 0, 0);

	texScale[0] = 1.0f / vid.width;
	texScale[1] = 1.0f / vid.height;
	glProgramUniform2f(gl_postprog, u_postTexScale, texScale[0], texScale[1]);

	waterwarpParam[0] = r_newrefdef.time * (128.0 / M_PI);
	waterwarpParam[1] = 2.0f;
	waterwarpParam[2] = M_PI / 128.0;
	waterwarpParam[3] = 0.125f;
	glProgramUniform4f(gl_postprog, u_postwaterwarpparam, waterwarpParam[0], waterwarpParam[1], waterwarpParam[2], waterwarpParam[3]);
	if (r_dowaterwarppost)
	{
		glProgramUniform1i(gl_postprog, u_postwaterwarp, 1);
	}
	else
	{
		glProgramUniform1i(gl_postprog, u_postwaterwarp, 0);
	}

	GL_Enable(BLEND_BIT);
	GL_BindTexture(GL_TEXTURE1, GL_TEXTURE_2D, r_drawclampsampler, r_currentRenderHDRImage);
	GL_BindTexture(GL_TEXTURE2, GL_TEXTURE_2D, r_drawwrapsampler, r_warpGradientImage);
	GL_BindVertexArray(r_postvao);
	GL_UseProgram(gl_postprog);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void RPostProcess_FinishToScreen(void)
{
	if (!r_worldmodel) return;
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL) return;
	if (!hdrRenderFBO) return;
	if (!r_dopostprocessing) return;

	R_BindNullFBO();

	// TODO
	// SSAO?

	// blit current hdr framebuffer into downscaled 64x64 texture
	// so we can do automatic exposure adaptation
	glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, hdrRenderFBO->frameBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, hdrDownscale64->frameBuffer);
	glBlitFramebuffer(0, 0, vid.width, vid.height, 0, 0, 64, 64, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// grab our eye adaptation levels
	RPostProcess_CalculateAdaptation ();

	// convert back from HDR to LDR range by tonemapping
	RPostProcess_DoTonemap ();

	// composite into bloom
	RPostProcess_DoBloom ();

	// TODO
	// MOTION BLUR?
	// DOF?
	// FXAA?

	// disable polyblend because we get it for free here
	v_blend[3] = 0;
}
