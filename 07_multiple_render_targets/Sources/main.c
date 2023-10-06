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
static kinc_g4_render_target_t target0;
static kinc_g4_render_target_t target1;
static kinc_g4_render_target_t target2;
static kinc_g4_render_target_t target3;
extern uint32_t *kinc_internal_g1_image;
static uint8_t pixels0[512 * 384 * 4];
static uint8_t pixels1[512 * 384 * 4];
static uint8_t pixels2[512 * 384 * 4];
static uint8_t pixels3[512 * 384 * 4];

#define HEAP_SIZE 1024 * 1024
static uint8_t *heap = NULL;
static size_t heap_top = 0;

static void *allocate(size_t size) {
	size_t old_top = heap_top;
	heap_top += size;
	assert(heap_top <= HEAP_SIZE);
	return &heap[old_top];
}

static void draw_image(uint8_t *pixels, int x, int y) {
	uint8_t *g1image = (uint8_t *)kinc_internal_g1_image;
	for (int i = 0; i < 512 * 384; ++i) {
		int xpos = i % 512 + x;
		int ypos = i / 512 + y;
		// if (kinc_g4_render_targets_inverted_y()) {}
		int j = ypos * 1024 + xpos;
		g1image[j * 4] = pixels[i * 4];
		g1image[j * 4 + 1] = pixels[i * 4 + 1];
		g1image[j * 4 + 2] = pixels[i * 4 + 2];
		g1image[j * 4 + 3] = pixels[i * 4 + 3];
	}
}

static void update(void *data) {
	kinc_g4_render_target_t *targets[] = {&target0, &target1, &target2, &target3};
	kinc_g4_set_render_targets(targets, 4);
	kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0, 0.0f, 0);
	kinc_g4_set_pipeline(&pipeline);
	kinc_g4_set_vertex_buffer(&vertices);
	kinc_g4_set_index_buffer(&indices);
	kinc_g4_draw_indexed_vertices();
	kinc_g4_restore_render_target();

	kinc_g4_render_target_get_pixels(&target0, pixels0);
	kinc_g4_render_target_get_pixels(&target1, pixels1);
	kinc_g4_render_target_get_pixels(&target2, pixels2);
	kinc_g4_render_target_get_pixels(&target3, pixels3);

	kinc_g1_begin();
	draw_image(pixels0, 0, 0);
	draw_image(pixels1, 512, 0);
	draw_image(pixels2, 0, 384);
	draw_image(pixels3, 512, 384);
	kinc_g1_end();
}

int kickstart(int argc, char **argv) {
	kinc_init("Example", 1024, 768, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	heap = (uint8_t *)malloc(HEAP_SIZE);
	assert(heap != NULL);

	kinc_g4_render_target_init(&target0, 512, 384, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);
	kinc_g4_render_target_init(&target1, 512, 384, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);
	kinc_g4_render_target_init(&target2, 512, 384, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);
	kinc_g4_render_target_init(&target3, 512, 384, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);
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
