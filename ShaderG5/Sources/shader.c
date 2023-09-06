#include <kinc/graphics5/commandlist.h>
#include <kinc/graphics5/graphics.h>
#include <kinc/graphics5/indexbuffer.h>
#include <kinc/graphics5/pipeline.h>
#include <kinc/graphics5/shader.h>
#include <kinc/graphics5/vertexbuffer.h>
#include <kinc/io/filereader.h>
#include <kinc/system.h>
#include <kinc/window.h>

#include <kong.h>

#include <assert.h>

#define BUFFER_COUNT 2
static int currentBuffer = -1;
static kinc_g5_render_target_t framebuffers[BUFFER_COUNT];
static kinc_g5_vertex_buffer_t vertices;
static kinc_g5_index_buffer_t indices;
static kinc_g5_command_list_t commandList;

#define HEAP_SIZE 1024 * 1024
static uint8_t *heap = NULL;
static size_t heap_top = 0;

static void *allocate(size_t size) {
	size_t old_top = heap_top;
	heap_top += size;
	assert(heap_top <= HEAP_SIZE);
	return &heap[old_top];
}

static bool first_update = true;

static void update(void *data) {
	currentBuffer = (currentBuffer + 1) % BUFFER_COUNT;

	kinc_g5_begin(&framebuffers[currentBuffer], 0);

	kinc_g5_command_list_begin(&commandList);

	if (first_update) {
		kinc_g5_command_list_upload_vertex_buffer(&commandList, &vertices);
		kinc_g5_command_list_upload_index_buffer(&commandList, &indices);
	}

	kinc_g5_command_list_framebuffer_to_render_target_barrier(&commandList, &framebuffers[currentBuffer]);

	kinc_g5_render_target_t *p_framebuffer = &framebuffers[currentBuffer];
	kinc_g5_command_list_set_render_targets(&commandList, &p_framebuffer, 1);
	kinc_g5_command_list_clear(&commandList, &framebuffers[currentBuffer], KINC_G5_CLEAR_COLOR, 0, 0.0f, 0);
	kinc_g5_command_list_set_pipeline(&commandList, &pipeline.impl._pipeline);

	int offset = 0;
	kinc_g5_vertex_buffer_t *p_vertices = &vertices;
	kinc_g5_command_list_set_vertex_buffers(&commandList, &p_vertices, &offset, 1);
	kinc_g5_command_list_set_index_buffer(&commandList, &indices);
	kinc_g5_command_list_draw_indexed_vertices(&commandList);

	kinc_g5_command_list_render_target_to_framebuffer_barrier(&commandList, &framebuffers[currentBuffer]);
	kinc_g5_command_list_end(&commandList);

	kinc_g5_command_list_execute(&commandList);

	kinc_g5_end(0);
	kinc_g5_swap_buffers();
}

int kickstart(int argc, char **argv) {
	kinc_init("Shader", 1024, 768, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	heap = (uint8_t *)malloc(HEAP_SIZE);
	assert(heap != NULL);

	kinc_g5_command_list_init(&commandList);
	for (int i = 0; i < BUFFER_COUNT; ++i) {
		kinc_g5_render_target_init_framebuffer(&framebuffers[i], kinc_window_width(0), kinc_window_height(0), KINC_G5_RENDER_TARGET_FORMAT_32BIT, 16, 0);
	}

	kinc_g5_vertex_buffer_init(&vertices, 3, &vertex_in_structure, true, 0);
	{
		vertex_in *v = (vertex_in *)kinc_g5_vertex_buffer_lock_all(&vertices);

		v[0].pos.x = -1;
		v[0].pos.y = -1;
		v[0].pos.z = 0.5;

		v[1].pos.x = 1;
		v[1].pos.y = -1;
		v[1].pos.z = 0.5;

		v[2].pos.x = -1;
		v[2].pos.y = 1;
		v[2].pos.z = 0.5;

		kinc_g5_vertex_buffer_unlock_all(&vertices);
	}

	kinc_g5_index_buffer_init(&indices, 3, KINC_G5_INDEX_BUFFER_FORMAT_16BIT, true);
	{
		uint16_t *i = (uint16_t *)kinc_g5_index_buffer_lock_all(&indices);
		i[0] = 0;
		i[1] = 1;
		i[2] = 2;
		kinc_g5_index_buffer_unlock_all(&indices);
	}

	kinc_start();

	return 0;
}
