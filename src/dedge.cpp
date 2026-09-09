/*
    dedge.cpp: Parallel directed edge data structure builder

    This file is part of the implementation of

        Instant Field-Aligned Meshes
        Wenzel Jakob, Daniele Panozzo, Marco Tarini, and Olga Sorkine-Hornung
        In ACM Transactions on Graphics (Proc. SIGGRAPH Asia 2015)

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include "dedge.h"

#include <atomic>

namespace instant_meshes {

void build_dedge(
    const MatrixXu& F,
    const MatrixXf& V,
    VectorXu& V2E,
    VectorXu& E2E,
    VectorXb& boundary,
    VectorXb& nonManifold,
    const ProgressCallback& progress,
    bool quiet)
{
    if (!quiet) {
        cout << "Building a directed edge data structure .. ";
        cout.flush();
    }
    Timer<> timer;

    if (progress && !quiet) progress("Building directed edge data structure", 0.0f);

    V2E.resize(V.cols());
    V2E.setConstant(INVALID);

    uint32_t deg = F.rows();
    std::vector<std::pair<uint32_t, uint32_t>> tmp(F.size());

    tbb::parallel_for(
        tbb::blocked_range<uint32_t>(0u, (uint32_t)F.cols(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t>& range) {
            for (uint32_t f = range.begin(); f != range.end(); ++f) {
                for (uint32_t i = 0; i < deg; ++i) {
                    uint32_t idx_cur = F(i, f), idx_next = F((i + 1) % deg, f),
                             edge_id = deg * f + i;
                    if (idx_cur >= V.cols() || idx_next >= V.cols())
                        throw std::runtime_error(
                            "Mesh data contains an out-of-bounds vertex reference!");
                    if (idx_cur == idx_next) continue;

                    // V2E[idx_cur] and tmp[*].second are updated concurrently by other
                    // threads via atomic compare-exchange, so every access to them must go
                    // through std::atomic_ref -- mixing atomic and plain accesses on the same
                    // location is a data race (undefined behavior). A failed compare-exchange
                    // returns the current value, which also removes the previous plain reads.
                    tmp[edge_id].first = idx_next;
                    std::atomic_ref<uint32_t>(tmp[edge_id].second).store(INVALID);

                    std::atomic_ref<uint32_t> head(V2E[idx_cur]);
                    uint32_t expected = INVALID;
                    if (!head.compare_exchange_strong(expected, edge_id)) {
                        // Another edge already owns this vertex; append to the linked list.
                        uint32_t idx = expected;
                        for (;;) {
                            std::atomic_ref<uint32_t> next(tmp[idx].second);
                            uint32_t slot = INVALID;
                            if (next.compare_exchange_strong(slot, edge_id)) break;
                            idx = slot;
                        }
                    }
                }
            }
            if (!quiet)
                SHOW_PROGRESS_RANGE(range, F.cols(), "Building directed edge data structure (1/3)");
        });

    nonManifold.resize(V.cols());
    nonManifold.setConstant(false);

    E2E.resize(F.cols() * deg);
    E2E.setConstant(INVALID);

    tbb::parallel_for(
        tbb::blocked_range<uint32_t>(0u, (uint32_t)F.cols(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t>& range) {
            for (uint32_t f = range.begin(); f != range.end(); ++f) {
                for (uint32_t i = 0; i < deg; ++i) {
                    uint32_t idx_cur = F(i, f), idx_next = F((i + 1) % deg, f),
                             edge_id_cur = deg * f + i;

                    if (idx_cur == idx_next) continue;

                    uint32_t it = V2E[idx_next], edge_id_opp = INVALID;
                    while (it != INVALID) {
                        if (tmp[it].first == idx_cur) {
                            if (edge_id_opp == INVALID) {
                                edge_id_opp = it;
                            } else {
                                nonManifold[idx_cur] = true;
                                nonManifold[idx_next] = true;
                                edge_id_opp = INVALID;
                                break;
                            }
                        }
                        it = tmp[it].second;
                    }

                    if (edge_id_opp != INVALID && edge_id_cur < edge_id_opp) {
                        E2E[edge_id_cur] = edge_id_opp;
                        E2E[edge_id_opp] = edge_id_cur;
                    }
                }
            }
            if (!quiet)
                SHOW_PROGRESS_RANGE(range, F.cols(), "Building directed edge data structure (2/3)");
        });

    std::atomic<uint32_t> nonManifoldCounter(0), boundaryCounter(0), isolatedCounter(0);

    boundary.resize(V.cols());
    boundary.setConstant(false);

    /* Detect boundary regions of the mesh and adjust vertex->edge pointers*/
    tbb::parallel_for(
        tbb::blocked_range<uint32_t>(0u, (uint32_t)V.cols(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t>& range) {
            for (uint32_t i = range.begin(); i != range.end(); ++i) {
                uint32_t edge = V2E[i];
                if (edge == INVALID) {
                    isolatedCounter++;
                    continue;
                }
                if (nonManifold[i]) {
                    nonManifoldCounter++;
                    V2E[i] = INVALID;
                    continue;
                }

                /* Walk backwards to the first boundary edge (if any) */
                uint32_t start = edge, v2e = INVALID;
                do {
                    v2e = std::min(v2e, edge);
                    uint32_t prevEdge = E2E[dedge_prev(edge, deg)];
                    if (prevEdge == INVALID) {
                        /* Reached boundary -- update the vertex->edge link */
                        v2e = edge;
                        boundary[i] = true;
                        boundaryCounter++;
                        break;
                    }
                    edge = prevEdge;
                } while (edge != start);
                V2E[i] = v2e;
            }
            if (!quiet)
                SHOW_PROGRESS_RANGE(range, V.cols(), "Building directed edge data structure (3/3)");
        });

    if (!quiet) {
        cout << "done. (";
        if (nonManifoldCounter) cout << nonManifoldCounter << " non-manifold vertices, ";
        if (boundaryCounter) cout << boundaryCounter << " boundary vertices, ";
        if (isolatedCounter) cout << isolatedCounter << " isolated vertices, ";
        cout << "took " << timeString(timer.value()) << ")" << endl;
    }
}
} // namespace instant_meshes
