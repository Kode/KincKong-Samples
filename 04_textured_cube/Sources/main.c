#include <kinc/graphics4/graphics.h>
#include <kinc/graphics4/indexbuffer.h>
#include <kinc/graphics4/pipeline.h>
#include <kinc/graphics4/shader.h>
#include <kinc/graphics4/texture.h>
#include <kinc/graphics4/vertexbuffer.h>
#include <kinc/io/filereader.h>
#include <kinc/log.h>
#include <kinc/system.h>
#include <kinc/window.h>

#include <kong.h>

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static kinc_g4_vertex_buffer_t vertices;
static kinc_g4_index_buffer_t indices;
static kinc_g4_texture_t texture;
static constants_type_buffer constants;

#define HEAP_SIZE 1024 * 1024 * 2
static uint8_t *heap = NULL;
static size_t heap_top = 0;

/* clang-format off */
static float vertices_data[] = {
    -1.0,-1.0,-1.0,
	-1.0,-1.0, 1.0,
	-1.0, 1.0, 1.0,
	 1.0, 1.0,-1.0,
	-1.0,-1.0,-1.0,
	-1.0, 1.0,-1.0,
	 1.0,-1.0, 1.0,
	-1.0,-1.0,-1.0,
	 1.0,-1.0,-1.0,
	 1.0, 1.0,-1.0,
	 1.0,-1.0,-1.0,
	-1.0,-1.0,-1.0,
	-1.0,-1.0,-1.0,
	-1.0, 1.0, 1.0,
	-1.0, 1.0,-1.0,
	 1.0,-1.0, 1.0,
	-1.0,-1.0, 1.0,
	-1.0,-1.0,-1.0,
	-1.0, 1.0, 1.0,
	-1.0,-1.0, 1.0,
	 1.0,-1.0, 1.0,
	 1.0, 1.0, 1.0,
	 1.0,-1.0,-1.0,
	 1.0, 1.0,-1.0,
	 1.0,-1.0,-1.0,
	 1.0, 1.0, 1.0,
	 1.0,-1.0, 1.0,
	 1.0, 1.0, 1.0,
	 1.0, 1.0,-1.0,
	-1.0, 1.0,-1.0,
	 1.0, 1.0, 1.0,
	-1.0, 1.0,-1.0,
	-1.0, 1.0, 1.0,
	 1.0, 1.0, 1.0,
	-1.0, 1.0, 1.0,
	 1.0,-1.0, 1.0
};

static float tex_data[] = {
    0.000059f, 0.000004f,
	0.000103f, 0.336048f,
	0.335973f, 0.335903f,
	1.000023f, 0.000013f,
	0.667979f, 0.335851f,
	0.999958f, 0.336064f,
	0.667979f, 0.335851f,
	0.336024f, 0.671877f,
	0.667969f, 0.671889f,
	1.000023f, 0.000013f,
	0.668104f, 0.000013f,
	0.667979f, 0.335851f,
	0.000059f, 0.000004f,
	0.335973f, 0.335903f,
	0.336098f, 0.000071f,
	0.667979f, 0.335851f,
	0.335973f, 0.335903f,
	0.336024f, 0.671877f,
	1.000004f, 0.671847f,
	0.999958f, 0.336064f,
	0.667979f, 0.335851f,
	0.668104f, 0.000013f,
	0.335973f, 0.335903f,
	0.667979f, 0.335851f,
	0.335973f, 0.335903f,
	0.668104f, 0.000013f,
	0.336098f, 0.000071f,
	0.000103f, 0.336048f,
	0.000004f, 0.671870f,
	0.336024f, 0.671877f,
	0.000103f, 0.336048f,
	0.336024f, 0.671877f,
	0.335973f, 0.335903f,
	0.667969f, 0.671889f,
	1.000004f, 0.671847f,
	0.667979f, 0.335851f
};
/* clang-format on */

static void *allocate(size_t size) {
	size_t old_top = heap_top;
	heap_top += size;
	assert(heap_top <= HEAP_SIZE);
	return &heap[old_top];
}

float vec4_length(kinc_vector3_t a) {
	return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

kinc_vector3_t vec4_normalize(kinc_vector3_t a) {
	float n = vec4_length(a);
	if (n > 0.0) {
		float inv_n = 1.0f / n;
		a.x *= inv_n;
		a.y *= inv_n;
		a.z *= inv_n;
	}
	return a;
}

kinc_vector3_t vec4_sub(kinc_vector3_t a, kinc_vector3_t b) {
	kinc_vector3_t v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	v.z = a.z - b.z;
	return v;
}

kinc_vector3_t vec4_cross(kinc_vector3_t a, kinc_vector3_t b) {
	kinc_vector3_t v;
	v.x = a.y * b.z - a.z * b.y;
	v.y = a.z * b.x - a.x * b.z;
	v.z = a.x * b.y - a.y * b.x;
	return v;
}

float vec4_dot(kinc_vector3_t a, kinc_vector3_t b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

kinc_matrix4x4_t matrix4x4_perspective_projection(float fovy, float aspect, float zn, float zf) {
	float uh = 1.0f / tanf(fovy / 2);
	float uw = uh / aspect;
	kinc_matrix4x4_t m = {uw, 0, 0, 0, 0, uh, 0, 0, 0, 0, (zf + zn) / (zn - zf), -1, 0, 0, 2 * zf * zn / (zn - zf), 0};
	return m;
}

kinc_matrix4x4_t matrix4x4_look_at(kinc_vector3_t eye, kinc_vector3_t at, kinc_vector3_t up) {
	kinc_vector3_t zaxis = vec4_normalize(vec4_sub(at, eye));
	kinc_vector3_t xaxis = vec4_normalize(vec4_cross(zaxis, up));
	kinc_vector3_t yaxis = vec4_cross(xaxis, zaxis);
	kinc_matrix4x4_t m = {xaxis.x,
	                      yaxis.x,
	                      -zaxis.x,
	                      0,
	                      xaxis.y,
	                      yaxis.y,
	                      -zaxis.y,
	                      0,
	                      xaxis.z,
	                      yaxis.z,
	                      -zaxis.z,
	                      0,
	                      -vec4_dot(xaxis, eye),
	                      -vec4_dot(yaxis, eye),
	                      vec4_dot(zaxis, eye),
	                      1};
	return m;
}

kinc_matrix4x4_t matrix4x4_identity(void) {
	kinc_matrix4x4_t m;
	memset(m.m, 0, sizeof(m.m));
	for (unsigned x = 0; x < 4; ++x) {
		kinc_matrix4x4_set(&m, x, x, 1.0f);
	}
	return m;
}

static void update(void *data) {
	kinc_matrix4x4_t projection = matrix4x4_perspective_projection(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	kinc_vector3_t v0 = {4, 3, 3};
	kinc_vector3_t v1 = {0, 0, 0};
	kinc_vector3_t v2 = {0, 1, 0};
	kinc_matrix4x4_t view = matrix4x4_look_at(v0, v1, v2);
	kinc_matrix4x4_t model = matrix4x4_identity();
	kinc_matrix4x4_t mvp = matrix4x4_identity();
	mvp = kinc_matrix4x4_multiply(&mvp, &projection);
	mvp = kinc_matrix4x4_multiply(&mvp, &view);
	mvp = kinc_matrix4x4_multiply(&mvp, &model);

	constants_type *constants_data = constants_type_buffer_lock(&constants);
	constants_data->mvp = mvp;
	constants_type_buffer_unlock(&constants);

	kinc_g4_begin(0);
	kinc_g4_clear(KINC_G4_CLEAR_COLOR | KINC_G4_CLEAR_DEPTH, 0xff000044, 1.0f, 0);
	kinc_g4_set_pipeline(&pipeline);
	constants_type_buffer_set(&constants);
	kinc_g4_set_texture(pix_texture, &texture);
	kinc_g4_set_vertex_buffer(&vertices);
	kinc_g4_set_index_buffer(&indices);
	kinc_g4_draw_indexed_vertices();

	kinc_g4_end(0);
	kinc_g4_swap_buffers();
}

int kickstart(int argc, char **argv) {
	kinc_init("Example", 1024, 768, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	heap = (uint8_t *)malloc(HEAP_SIZE);
	assert(heap != NULL);

	kinc_image_t image;
	void *image_mem = allocate(512 * 512 * 4);
	kinc_image_init_from_file(&image, image_mem, "uvtemplate.png");
	kinc_g4_texture_init_from_image(&texture, &image);
	kinc_image_destroy(&image);

	// pipeline.depth_write = true;
	// pipeline.depth_mode = KINC_G4_COMPARE_LESS;

	int vertex_count = sizeof(vertices_data) / 3 / 4;
	kinc_g4_vertex_buffer_init(&vertices, vertex_count, &vertex_in_structure, KINC_G4_USAGE_STATIC, 0);
	{
		vertex_in *v = (vertex_in *)kinc_g4_vertex_buffer_lock_all(&vertices);
		for (int i = 0; i < vertex_count; ++i) {
			v[i].pos.x = vertices_data[i * 3];
			v[i].pos.y = vertices_data[i * 3 + 1];
			v[i].pos.z = vertices_data[i * 3 + 2];
			v[i].tex.x = tex_data[i * 2];
			v[i].tex.y = tex_data[i * 2 + 1];
		}
		kinc_g4_vertex_buffer_unlock_all(&vertices);
	}

	kinc_g4_index_buffer_init(&indices, vertex_count, KINC_G4_INDEX_BUFFER_FORMAT_16BIT, KINC_G4_USAGE_STATIC);
	{
		uint16_t *id = (uint16_t *)kinc_g4_index_buffer_lock_all(&indices);
		for (int i = 0; i < vertex_count; ++i) {
			id[i] = i;
		}
		kinc_g4_index_buffer_unlock_all(&indices);
	}

	constants_type_buffer_init(&constants);

	kinc_start();

	return 0;
}
