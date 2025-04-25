// https://github.com/microsoft/vscode-cpptools/issues/9692
#if __INTELLISENSE__
#undef __ARM_NEON
#undef __ARM_NEON__
#endif

#include <pybind11/pybind11.h>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jmespath/jmespath.hpp>
#include <jsoncons_ext/msgpack/msgpack.hpp>

#include <sstream>

using jsoncons::json;
namespace jmespath = jsoncons::jmespath;

int add(int i, int j) {
    return i + j;
}

namespace py = pybind11;
using namespace pybind11::literals;

// https://github.com/danielaparker/jsoncons/blob/master/doc/ref/jmespath/jmespath.md


struct JsonQueryRepl {
    JsonQueryRepl(const std::string &jsontext, bool debug = false): doc_(json::parse(jsontext)), debug(debug) { }
    std::string eval(const std::string &expr) const {
        auto result = jmespath::search(doc_, expr);
        if (debug) {
            std::cerr << pretty_print(result) << std::endl;
        }
        std::ostringstream os;
        os << result;
        return os.str();
    }
    bool debug = false;
    private:
    json doc_;
};

struct JsonQuery {
    JsonQuery() {}
    void setup_predicate(const std::string &predicate) {
        // predicate_expr_ = std::move(jmespath::make_expression<json>(predicate));
        predicate_ = predicate;
    }
    void setup_transforms(const std::vector<std::string> &transforms) {
        transforms_expr_.reserve(transforms.size());
        for (auto &t: transforms) {
            transforms_expr_.emplace_back(jmespath::make_expression<json>(t));
        }
        transforms_ = transforms;
    }

    bool matches(const std::string &msg) const {
        return true;
    }

    bool process(const std::string &msg, bool skip_predicate = false) {
        if (!skip_predicate) {
        }

        return {};
    }

    std::string export_() const {
        return "";
    }

    void clear() {
        outputs_.clear();
    }

    bool debug = false;

private:
    std::string predicate_;
    jmespath::jmespath_expression<json> predicate_expr_;
    std::vector<std::string> transforms_;
    std::vector<jmespath::jmespath_expression<json>> transforms_expr_;


    std::vector<std::vector<json>> outputs_;
};

PYBIND11_MODULE(_core, m) {
    m.doc() = R"pbdoc(
        Pybind11 example plugin
        -----------------------

        .. currentmodule:: jsoncons

        .. autosummary::
           :toctree: _generate

           add
           subtract
    )pbdoc";

    m.def("add", &add, R"pbdoc(
        Add two numbers

        Some other explanation about the add function.
    )pbdoc");

    m.def("subtract", [](int i, int j) { return i - j; }, R"pbdoc(
        Subtract two numbers

        Some other explanation about the subtract function.
    )pbdoc");

    py::class_<JsonQueryRepl>(m, "JsonQueryRepl", py::module_local(), py::dynamic_attr()) //
        .def(py::init<const std::string &, bool>(), "json"_a, "debug"_a = false)
        .def("eval", &JsonQueryRepl::eval)
        .def_readwrite("debug", &JsonQueryRepl::debug)
        //
        ;

    py::class_<JsonQuery>(m, "JsonQuery", py::module_local(), py::dynamic_attr()) //
        .def(py::init<>())
        .def("setup_predicate", &JsonQuery::setup_predicate)
        .def("setup_transforms", &JsonQuery::setup_transforms)
        .def("matches", &JsonQuery::matches)
        .def("process", &JsonQuery::process)
        .def("export", &JsonQuery::export_)
        .def_readwrite("debug", &JsonQuery::debug)
        //
        ;



#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
