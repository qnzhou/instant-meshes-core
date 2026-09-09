/*
    test_optimizer.cpp -- Threaded smoke test for the field Optimizer.

    This exercises the orientation/position optimization pipeline, which spawns a worker
    thread. It is primarily intended to be run under ThreadSanitizer: the Optimizer's
    optimizeOrientations()/optimizePositions() write the mOptimize* predicate flags without
    holding mRes.mutex(), while the worker reads them under that mutex, so TSan should flag a
    data race here.

    Copyright 2026 Adobe. All rights reserved.
    This file is licensed to you under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License. You may obtain a copy
    of the License at http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software distributed under
    the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
    OF ANY KIND, either express or implied. See the License for the specific language
    governing permissions and limitations under the License.
*/

#include <catch2/catch_test_macros.hpp>

#include <adjacency.h>
#include <common.h>
#include <dedge.h>
#include <field.h>
#include <hierarchy.h>
#include <meshstats.h>
#include <normal.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

using namespace instant_meshes;

namespace {

// Build a flat n-by-n triangulated grid (consistent CCW winding, viewed from +z).
void make_grid(int n, MatrixXu& F, MatrixXf& V)
{
    const int nv = n * n;
    V.resize(3, nv);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            V.col(i * n + j) = Vector3f((Float)i, (Float)j, 0.f);

    const int ncells = (n - 1) * (n - 1);
    F.resize(3, 2 * ncells);
    auto idx = [n](int i, int j) { return (uint32_t)(i * n + j); };
    int f = 0;
    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - 1; ++j) {
            uint32_t v00 = idx(i, j), v10 = idx(i + 1, j);
            uint32_t v11 = idx(i + 1, j + 1), v01 = idx(i, j + 1);
            F.col(f++) = Vector3u(v00, v10, v11);
            F.col(f++) = Vector3u(v00, v11, v01);
        }
    }
}

} // namespace

TEST_CASE("Optimizer orientation and position smoke test", "[optimizer][threads]")
{
    MatrixXu F;
    MatrixXf V;
    make_grid(24, F, V);

    MeshStats stats = compute_mesh_stats(F, V, /*deterministic=*/true);

    VectorXu V2E, E2E;
    VectorXb boundary, nonManifold;
    build_dedge(F, V, V2E, E2E, boundary, nonManifold);

    AdjacencyMatrix adj = generate_adjacency_matrix_uniform(F, V2E, E2E, nonManifold);

    MatrixXf N;
    generate_smooth_normals(F, V, V2E, E2E, nonManifold, N);

    VectorXf A;
    compute_dual_vertex_areas(F, V, V2E, E2E, nonManifold, A);

    const Float scale = (Float)(stats.mAverageEdgeLength * 2.0);

    MultiResolutionHierarchy mRes;
    mRes.setE2E(std::move(E2E));
    mRes.setAdj(std::move(adj));
    mRes.setF(std::move(F));
    mRes.setV(std::move(V));
    mRes.setA(std::move(A));
    mRes.setN(std::move(N));
    mRes.setScale(scale);
    mRes.build(/*deterministic=*/true);
    mRes.resetSolution();

    // The worker only runs when the hierarchy has at least one level; otherwise
    // optimize*()/wait() would deadlock. Bail out cleanly before touching the Optimizer.
    REQUIRE(mRes.levels() >= 1);

    // Watchdog: a lost wakeup in the Optimizer could deadlock wait(). Guarantee termination
    // so a hang surfaces as a failing exit code instead of blocking the whole run.
    std::atomic<bool> done{false};
    std::thread watchdog([&done] {
        for (int i = 0; i < 600 && !done.load(std::memory_order_relaxed); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!done.load(std::memory_order_relaxed)) {
            std::fprintf(stderr, "Optimizer test watchdog: deadlock detected, aborting.\n");
            std::_Exit(42);
        }
    });

    Optimizer optimizer(mRes, /*interactive=*/false);
    optimizer.setRoSy(4);
    optimizer.setPoSy(4);
    optimizer.setExtrinsic(true);

    // Give the worker thread time to reach mCond.wait() and park. This forces the
    // interleaving in which the worker is already waiting when optimizeOrientations() writes
    // the predicate flags without holding mRes.mutex(): the worker's wakeup re-acquires the
    // mutex with no happens-before edge to that write, so the unsynchronized flag access is a
    // genuine data race that ThreadSanitizer can observe. Without this delay the main thread
    // often grabs the mutex first (in wait()), accidentally ordering the write and hiding it.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    optimizer.optimizeOrientations(-1);
    optimizer.notify();
    optimizer.wait();

    optimizer.optimizePositions(-1);
    optimizer.notify();
    optimizer.wait();

    optimizer.shutdown();

    done.store(true, std::memory_order_relaxed);
    watchdog.join();

    mRes.free();
    SUCCEED("optimizer completed without deadlock");
}
