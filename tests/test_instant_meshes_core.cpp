/*
    test_instant_meshes_core.cpp -- Catch2 unit tests for InstantMeshesCore.

    Copyright 2026 Adobe. All rights reserved.
    This file is licensed to you under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License. You may obtain a copy
    of the License at http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software distributed under
    the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
    OF ANY KIND, either express or implied. See the License for the specific language
    governing permissions and limitations under the License.
*/

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <aabb.h>
#include <meshstats.h>

#include <cmath>

using namespace instant_meshes;
using Catch::Approx;

namespace {

// Two triangles forming a unit square in the z = 0 plane:
//
//   D(0,1)---C(1,1)
//     |    /  |
//     |  /    |
//   A(0,0)---B(1,0)
//
// Faces: (A, B, C) and (A, C, D); the shared edge is the diagonal A--C.
void make_unit_square(MatrixXu& F, MatrixXf& V)
{
    V.resize(3, 4);
    V.col(0) = Vector3f(0.f, 0.f, 0.f); // A
    V.col(1) = Vector3f(1.f, 0.f, 0.f); // B
    V.col(2) = Vector3f(1.f, 1.f, 0.f); // C
    V.col(3) = Vector3f(0.f, 1.f, 0.f); // D

    F.resize(3, 2);
    F.col(0) = Vector3u(0, 1, 2);
    F.col(1) = Vector3u(0, 2, 3);
}

} // namespace

TEST_CASE("AABB accumulates bounds and answers geometry queries", "[aabb]")
{
    AABB box;
    // A freshly-cleared box is empty (inverted bounds).
    REQUIRE(box.min.x() > box.max.x());

    box.expandBy(Vector3f(0.f, 0.f, 0.f));
    box.expandBy(Vector3f(1.f, 2.f, 3.f));

    REQUIRE(box.min == Vector3f(0.f, 0.f, 0.f));
    REQUIRE(box.max == Vector3f(1.f, 2.f, 3.f));

    // Surface area of a 1x2x3 box: 2 * (1*2 + 1*3 + 2*3) = 22.
    REQUIRE(box.surfaceArea() == Approx(22.f));
    REQUIRE(box.center() == Vector3f(0.5f, 1.f, 1.5f));
    REQUIRE(box.largestAxis() == 2);

    REQUIRE(box.contains(Vector3f(0.5f, 1.f, 1.5f)));
    REQUIRE_FALSE(box.contains(Vector3f(-0.1f, 0.f, 0.f)));
}

TEST_CASE("AABB ray intersection", "[aabb]")
{
    AABB box(Vector3f(0.f, 0.f, 0.f), Vector3f(1.f, 1.f, 1.f));

    // Ray passing straight through the box along +x.
    REQUIRE(box.rayIntersect(Ray(Vector3f(-1.f, 0.5f, 0.5f), Vector3f(1.f, 0.f, 0.f))));
    // Parallel ray that misses in y/z.
    REQUIRE_FALSE(box.rayIntersect(Ray(Vector3f(-1.f, 2.f, 2.f), Vector3f(1.f, 0.f, 0.f))));
}

TEST_CASE("AABB merge produces the union bounds", "[aabb]")
{
    AABB a(Vector3f(0.f, 0.f, 0.f), Vector3f(1.f, 1.f, 1.f));
    AABB b(Vector3f(-1.f, 0.5f, 2.f), Vector3f(0.5f, 3.f, 4.f));

    AABB m = AABB::merge(a, b);
    REQUIRE(m.min == Vector3f(-1.f, 0.f, 0.f));
    REQUIRE(m.max == Vector3f(1.f, 3.f, 4.f));
}

TEST_CASE("compute_mesh_stats on a unit square", "[meshstats]")
{
    MatrixXu F;
    MatrixXf V;
    make_unit_square(F, V);

    // Serial and deterministic-parallel reductions must agree on this tiny mesh.
    for (bool deterministic : {false, true}) {
        INFO("deterministic = " << deterministic);
        MeshStats stats = compute_mesh_stats(F, V, deterministic);

        // Two triangles of area 0.5 each.
        REQUIRE(stats.mSurfaceArea == Approx(1.0));

        // Bounding box is the unit square in the z = 0 plane.
        REQUIRE(stats.mAABB.min == Vector3f(0.f, 0.f, 0.f));
        REQUIRE(stats.mAABB.max == Vector3f(1.f, 1.f, 0.f));

        // Longest edge is the shared diagonal A--C of length sqrt(2).
        REQUIRE(stats.mMaximumEdgeLength == Approx(std::sqrt(2.0)));

        // Average over all 6 face-corner edges: (4 + 2*sqrt(2)) / 6.
        REQUIRE(stats.mAverageEdgeLength == Approx((4.0 + 2.0 * std::sqrt(2.0)) / 6.0));

        // Area-weighted centroid of the square.
        REQUIRE(stats.mWeightedCenter.x() == Approx(0.5f));
        REQUIRE(stats.mWeightedCenter.y() == Approx(0.5f));
        REQUIRE(stats.mWeightedCenter.z() == Approx(0.f));
    }
}
