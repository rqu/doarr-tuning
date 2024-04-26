#ifndef DOARR_TUNING_COMMON_HPP_
#define DOARR_TUNING_COMMON_HPP_

#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "noarr_tuning.hpp"

namespace doarr::tuning {

#define FWD(NAME) decltype(NAME)(NAME)

/////////////////////////////
//                         //
//  generic c++ utilities  //
//                         //
/////////////////////////////

using zero_t = std::integral_constant<std::size_t, 0>;
using one_t = std::integral_constant<std::size_t, 1>;

template<typename T>
concept Forwardable = requires(T &&t, void f(T)) {
	f(FWD(t));
};

////////////////////////////////////////////////
//                                            //
//  `struct parameter` and related utilities  //
//                                            //
////////////////////////////////////////////////

// specialized below for parameter<...>
template<typename T>
struct param_traits {
	// using type = N/A;
	using remove_par = T;
};

// for (possibly cvref) parameter<T>, the T; otherwise, substitution failure
template<typename T>
using param_type_t = typename param_traits<std::remove_cvref_t<T>>::type;

template<typename T>
using remove_cvrefpar_t = typename param_traits<std::remove_cvref_t<T>>::remove_par;

template<typename T>
concept Parameter = requires { typename param_type_t<T>; };

// a monadic type representing the value of a tuning parameter;
// during tuning, the wrapped value can be obtained using dereference operator or called using call operator
template<typename T> requires (!Parameter<T>)
struct parameter {
	std::function<T()> get_value;

	auto operator()(auto &&... args) const -> decltype(std::declval<T>()(FWD(args)...)) const {
		return get_value()(FWD(args)...);
	}

	T operator *() const {
		return get_value();
	}

	auto map(Forwardable auto &&f) && -> parameter<decltype(std::as_const(f)(std::declval<T>()))> {
		using U = decltype(std::as_const(f)(std::declval<T>()));

		return parameter<U>{[tp = std::move(*this), f = FWD(f)]() -> U {
			return f(*tp);
		}};
	}

	// actually bind
	auto map(Forwardable auto &&f) && -> parameter<param_type_t<decltype(std::as_const(f)(std::declval<T>()))>> {
		using U = param_type_t<decltype(std::as_const(f)(std::declval<T>()))>;

		return parameter<U>{[tp = std::move(*this), f = FWD(f)]() -> U {
			return *f(*tp);
		}};
	}

	static parameter<T> make_const(auto &&... args) requires Forwardable<const T &> && requires { std::remove_cvref_t<T>(FWD(args)...); } {
		return parameter<T>{[t = std::remove_cvref_t<T>(FWD(args)...)]() -> T {
			return t;
		}};
	}
};

template<typename T>
struct param_traits<parameter<T>> {
	using type = T;
	using remove_par = T;
};

template<typename F>
constexpr parameter<decltype(std::declval<F &>()())> make_parameter(F &&f) {
	return {FWD(f)};
}

///////////////////////////////////////
//                                   //
//  common tuning backend interface  //
//                                   //
///////////////////////////////////////

using simple_range_parameter = range_parameter<zero_t, std::size_t, one_t>;

// represents an unmapped permutation
using raw_perm_t = std::unique_ptr<std::size_t[]>;

struct backend {
	virtual parameter<std::size_t> add_parameter(std::string &&name, const category_parameter &) = 0;
	virtual parameter<raw_perm_t> add_parameter(std::string &&name, const permutation_parameter &) = 0;
	virtual parameter<std::size_t> add_parameter(std::string &&name, const simple_range_parameter &) = 0;

	virtual void tune(std::function<std::optional<double>()> &&) = 0;

	virtual ~backend() = default;
};

#undef FWD

}

#endif
