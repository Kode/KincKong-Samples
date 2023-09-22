#include <kinc/graphics1/graphics.h>
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
static kinc_g4_render_target_t target;
extern uint32_t *kinc_internal_g1_image;
static uint8_t pixels[1024 * 768 * 4];

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
	kinc_g4_render_target_t *targets = {&target};
	kinc_g4_set_render_targets(&targets, 1);
	kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0, 0.0f, 0);
	kinc_g4_set_pipeline(&pipeline);
	kinc_g4_set_vertex_buffer(&vertices);
	kinc_g4_set_index_buffer(&indices);
	kinc_g4_draw_indexed_vertices();
	kinc_g4_restore_render_target();
	kinc_g4_render_target_get_pixels(&target, pixels);

	kinc_g1_begin();
	uint8_t *g1image = (uint8_t *)kinc_internal_g1_image;
	for (int i = 0; i < 1024 * 768 * 4; ++i) {
		// if (kinc_g4_render_targets_inverted_y()) {}
		g1image[i] = pixels[i];
	}
	kinc_g1_end();
}

int kickstart(int argc, char **argv) {
	kinc_init("Example", 1024, 768, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	heap = (uint8_t *)malloc(HEAP_SIZE);
	assert(heap != NULL);

	kinc_g4_render_target_init(&target, 1024, 768, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);
	kinc_g1_init(1024, 768);

	kinc_g4_vertex_buffer_init(&vertices, 3, &vertex_in_structure, KINC_G4_USAGE_STATIC, 0);
	{
		vertex_in *v = (vertex_in *)kinc_g4_vertex_buffer_lock_all(&vertices);

		v[0].pos.x = -1.0;
		v[0].pos.y = -1.0;
		v[0].pos.z = 0.0;

		v[1].pos.x = 1.0;
		v[1].pos.y = -1.0;
		v[1].pos.z = 0.0;

		v[2].pos.x = 0.0;
		v[2].pos.y = 1.0;
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

	kinc_start();

	return 0;
}
