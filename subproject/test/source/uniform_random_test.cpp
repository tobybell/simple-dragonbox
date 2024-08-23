// Copyright 2020 Junekey Jeon
//
// The contents of this file may be used under the terms of
// the Apache License v2.0 with LLVM Exceptions.
//
//    (See accompanying file LICENSE-Apache or copy at
//     https://llvm.org/foundation/relicensing/LICENSE.txt)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

#include "dragonbox/dragonbox.h"
#include "random_float.h"
#include "ryu/ryu.h"

#include <iostream>
#include <string_view>
#include <utility>

static void reference_implementation(float x, char* buffer) { f2s_buffered(x, buffer); }
static void reference_implementation(double x, char* buffer) { d2s_buffered(x, buffer); }

template <class Float, class TypenameString, class... Args>
static bool uniform_random_test(std::size_t number_of_tests, TypenameString&& type_name_string,
                                Args&&... args) {
    char buffer1[64];
    char buffer2[64];
    auto rg = generate_correctly_seeded_mt19937_64();
    bool success = true;
    for (std::size_t test_idx = 0; test_idx < number_of_tests; ++test_idx) {
        auto x = uniformly_randomly_generate_general_float<Float>(rg);

        // Check if the output is identical to the reference implementation (Ryu).
        jkj::dragonbox::to_chars(x, buffer1, std::forward<Args>(args)...);
        reference_implementation(x, buffer2);

        std::string_view view1(buffer1);
        std::string_view view2(buffer2);

        if (view1 != view2) {
            std::cout << "Error detected! [Reference = " << buffer2 << ", Dragonbox = " << buffer1
                      << "]\n";
            success = false;
        }
    }

    if (success) {
        std::cout << "Uniform random test for " << type_name_string << " with " << number_of_tests
                  << " examples succeeded.\n";
    }

    return success;
}

int main() {
    constexpr bool run_float = true;
    constexpr std::size_t number_of_uniform_random_tests_float = 10000000;

    constexpr bool run_float_with_compressed_cache = true;
    constexpr std::size_t number_of_uniform_random_tests_float_compressed = 10000000;

    constexpr bool run_double = true;
    constexpr std::size_t number_of_uniform_random_tests_double = 10000000;

    constexpr bool run_double_with_compressed_cache = true;
    constexpr std::size_t number_of_uniform_random_tests_double_compressed = 10000000;

    bool success = true;

    if (run_float) {
        std::cout << "[Testing uniformly randomly generated float inputs...]\n";
        success &= uniform_random_test<float>(number_of_uniform_random_tests_float, "float");
        std::cout << "Done.\n\n\n";
    }
    if (run_float_with_compressed_cache) {
        std::cout << "[Testing uniformly randomly generated float inputs with compressed cache...]\n";
        success &= uniform_random_test<float>(number_of_uniform_random_tests_float_compressed, "float",
                                              jkj::dragonbox::policy::cache::compact);
        std::cout << "Done.\n\n\n";
    }
    if (run_double) {
        std::cout << "[Testing uniformly randomly generated double inputs...]\n";
        success &= uniform_random_test<double>(number_of_uniform_random_tests_double, "double");
        std::cout << "Done.\n\n\n";
    }
    if (run_double_with_compressed_cache) {
        std::cout << "[Testing uniformly randomly generated double inputs with compressed cache...]\n";
        success &= uniform_random_test<double>(number_of_uniform_random_tests_double_compressed,
                                               "double", jkj::dragonbox::policy::cache::compact);
        std::cout << "Done.\n\n\n";
    }

    if (!success) {
        return -1;
    }
}
