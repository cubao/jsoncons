#include <pybind11/pybind11.h>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jmespath/jmespath.hpp>

using jsoncons::json;
namespace jmespath = jsoncons::jmespath;

int add(int i, int j) {
    return i + j;
}

namespace py = pybind11;

struct JsonQuery {

    JsonQuery() {}
    bool setup_predicate(const std::string &predicate) {
        // auto expr = jmespath::make_expression<json>(predicate);
        predicate_ = predicate;
        return true;
    }
    bool setup_transforms(const std::vector<std::string> &transforms) {
        transforms_ = transforms;
        return true;
    }

    bool matches(const std::string &msg) const {
        return true;
    }

    bool process(const std::string &key, const std::string &msg, bool skip_predicate = false) {
        return {};
    }

    std::string outputs() const {
        return "";
    }

private:
    std::string predicate_;
    std::vector<std::string> transforms_;


    std::vector<std::pair<std::string, std::string>> outputs_;
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

    py::class_<JsonQuery>(m, "JsonQuery", py::module_local()) //
        .def(py::init<>())
        .def("setup_predicate", &JsonQuery::setup_predicate)
        .def("setup_transforms", &JsonQuery::setup_transforms)
        .def("matches", &JsonQuery::matches)
        .def("handle", &JsonQuery::handle)
        .def("outputs", &JsonQuery::outputs);



#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
