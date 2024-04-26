#ifndef DOARR_TUNING_NOARR_TUNING_HPP_
#define DOARR_TUNING_NOARR_TUNING_HPP_

#include <cstddef>

// Interface copied from noarr-tuning.

namespace noarr::tuning {

// low-level parameter types used with formatters

// https://github.com/jiriklepl/noarr-tuning/blob/68caf6019afcdf7c490023a7a556e5e854a39819/include/noarr/tuning/formatter.hpp

struct category_parameter {
	constexpr category_parameter(std::size_t num) noexcept : num_(num) {}

	std::size_t num_;
};

template<class Start, class End, class Step>
struct range_parameter {
	constexpr range_parameter(End max) noexcept : min_(0), max_(max), step_((Step)1) {}
	constexpr range_parameter(Start min, End max, Step step = (Step)1) noexcept : min_(min), max_(max), step_(step) {}

	Start min_;
	End max_;
	Step step_;
};

struct permutation_parameter {
	constexpr permutation_parameter(std::size_t num) noexcept : num_(num) {}

	std::size_t num_;
};

// high-level parameter type tags used in tuning structures

// https://github.com/jiriklepl/noarr-tuning/blob/68caf6019afcdf7c490023a7a556e5e854a39819/include/noarr/tuning/tuning.hpp

template<class ...>
struct name_holder {};

struct begin_t {};
constexpr begin_t begin;

struct end_t {};
constexpr end_t end;

struct choice_t {};
constexpr choice_t choice;

struct constant_t {};
constexpr constant_t constant;

struct permutation_t {};
constexpr permutation_t permutation;

struct mapped_permutation_t {};
constexpr mapped_permutation_t mapped_permutation;

struct range_t {};
constexpr range_t range;

struct mapped_range_t {};
constexpr mapped_range_t mapped_range;

}

namespace doarr::tuning {

using namespace noarr::tuning;

}

#endif
