#ifndef DOARR_TUNING_TUNING_HPP_
#define DOARR_TUNING_TUNING_HPP_

#include <array>

#include "common.hpp"

namespace doarr::tuning {

#define FWD(name) decltype(name)(name)

struct tuner {
private:
	template<typename T, std::size_t... I>
	static constexpr std::array<T, sizeof...(I)> map_collect_arr(std::index_sequence<I...>, auto f) {
		return {f.template operator()<I>()...};
	}

	template<std::size_t... I>
	static constexpr decltype(auto) map_collect_fn(std::index_sequence<I...>, auto fm, const auto &fc) {
		return fc(fm.template operator()<I>()...);
	}

	template<typename... Refs>
	struct storage_helper {
		using common_type = std::common_type_t<remove_cvrefpar_t<Refs>...>;
		using stored_type = common_type;
		using array_type = std::array<stored_type, sizeof...(Refs)>;
		using load_type = const stored_type &;

		static constexpr array_type store(Refs... refs) {
			return array_type{(Refs)refs...};
		}

		static constexpr load_type load(const stored_type &ref) {
			return ref;
		}
	};
	template<typename... Refs> requires (... || Parameter<Refs>)
	struct storage_helper<Refs...> {
		using common_type = std::common_type_t<remove_cvrefpar_t<Refs>...>;
		using stored_type = parameter<common_type>;
		using array_type = std::array<stored_type, sizeof...(Refs)>;
		using load_type = common_type;

		template<typename Ref>
		static stored_type store_one(Ref ref) {
			return stored_type::make_const(FWD(ref));
		}

		template<typename T>
		static stored_type store_one(parameter<T> &&tp) {
			return tp.map([](T &&t) -> common_type {
				return FWD(t);
			});
		}

		static constexpr stored_type &&store_one(stored_type &&tp) {
			return FWD(tp);
		}

		static constexpr array_type store(Refs... refs) {
			return array_type{store_one((Refs)refs)...};
		}

		static load_type load(const stored_type &ref) {
			return *ref;
		}
	};

	template<typename... Refs>
	using comm = typename storage_helper<Refs...>::load_type;

	template<typename T, typename>
	using const_t = T;

	template<typename F, typename A, typename... Count>
	using apply_map_const = decltype(std::declval<F>()(std::declval<const_t<A, Count>>()...));

	std::size_t idx = 1;

public:
	std::unique_ptr<backend> impl;

	tuner(std::unique_ptr<backend> &&impl) : impl(FWD(impl)) {}

	auto add_parameter(std::string &&name, choice_t, auto &&... choices) -> parameter<comm<decltype(choices)...>> {
		using storage = storage_helper<decltype(choices)...>;

		return impl->add_parameter(FWD(name), category_parameter(sizeof...(choices)))
			.map([choices = storage::store(FWD(choices)...)](std::size_t chosen_index) -> typename storage::load_type {
				return storage::load(choices[chosen_index]);
			});
	}

	auto add_parameter(std::string &&name, permutation_t, auto &&... choices) -> parameter<std::array<comm<decltype(choices)...>, sizeof...(choices)>> {
		using storage = storage_helper<decltype(choices)...>;
		using index_sequence = std::make_index_sequence<sizeof...(choices)>;

		return impl->add_parameter(FWD(name), permutation_parameter(sizeof...(choices)))
			.map([choices = storage::store(FWD(choices)...)](raw_perm_t chosen_permutation) {
				return map_collect_arr<typename storage::load_type>(index_sequence(), [&choices, &chosen_permutation]<std::size_t I>() -> typename storage::load_type {
					return storage::load(choices[chosen_permutation[I]]);
				});
			});
	}

	auto add_parameter(std::string &&name, mapped_permutation_t, auto &&f, auto &&... choices) -> parameter<apply_map_const<decltype(std::as_const(f)), comm<decltype(choices)...>, decltype(choices)...>> {
		using storage = storage_helper<decltype(choices)...>;
		using index_sequence = std::make_index_sequence<sizeof...(choices)>;

		return impl->add_parameter(FWD(name), permutation_parameter(sizeof...(choices)))
			.map([choices = storage::store(FWD(choices)...), f = FWD(f)](raw_perm_t chosen_permutation) {
				return map_collect_fn(index_sequence(), [&choices, &chosen_permutation]<std::size_t I>() -> typename storage::load_type {
					return storage::load(choices[chosen_permutation[I]]);
				}, f);
			});
	}

	auto add_parameter(std::string &&name, range_t, auto &&begin, auto &&end, auto &&step) -> parameter<decltype(std::as_const(begin) + std::size_t() * std::as_const(step))> {
		std::size_t range_size = FWD(end) - begin;

		return impl->add_parameter(FWD(name), range_parameter(zero_t(), range_size, one_t()))
			.map([begin = FWD(begin), step = FWD(step)](std::size_t chosen_index) -> decltype(auto) {
				return begin + std::move(chosen_index) * step;
			});
	}

	auto add_parameter(std::string &&name, range_t, auto &&begin, auto &&end) -> parameter<decltype(std::as_const(begin) + std::size_t())> {
		return add_parameter(FWD(name), range, FWD(begin), FWD(end), one_t());
	}

	auto add_parameter(std::string &&name, mapped_range_t, auto &&f, auto &&begin, auto &&end, auto &&step) -> parameter<decltype(std::as_const(f)(std::as_const(begin) + std::size_t() * std::as_const(step)))> {
		std::size_t range_size = FWD(end) - begin;

		return impl->add_parameter(FWD(name), range_parameter(zero_t(), range_size, one_t()))
			.map([f = FWD(f), begin = FWD(begin), step = FWD(step)](std::size_t chosen_index) -> decltype(auto) {
				return f(begin + std::move(chosen_index) * step);
			});
	}

	auto add_parameter(std::string &&name, mapped_range_t, auto &&f, auto &&begin, auto &&end) -> parameter<decltype(std::as_const(f)(std::as_const(begin) + std::size_t()))> {
		return add_parameter(FWD(name), mapped_range, FWD(f), FWD(begin), FWD(end), one_t());
	}

	auto add_parameter(auto &&... args) -> decltype(add_parameter("", FWD(args)...)) {
		return add_parameter(std::to_string(idx), FWD(args)...);
	}

	void tune(std::function<std::optional<double>()> &&f) {
		impl->tune(FWD(f));
	}
};

template<std::derived_from<backend> Backend>
constexpr tuner make_tuner(auto &&... args) {
	return tuner(std::make_unique<Backend>(FWD(args)...));
}

#undef FWD

}

#endif
