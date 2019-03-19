#include <afina/concurrency/Executor.h>

namespace Afina {
namespace Concurrency {

Executor::Executor(unsigned int low_watermark,
                   unsigned int high_watermark,
                   unsigned int max_queue_size,
                   size_t idle_time) {
  _low_watermark = low_watermark;
  _high_watermark = high_watermark;
  _max_queue_size = max_queue_size;
  _idle_time = idle_time;
}

void Executor::Start() {
  for (unsigned int worker_id = 0; worker_id < _low_watermark; ++worker_id)
    threads.emplace_back(perform, this);
  _n_existing_workers = _low_watermark;
  _n_free_workers = _low_watermark;
}

void perform(Afina::Concurrency::Executor *executor) {
  for (;;) {
    std::unique_lock<std::mutex> lock (executor->mutex);

    executor->empty_condition.wait_until()
  }
}

}
} // namespace Afina
