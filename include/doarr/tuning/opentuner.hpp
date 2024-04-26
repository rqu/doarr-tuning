#ifndef DOARR_TUNING_OPENTUNER_HPP_
#define DOARR_TUNING_OPENTUNER_HPP_

#include <vector>
#include <iostream>
#include <string_view>
#include <ranges>

#include <snaketongs.hpp>

#include "common.hpp"

namespace doarr::tuning {

namespace py = snaketongs;

class opentuner_backend final : public backend {
	py::process proc;

	const py::object ConfigurationManipulator = proc["opentuner.ConfigurationManipulator"];
	const py::object MeasurementInterface     = proc["opentuner.MeasurementInterface"];
	const py::object SwitchParameter          = proc["opentuner.SwitchParameter"];
	const py::object PermutationParameter     = proc["opentuner.PermutationParameter"];
	const py::object IntegerParameter         = proc["opentuner.IntegerParameter"];
	const py::object Result                   = proc["opentuner.Result"];

	const py::object args = proc.list();

	std::vector<py::object> param_desc;

	py::object config {nullptr};

	void add_arg(std::string_view arg) {
		args.call("append", arg);
	}

public:
	opentuner_backend(std::ranges::range auto &&args) requires requires { add_arg(*args.begin()); } {
		for(auto &&arg : args)
			add_arg(arg);
	}

	parameter<std::size_t> add_parameter(std::string &&name, const category_parameter &par) override {
		param_desc.push_back(SwitchParameter(name, par.num_));

		return make_parameter([name = std::move(name), &config = config] {
			return (std::size_t) config[name];
		});
	}

	parameter<raw_perm_t> add_parameter(std::string &&name, const permutation_parameter &par) override {
		param_desc.push_back(PermutationParameter(name, proc.range(par.num_)));

		return make_parameter([name = std::move(name), &config = config, num = par.num_] {
			auto py_result = config[name];
			raw_perm_t result = std::make_unique_for_overwrite<std::size_t[]>(num);
			for(std::size_t i = 0; i < num; i++)
				result[i] = (std::size_t) py_result[i];
			return result;
		});
	}

	parameter<std::size_t> add_parameter(std::string &&name, const simple_range_parameter &par) override {
		param_desc.push_back(IntegerParameter(name, par.min_, par.max_));

		return make_parameter([name = std::move(name), &config = config] {
			return (std::size_t) config[name];
		});
	}

	void tune(std::function<std::optional<double>()> &&measure) override {
		using py::kw;

		auto ApplicationTuner = proc.type("ApplicationTuner", proc.make_tuple(MeasurementInterface), proc.dict(
			kw("save_final_config") = [](auto self, auto configuration) {
				self.call("manipulator").call("save_to_file", configuration.get("data"), "mmm_final_config.json");
			},

			kw("manipulator") = [this](auto) {
				auto manipulator = ConfigurationManipulator();
				for(const auto &param : param_desc)
					manipulator.call("add_parameter", param);
				return manipulator;
			},

			kw("run") = [this, measure = std::move(measure)](auto /*self*/, auto desired_result, auto /*input*/, auto /*limit*/) {
				// accessed by measure()!
				config = desired_result.get("configuration").get("data");

				std::optional<double> result = measure();

#if defined(NOARR_TUNING_VERBOSE) && NOARR_TUNING_VERBOSE >= 1
				if(result)
					std::cerr << "Error" << std::endl;
				else
					std::cerr << "Run time: " << *result << std::endl;
#endif

				if(result)
					return Result(kw("time")=*result);
				else
					return Result(kw("state")="ERROR", kw("time")=proc["math.inf"]);
			}
		));

		auto argparser = proc["opentuner.default_argparser"]();
		ApplicationTuner.call("main", argparser.call("parse_args", args));
	}
};

}

#endif
