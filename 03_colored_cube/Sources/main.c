#include <kinc/graphics4/constantbuffer.h>
#include <kinc/graphics4/graphics.h>
#include <kinc/graphics4/indexbuffer.h>
#include <kinc/graphics4/pipeline.h>
#include <kinc/graphics4/shader.h>
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
static constants_type_buffer constants;

#define HEAP_SIZE 1024 * 1024
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

static float colors_data[] = {
    0.583f,  0.771f,  0.014f,
	0.609f,  0.115f,  0.436f,
	0.327f,  0.483f,  0.844f,
	0.822f,  0.569f,  0.201f,
	0.435f,  0.602f,  0.223f,
	0.310f,  0.747f,  0.185f,
	0.597f,  0.770f,  0.761f,
	0.559f,  0.436f,  0.730f,
	0.359f,  0.583f,  0.152f,
	0.483f,  0.596f,  0.789f,
	0.559f,  0.861f,  0.639f,
	0.195f,  0.548f,  0.859f,
	0.014f,  0.184f,  0.576f,
	0.771f,  0.328f,  0.970f,
	0.406f,  0.615f,  0.116f,
	0.676f,  0.977f,  0.133f,
	0.971f,  0.572f,  0.833f,
	0.140f,  0.616f,  0.489f,
	0.997f,  0.513f,  0.064f,
	0.945f,  0.719f,  0.592f,
	0.543f,  0.021f,  0.978f,
	0.279f,  0.317f,  0.505f,
	0.167f,  0.620f,  0.077f,
	0.347f,  0.857f,  0.137f,
	0.055f,  0.953f,  0.042f,
	0.714f,  0.505f,  0.345f,
	0.783f,  0.290f,  0.734f,
	0.722f,  0.645f,  0.174f,
	0.302f,  0.455f,  0.848f,
	0.225f,  0.587f,  0.040f,
	0.517f,  0.713f,  0.338f,
	0.053f,  0.959f,  0.120f,
	0.393f,  0.621f,  0.362f,
	0.673f,  0.211f,  0.457f,
	0.820f,  0.883f,  0.371f,
	0.982f,  0.099f,  0.879f
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

	int vertex_count = sizeof(vertices_data) / 3 / 4;
	kinc_g4_vertex_buffer_init(&vertices, vertex_count, &vertex_in_structure, KINC_G4_USAGE_STATIC, 0);
	{
		vertex_in *v = (vertex_in *)kinc_g4_vertex_buffer_lock_all(&vertices);
		for (int i = 0; i < vertex_count; ++i) {
			v[i].pos.x = vertices_data[i * 3];
			v[i].pos.y = vertices_data[i * 3 + 1];
			v[i].pos.z = vertices_data[i * 3 + 2];
			v[i].col.x = colors_data[i * 3];
			v[i].col.y = colors_data[i * 3 + 1];
			v[i].col.z = colors_data[i * 3 + 2];
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
