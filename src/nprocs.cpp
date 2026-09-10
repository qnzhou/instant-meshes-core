#include <tbb/task_arena.h>

namespace instant_meshes {

// Number of processors TBB is allowed to use. Lazily initialized on first use rather than in a
// namespace-scope initializer: calling into TBB during static initialization (before TBB's own
// runtime globals are constructed) is a static-initialization-order fiasco. It happens to
// survive in ordinary builds but AddressSanitizer turns the uninitialized read into a crash.
static int& nprocs_storage()
{
    static int value = tbb::this_task_arena::max_concurrency();
    return value;
}

void set_nprocs(int value)
{
    if (value <= 0) {
        nprocs_storage() = tbb::this_task_arena::max_concurrency();
    } else {
        nprocs_storage() = value;
    }
}

int get_nprocs()
{
    return nprocs_storage();
}

} // namespace instant_meshes
