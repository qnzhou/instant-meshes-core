#
# Copyright 2026 Adobe. All rights reserved.
# This file is licensed to you under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License. You may obtain a copy
# of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
# OF ANY KIND, either express or implied. See the License for the specific language
# governing permissions and limitations under the License.
#

add_executable(instant_meshes_core_tests
    test_instant_meshes_core.cpp
    test_optimizer.cpp)
target_link_libraries(instant_meshes_core_tests PRIVATE
    instant-meshes-core
    Catch2::Catch2WithMain)
set_target_properties(instant_meshes_core_tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES)

# Register each TEST_CASE with CTest via Catch2's discovery helper (CMAKE_MODULE_PATH was
# extended with Catch2's `extras` directory in cmake/catch2.cmake).
include(Catch)
catch_discover_tests(instant_meshes_core_tests)
