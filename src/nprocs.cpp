#include <tbb/task_arena.h>

// Default to TBB's maximum concurrency; can be overridden by set_nprocs().
int nprocs = tbb::this_task_arena::max_concurrency();

namespace instant_meshes {

void set_nprocs(int value)
{
    if (value <= 0) {
        nprocs = tbb::this_task_arena::max_concurrency();
    } else {
        nprocs = value;
    }
}

int get_nprocs()
{
    return nprocs;
}

} // namespace instant_meshes
