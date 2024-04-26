#include <chrono>
#include <iostream>
#include <sstream>

#include <doarr/tuning/tuning.hpp>
#include <doarr/tuning/opentuner.hpp>

#include <doarr/import.hpp>
#include <doarr/expr.hpp>

using num_t = float;

extern doarr::imported run_matmul;

#define PARAMETER(...) doarr::tuning::make_parameter([&]{return __VA_ARGS__;})

int main(int argc, char **argv) {
	if(argc != 3) {
		std::cerr << "Usage: PROGRAM FILE SIZE" << std::endl;
		std::exit(1);
	}

	std::size_t size;
	std::istringstream(argv[2]) >> size;

	// define tuning parameters

	constexpr auto &dn = doarr::noarr;

	auto i_st = dn.vector['i']();
	auto j_st = dn.vector['j']();
	auto k_st = dn.vector['k']();

	auto tuner_args = {"--test-limit=20", "--no-dups"};
	auto tuner = doarr::tuning::make_tuner<doarr::tuning::opentuner_backend>(tuner_args);

	auto block_size = tuner.add_parameter("block_size", noarr::tuning::mapped_range, [](auto i) { return 1 << i; }, 1, 6);

	auto a_order = tuner.add_parameter("a_order", noarr::tuning::choice, i_st ^ k_st, k_st ^ i_st);
	auto b_order = tuner.add_parameter("b_order", noarr::tuning::choice, k_st ^ j_st, j_st ^ k_st);
	auto c_order = tuner.add_parameter("c_order", noarr::tuning::choice, i_st ^ j_st, j_st ^ i_st);

	auto block_i = tuner.add_parameter("block_i", noarr::tuning::choice,
		PARAMETER(dn.into_blocks[{'i', 'I', 'i'}](*block_size)),
		dn.bcast['I'](dn.lit[1]));

	auto block_j = tuner.add_parameter("block_j", noarr::tuning::choice,
		PARAMETER(dn.into_blocks[{'j', 'J', 'j'}](*block_size)),
		dn.bcast['J'](dn.lit[1]));

	auto block_k = tuner.add_parameter("block_k", noarr::tuning::choice,
		PARAMETER(dn.into_blocks[{'k', 'K', 'k'}](*block_size)),
		dn.bcast['K'](dn.lit[1]));

	auto block_order = tuner.add_parameter("block_order", noarr::tuning::mapped_permutation, [](auto &&... pars) requires (sizeof...(pars) == 5) { return (... ^ pars); },
		dn.hoist['I'](),
		dn.hoist['J'](),
		dn.hoist['K'](),
		dn.hoist['i'](),
		dn.hoist['j']());

	// initialize

	std::size_t a_sz = size * size * sizeof(num_t);
	std::size_t b_sz = size * size * sizeof(num_t);
	std::size_t c_sz = size * size * sizeof(num_t);

	num_t *data;

	if (!(data = (num_t *)malloc(a_sz + b_sz + c_sz))) {
		std::cerr << __FILE__ ":" << __LINE__ << ": error: failed to allocate memory" << std::endl;
		exit(1);
	}

	std::FILE *file = std::fopen(argv[1], "r");
	if(std::fread(data, 1, a_sz + b_sz, file) != a_sz + b_sz) {
		std::cerr << "Input error" << std::endl;
		std::abort();
	}
	std::fclose(file);

	doarr::ptr pa = data;
	doarr::ptr pb = data + a_sz / sizeof(num_t);
	doarr::ptr pc = data + (a_sz + b_sz) / sizeof(num_t);

	// tune

	tuner.tune([&]() -> std::optional<double> {
		auto ta = dn.scalar["num_t"]() ^ *a_order ^ dn.set_length[{'i', 'k'}](size, size);
		auto tb = dn.scalar["num_t"]() ^ *b_order ^ dn.set_length[{'k', 'j'}](size, size);
		auto tc = dn.scalar["num_t"]() ^ *c_order ^ dn.set_length[{'i', 'j'}](size, size);

		auto order = *block_i ^ *block_j ^ *block_k ^ *block_order;

		try {
			std::chrono::nanoseconds duration;
			run_matmul(ta, tb, tc, pa, pb, pc, order, doarr::ptr(&duration));
			run_matmul(ta, tb, tc, pa, pb, pc, order, doarr::ptr(&duration));
			run_matmul(ta, tb, tc, pa, pb, pc, order, doarr::ptr(&duration));
			run_matmul(ta, tb, tc, pa, pb, pc, order, doarr::ptr(&duration));
			return duration.count();
		} catch(const doarr::compilation_error &) {
			return std::nullopt;
		}
	});

	//std::fwrite(data + (a_sz + b_sz) / sizeof(num_t), 1, c_sz, stdout);

	free(data);

	return 0;
}
