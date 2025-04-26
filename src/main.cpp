// https://github.com/microsoft/vscode-cpptools/issues/9692
#if __INTELLISENSE__
#undef __ARM_NEON
#undef __ARM_NEON__
#endif

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jmespath/jmespath.hpp>
#include <jsoncons_ext/msgpack/msgpack.hpp>

#include <memory>
#include <deque>

using jsoncons::json;
namespace jmespath = jsoncons::jmespath;
namespace msgpack = jsoncons::msgpack;

int add(int i, int j) {
    return i + j;
}

namespace py = pybind11;
using rvp = py::return_value_policy;
using namespace pybind11::literals;

// https://github.com/danielaparker/jsoncons/blob/master/doc/ref/jmespath/jmespath.md


struct JsonQueryRepl {
    JsonQueryRepl(const std::string &jsontext, bool debug = false): doc_(json::parse(jsontext)), debug(debug) { }
    std::string eval(const std::string &expr_text) const {
        // auto result = jmespath::search(doc_, expr);
        auto expr = jmespath::make_expression<json>(expr_text);
        auto result = expr.evaluate(doc_, params_);
        if (debug) {
            std::cerr << pretty_print(result) << std::endl;
        }
        return result.to_string();
    }
    void add_params(const std::string &key, const std::string &value) {
        params_[key] = json::parse(value);
    }

    bool debug = false;
    private:
    json doc_;
    std::map<std::string, json> params_;
};

struct JsonQuery {
    JsonQuery() {}
    void setup_predicate(const std::string &predicate) {
        predicate_expr_ = std::make_unique<jmespath::jmespath_expression<json>>(jmespath::make_expression<json>(predicate));
        predicate_ = predicate;
    }
    void setup_transforms(const std::vector<std::string> &transforms) {
        transforms_expr_.clear();
        transforms_expr_.reserve(transforms.size());
        for (auto &t: transforms) {
            transforms_expr_.push_back(std::make_unique<jmespath::jmespath_expression<json>>(jmespath::make_expression<json>(t)));
        }
        transforms_ = transforms;
    }
    void add_params(const std::string &key, const std::string &value) {
        params_[key] = json::parse(value);
    }

    bool matches(const std::string &msg) const {
        if (!predicate_expr_) {
            return false;
        }
        auto doc = msgpack::decode_msgpack<json>(msg);
        return __matches(doc);
    }

    bool matches_json(const json &doc) const {
        if (!predicate_expr_) {
            return false;
        }
        return __matches(doc);
    }

    bool process(const std::string &msg, bool skip_predicate = false, bool raise_error = false) {
        auto doc = msgpack::decode_msgpack<json>(msg);
        return process_json(doc, skip_predicate, raise_error);
    }

    bool process_json(const json &doc, bool skip_predicate = false, bool raise_error = false) {
        if (!skip_predicate && !__matches(doc)) {
            return false;
        }
        std::vector<json> row;
        row.reserve(transforms_expr_.size());
        for (auto &expr: transforms_expr_) {
            try {
                row.push_back(expr->evaluate(doc, params_));
            } catch (const std::exception &e) {
                if (raise_error) {
                    throw e;
                }
                row.push_back(json::null());
            }
        }
        outputs_.emplace_back(std::move(row));
        return true;
    }

    json export_json() const {
        json result = json::make_array();
        result.reserve(outputs_.size());
        for (const auto& row : outputs_) {
            json json_row = json::make_array();
            json_row.reserve(row.size());
            for (const auto& cell : row) {
                json_row.push_back(cell);
            }
            result.push_back(json_row);
        }
        return result;
    }
    std::vector<uint8_t> export_() const {
        std::vector<uint8_t> output;
        msgpack::encode_msgpack(export_json(), output);
        return output;
    }

    void clear() {
        outputs_.clear();
    }

    bool debug = false;

private:
    std::string predicate_;
    std::unique_ptr<jmespath::jmespath_expression<json>> predicate_expr_;
    std::vector<std::string> transforms_;
    std::vector<std::unique_ptr<jmespath::jmespath_expression<json>>> transforms_expr_;
    std::map<std::string, json> params_;


    std::deque<std::vector<json>> outputs_;

    bool __matches(const json &msg) const {
        auto ret = predicate_expr_->evaluate(msg, params_);
        return /*ret.is_bool() && */ ret.as_bool();
    }
};

PYBIND11_MODULE(_core, m) {
    m.doc() = R"pbdoc(
    Python bindings for jsoncons library

    This module provides Python bindings for the jsoncons C++ library, allowing for
    efficient JSON processing, filtering, and transformation using JMESPath expressions.

    Classes:
        Json: A class for handling JSON data with conversion to/from JSON and MessagePack formats.
        JsonQueryRepl: A REPL (Read-Eval-Print Loop) for evaluating JMESPath expressions on JSON data.
        JsonQuery: A class for filtering and transforming JSON data using JMESPath expressions.

    Functions:
        msgpack_encode: Convert a JSON string to MessagePack binary format.
        msgpack_decode: Convert MessagePack binary data to a JSON string.
    )pbdoc";

    m.def("msgpack_encode", [](const std::string &input) {
        std::vector<uint8_t> output;
        msgpack::encode_msgpack(json::parse(input), output);
        return py::bytes(reinterpret_cast<const char *>(output.data()), output.size());
    }, "json_string"_a);
    m.def("msgpack_decode", [](const std::string &input) {
        auto doc = msgpack::decode_msgpack<json>(input);
        return doc.to_string();
    }, "msgpack_bytes"_a);

    py::class_<json>(m, "Json", py::module_local(), py::dynamic_attr()) //
    .def(py::init<>())
    // from/to_json
    .def("from_json", [](json &self, const std::string &input) -> json & {
        self = json::parse(input);
        return self;
    }, "json_string"_a, rvp::reference_internal)
    .def("to_json", [](const json &self) {
        return self.to_string();
    })
    // from/to_msgpack
    .def("from_msgpack", [](json &self, const std::string &input) -> json & {
        self = msgpack::decode_msgpack<json>(input);
        return self;
    }, "msgpack_bytes"_a, rvp::reference_internal)
    .def("to_msgpack", [](const json &self) {
        std::vector<uint8_t> output;
        msgpack::encode_msgpack(self, output);
        return py::bytes(reinterpret_cast<const char *>(output.data()), output.size());
    })
    //
    ;

    py::class_<JsonQueryRepl>(m, "JsonQueryRepl", py::module_local(), py::dynamic_attr()) //
        .def(py::init<const std::string &, bool>(), "json"_a, "debug"_a = false)
        .def("eval", &JsonQueryRepl::eval, "expr"_a)
        .def("add_params", &JsonQueryRepl::add_params, "key"_a, "value"_a)
        .def_readwrite("debug", &JsonQueryRepl::debug)
        //
        ;

    py::class_<JsonQuery>(m, "JsonQuery", py::module_local(), py::dynamic_attr()) //
        .def(py::init<>())
        .def("setup_predicate", &JsonQuery::setup_predicate)
        .def("setup_transforms", &JsonQuery::setup_transforms)
        .def("add_params", &JsonQuery::add_params, "key"_a, "value"_a)
        .def("matches", &JsonQuery::matches, "msgpack"_a)
        .def("matches_json", &JsonQuery::matches_json, "json"_a)
        .def("process", &JsonQuery::process, "msgpack"_a, py::kw_only(), "skip_predicate"_a = false, "raise_error"_a = false)
        .def("process_json", &JsonQuery::process_json, "msgpack"_a, py::kw_only(), "skip_predicate"_a = false, "raise_error"_a = false)
        .def("export", [](const JsonQuery& self) {
            auto output = self.export_();
            return py::bytes(reinterpret_cast<const char *>(output.data()), output.size());
        }, "Export as bytes")
        .def("export_json", &JsonQuery::export_json, "Export as json")
        .def_readwrite("debug", &JsonQuery::debug)
        //
        ;



#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
