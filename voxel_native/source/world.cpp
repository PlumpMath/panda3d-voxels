#include <thread>

#include "asyncTaskChain.h"
#include "throw_event.h"

#include "chunk_generation_task.h"
#include "world.h"

World::World(unsigned size_x, unsigned size_y, string done_event_name, string progress_event_name) : NodePath("World"),
task_manager(*AsyncTaskManager::get_global_ptr()),
event_handler(*EventHandler::get_global_event_handler()),
size_x(size_x),
size_y(size_y),
done_event_name(done_event_name),
progress_event_name(progress_event_name)
{
	/*chunks = std::vector<std::vector<PT(Chunk)>>(size_x, std::vector<PT(Chunk)>(size_y));

	for (int x = 0; x < size_x; x++) {
		auto col = std::vector<PT(Chunk)>(size_y);
		col.resize(size_y);
		for (int y = 0; y < size_y; y++) {
			col[y] = new Chunk();
		}

		chunks[x] = col;
	}*/

	chunks = std::map<ChunkCoord, PT(Chunk)>();

	for (int x = 0; x < size_x; x++) {
		for (int y = 0; y < size_y; y++) {
			chunks[ChunkCoord(x, y)] = new Chunk();
		}
	}

	if (!task_chain_initialized) {
		setup_task_chain();
		task_chain_initialized = true;
	}
}

World::~World() {}

/**
 * Generates the world based on the given world size by adding ChunkGenerationTasks
 * to the multithreaded task chain.
 */
void World::generate() {
	event_handler.add_hook("done_generating_chunk", chunk_done_callback, this);

	for (auto& chunkKeyVal : chunks) {
		PT(ChunkGenerationTask) task = new ChunkGenerationTask(*chunkKeyVal.second, chunkKeyVal.first.first, chunkKeyVal.first.second, "done_generating_chunk");
		task->set_task_chain("ChunkGenTaskChain");
		task_manager.add(task);
	}

	//for (unsigned i = 0; i < size_x; i++) {
	//	for (unsigned j = 0; j < size_y; j++) {
	//		PT(ChunkGenerationTask) task = new ChunkGenerationTask(*chunks[i][j], i, j, "done_generating_chunk");
	//		task->set_task_chain("ChunkGenTaskChain");
	//		task_manager.add(task);
	//	}
	//}
}

void World::chunk_done_callback(const Event *event, void *data) {
	World *self = (World *)data;
	self->num_chunks_finished++;
	throw_event(self->progress_event_name, self->num_chunks_finished);
}

/**
 * Sets up the task chain inferring the number of cores.
 */
void World::setup_task_chain() {
	unsigned num_threads = std::thread::hardware_concurrency();
	if (num_threads != 0) num_threads -= 1; // Giving a thread count of 1 spawns an additional thread, 0 is the main thread.
	World::setup_task_chain(num_threads);
}

/**
 * Sets up the task chain with the specified number of cores.
 */
void World::setup_task_chain(unsigned num_threads) {
	AsyncTaskManager &task_manager = *AsyncTaskManager::get_global_ptr();
	AsyncTaskChain *task_chain = task_manager.make_task_chain("ChunkGenTaskChain");
	task_chain->set_num_threads(num_threads);
	task_chain->set_timeslice_priority(true);
	//task_chain->set_frame_budget(1.0/100.0f);
	//task_chain->set_frame_sync(true);
	//task_chain->set_thread_priority(ThreadPriority::TP_low);
}