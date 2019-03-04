// SPDX-License-Identifier: MPL-2.0
// Copyright (c) Yuxuan Shui <yshuiv7@gmail.com>
#pragma once
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdbool.h>
#include <string.h>

#include "backend/backend.h"
#include "config.h"
#include "log.h"
#include "region.h"

#define CASESTRRET(s)                                                                    \
	case s: return #s

// Program and uniforms for window shader
typedef struct {
	/// GLSL program.
	GLuint prog;
	/// Location of uniform "opacity" in window GLSL program.
	GLint unifm_opacity;
	/// Location of uniform "invert_color" in blur GLSL program.
	GLint unifm_invert_color;
	/// Location of uniform "tex" in window GLSL program.
	GLint unifm_tex;
} gl_win_shader_t;

// Program and uniforms for blur shader
typedef struct {
	/// Fragment shader for blur.
	GLuint frag_shader;
	/// GLSL program for blur.
	GLuint prog;
	/// Location of uniform "offset_x" in blur GLSL program.
	GLint unifm_offset_x;
	/// Location of uniform "offset_y" in blur GLSL program.
	GLint unifm_offset_y;
	/// Location of uniform "factor_center" in blur GLSL program.
	GLint unifm_factor_center;
} gl_blur_shader_t;

/// @brief Wrapper of a binded GLX texture.
typedef struct gl_texture {
	double opacity;
	GLuint texture;
	GLenum target;
	unsigned width;
	unsigned height;
	unsigned depth;
	bool y_inverted;
	bool has_alpha;
	bool color_inverted;
} gl_texture_t;

struct gl_data {
	backend_t base;
	// Height and width of the viewport
	int height, width;
	gl_win_shader_t win_shader;
	gl_blur_shader_t blur_shader[MAX_BLUR_PASS];
	bool non_power_of_two_texture;
};

typedef struct {
	/// Framebuffer used for blurring.
	GLuint fbo;
	/// Textures used for blurring.
	GLuint textures[2];
	/// Width of the textures.
	int width;
	/// Height of the textures.
	int height;
} gl_blur_cache_t;

typedef struct session session_t;

#define GL_PROG_MAIN_INIT                                                                \
	{ .prog = 0, .unifm_opacity = -1, .unifm_invert_color = -1, .unifm_tex = -1, }

GLuint gl_create_shader(GLenum shader_type, const char *shader_str);
GLuint gl_create_program(const GLuint *const shaders, int nshaders);
GLuint gl_create_program_from_str(const char *vert_shader_str, const char *frag_shader_str);

/**
 * @brief Render a region with texture data.
 */
void gl_compose(backend_t *, void *ptex, int dst_x, int dst_y, const region_t *reg_tgt,
                const region_t *reg_visible);

bool gl_load_prog_main(session_t *ps, const char *vshader_str, const char *fshader_str,
                       gl_win_shader_t *pprogram);

unsigned char *gl_take_screenshot(session_t *ps, int *out_length);
void gl_resize(struct gl_data *, int width, int height);
//bool gl_create_blur_filters(session_t *ps, gl_blur_shader_t *passes, const gl_cap_t *cap);

GLuint glGetUniformLocationChecked(GLuint p, const char *name);

bool gl_init(struct gl_data *gd, session_t *);
void gl_deinit(struct gl_data *gd);

GLuint gl_new_texture(GLenum target);

bool gl_image_op(backend_t *base, enum image_operations op, void *image_data,
                 const region_t *reg_op, const region_t *reg_visible, void *arg);

void *gl_copy(backend_t *base, const void *image_data, const region_t *reg_visible);

bool gl_blur(backend_t *base, double opacity, const region_t *reg_blur,
             const region_t *reg_visible);

bool gl_is_image_transparent(backend_t *base, void *image_data);

static inline void gl_delete_texture(GLuint texture) {
	glDeleteTextures(1, &texture);
}

/**
 * Get a textual representation of an OpenGL error.
 */
static inline const char *gl_get_err_str(GLenum err) {
	switch (err) {
		CASESTRRET(GL_NO_ERROR);
		CASESTRRET(GL_INVALID_ENUM);
		CASESTRRET(GL_INVALID_VALUE);
		CASESTRRET(GL_INVALID_OPERATION);
		CASESTRRET(GL_INVALID_FRAMEBUFFER_OPERATION);
		CASESTRRET(GL_OUT_OF_MEMORY);
		CASESTRRET(GL_STACK_UNDERFLOW);
		CASESTRRET(GL_STACK_OVERFLOW);
	}
	return NULL;
}

/**
 * Check for GLX error.
 *
 * http://blog.nobel-joergensen.com/2013/01/29/debugging-opengl-using-glgeterror/
 */
static inline void gl_check_err_(const char *func, int line) {
	GLenum err = GL_NO_ERROR;

	while (GL_NO_ERROR != (err = glGetError())) {
		const char *errtext = gl_get_err_str(err);
		if (errtext) {
			log_printf(tls_logger, LOG_LEVEL_ERROR, func,
			           "GLX error at line %d: %s", line, errtext);
		} else {
			log_printf(tls_logger, LOG_LEVEL_ERROR, func,
			           "GLX error at line %d: %d", line, err);
		}
	}
}

#define gl_check_err() gl_check_err_(__func__, __LINE__)

/**
 * Check if a GLX extension exists.
 */
static inline bool gl_has_extension(const char *ext) {
	GLint nexts = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nexts);
	if (!nexts) {
		log_error("Failed to get GL extension list.");
		return false;
	}

	for (int i = 0; i < nexts; i++) {
		const char *exti = (const char *)glGetStringi(GL_EXTENSIONS, i);
		if (strcmp(ext, exti) == 0)
			return true;
	}
	log_info("Missing GL extension %s.", ext);
	return false;
}
