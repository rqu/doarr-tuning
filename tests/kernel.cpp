#include <chrono>
#include <iostream>
#include <sstream>
#include <utility>
#include <type_traits>

#include <noarr/structures_extended.hpp>
#include <noarr/structures/extra/traverser.hpp>
#include <noarr/structures/interop/bag.hpp>

#include <doarr/export.hpp>

namespace {

using num_t = float;

constexpr auto reset(auto C) {
	return [=](auto state) {
		C[state] = 0;
	};
}

constexpr auto matmul(auto A, auto B, auto C) {
	return [=](auto trav) {
		num_t result = C[trav.state()];

		trav.for_each([=, &result](auto state) {
			result += A[state] * B[state];
		});

		C[trav.state()] = result;
	};
}

doarr::exported run_matmul(auto ta, auto tb, auto tc, void *pa, void *pb, void *pc, auto order, void *duration) {
	auto start = std::chrono::high_resolution_clock::now();

	auto A = noarr::make_bag(ta, pa);
	auto B = noarr::make_bag(tb, pb);
	auto C = noarr::make_bag(tc, pc);

	noarr::traverser(C).for_each(reset(C));

	auto trav = noarr::traverser(A, B, C).order(order);

	trav.template for_sections<'I', 'J', 'K', 'i', 'j'>(matmul(A, B, C));

	auto end = std::chrono::high_resolution_clock::now();

	*(std::chrono::nanoseconds *)duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

}
