#include <kinc/graphics4/graphics.h>
#include <kinc/graphics4/indexbuffer.h>
#include <kinc/graphics4/pipeline.h>
#include <kinc/graphics4/rendertarget.h>
#include <kinc/graphics4/shader.h>
#include <kinc/graphics4/vertexbuffer.h>
#include <kinc/io/filereader.h>
#include <kinc/system.h>

#include <kong.h>

#include <assert.h>
#include <stdlib.h>

static kinc_g4_vertex_buffer_t vertices;
static kinc_g4_index_buffer_t indices;
static kinc_g4_vertex_buffer_t vertices_fs;
static kinc_g4_index_buffer_t indices_fs;
static kinc_g4_render_target_t target;

#define HEAP_SIZE 1024 * 1024
static uint8_t *heap = NULL;
static size_t heap_top = 0;

static void *allocate(size_t size) {
	size_t old_top = heap_top;
	heap_top += size;
	assert(heap_top <= HEAP_SIZE);
	return &heap[old_top];
}

static void update(void *data) {
	kinc_g4_begin(0);

	kinc_g4_render_target_t *targets = {&target};
	kinc_g4_set_render_targets(&targets, 1);
	kinc_g4_clear(KINC_G4_CLEAR_DEPTH, 0, 1.0f, 0);
	kinc_g4_set_pipeline(&pipeline);
	kinc_g4_set_vertex_buffer(&vertices);
	kinc_g4_set_index_buffer(&indices);
	kinc_g4_draw_indexed_vertices();

	kinc_g4_restore_render_target();
	kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0, 0.0f, 0);
	kinc_g4_set_pipeline(&fs_pipeline);
	kinc_g4_render_target_use_color_as_texture(&target, fs_texture);
	// kinc_g4_render_target_use_depth_as_texture(&target, tex_unit);
	kinc_g4_set_vertex_buffer(&vertices_fs);
	kinc_g4_set_index_buffer(&indices_fs);
	kinc_g4_draw_indexed_vertices();

	kinc_g4_end(0);
	kinc_g4_swap_buffers();
}

int kickstart(int argc, char **argv) {
	kinc_init("Example", 1024, 768, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	heap = (uint8_t *)malloc(HEAP_SIZE);
	assert(heap != NULL);

	// pipeline.depth_write = true;
	// pipeline.depth_mode = KINC_G4_COMPARE_LESS;

	kinc_g4_render_target_init(&target, 1024, 768, KINC_G4_RENDER_TARGET_FORMAT_16BIT_DEPTH, 0, 0);
	// kinc_g4_render_target_init(&target, 1024, 768, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 16, 0);

	kinc_g4_vertex_buffer_init(&vertices, 3, &vertex_in_structure, KINC_G4_USAGE_STATIC, 0);
	{
		vertex_in *v = (vertex_in *)kinc_g4_vertex_buffer_lock_all(&vertices);

		v[0].pos.x = -1.0;
		v[0].pos.y = kinc_g4_render_targets_inverted_y() ? -1.0f : 1.0f;
		v[0].pos.z = 0.0;

		v[1].pos.x = 1.0;
		v[1].pos.y = kinc_g4_render_targets_inverted_y() ? -1.0f : 1.0f;
		v[1].pos.z = 0.0;

		v[2].pos.x = 0.0;
		v[2].pos.y = kinc_g4_render_targets_inverted_y() ? 1.0f : -1.0f;
		v[2].pos.z = 0.0;

		kinc_g4_vertex_buffer_unlock_all(&vertices);
	}

	kinc_g4_index_buffer_init(&indices, 3, KINC_G4_INDEX_BUFFER_FORMAT_16BIT, KINC_G4_USAGE_STATIC);
	{
		uint16_t *i = (uint16_t *)kinc_g4_index_buffer_lock_all(&indices);
		i[0] = 0;
		i[1] = 1;
		i[2] = 2;
		kinc_g4_index_buffer_unlock_all(&indices);
	}

	kinc_g4_vertex_buffer_init(&vertices_fs, 3, &fs_vertex_in_structure, KINC_G4_USAGE_STATIC, 0);
	{
		fs_vertex_in *v = (fs_vertex_in *)kinc_g4_vertex_buffer_lock_all(&vertices_fs);

		v[0].pos.x = -1.0;
		v[0].pos.y = -1.0;

		v[1].pos.x = 3.0;
		v[1].pos.y = -1.0;

		v[2].pos.x = -1.0;
		v[2].pos.y = 3.0;

		kinc_g4_vertex_buffer_unlock_all(&vertices_fs);
	}

	kinc_g4_index_buffer_init(&indices_fs, 3, KINC_G4_INDEX_BUFFER_FORMAT_16BIT, KINC_G4_USAGE_STATIC);
	{
		uint16_t *i = (uint16_t *)kinc_g4_index_buffer_lock_all(&indices_fs);
		i[0] = 0;
		i[1] = 1;
		i[2] = 2;
		kinc_g4_index_buffer_unlock_all(&indices_fs);
	}

	kinc_start();

	return 0;
}
