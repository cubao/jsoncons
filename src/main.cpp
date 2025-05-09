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

using json = jsoncons::ojson; // using json = jsoncons::json;
namespace jmespath = jsoncons::jmespath;
namespace msgpack = jsoncons::msgpack;

namespace py = pybind11;
using rvp = py::return_value_policy;
using namespace pybind11::literals;

// Type conversion between jsoncons::json and py::dict
// https://github.com/pybind/pybind11_json/blob/master/include/pybind11_json/pybind11_json.hpp
namespace pyjson
{
    inline py::object from_json(const json& j)
    {
        if (j.is_null())
        {
            return py::none();
        }
        else if (j.is_bool())
        {
            return py::bool_(j.as_bool());
        }
        else if (j.is_int64())
        {
            return py::int_(j.as_integer<int64_t>());
        }
        else if (j.is_uint64())
        {
            return py::int_(j.as_integer<uint64_t>());
        }
        else if (j.is_double())
        {
            return py::float_(j.as_double());
        }
        else if (j.is_string())
        {
            return py::str(j.as_string());
        }
        else if (j.is_array())
        {
            py::list obj(j.size());
            for (std::size_t i = 0; i < j.size(); i++)
            {
                obj[i] = from_json(j[i]);
            }
            return obj;
        }
        else // Object
        {
            py::dict obj;
            for (const auto& item : j.object_range())
            {
                obj[py::str(item.key())] = from_json(item.value());
            }
            return obj;
        }
    }

    inline json to_json(const py::handle& obj, std::set<const PyObject*>& refs)
    {
        if (obj.ptr() == nullptr || obj.is_none())
        {
            return json::null();
        }
        if (py::isinstance<py::bool_>(obj))
        {
            return obj.cast<bool>();
        }
        if (py::isinstance<py::int_>(obj))
        {
            try
            {
                int64_t s = obj.cast<int64_t>();
                if (py::int_(s).equal(obj))
                {
                    return s;
                }
            }
            catch (...)
            {
            }
            try
            {
                uint64_t u = obj.cast<uint64_t>();
                if (py::int_(u).equal(obj))
                {
                    return u;
                }
            }
            catch (...)
            {
            }
            throw std::runtime_error("to_json received an integer out of range for both int64_t and uint64_t type: " + py::repr(obj).cast<std::string>());
        }
        if (py::isinstance<py::float_>(obj))
        {
            return obj.cast<double>();
        }
        if (py::isinstance<py::bytes>(obj))
        {
            py::module base64 = py::module::import("base64");
            return base64.attr("b64encode")(obj).attr("decode")("utf-8").cast<std::string>();
        }
        if (py::isinstance<py::str>(obj))
        {
            return obj.cast<std::string>();
        }
        if (py::isinstance<py::tuple>(obj) || py::isinstance<py::list>(obj))
        {
            auto insert_ret = refs.insert(obj.ptr());
            if (!insert_ret.second) {
                throw std::runtime_error("Circular reference detected");
            }

            auto out = json::array();
            for (const py::handle value : obj)
            {
                out.push_back(to_json(value, refs));
            }

            refs.erase(insert_ret.first);

            return out;
        }
        if (py::isinstance<py::dict>(obj))
        {
            auto insert_ret = refs.insert(obj.ptr());
            if (!insert_ret.second) {
                throw std::runtime_error("Circular reference detected");
            }

            auto out = json::object();
            for (const py::handle key : obj)
            {
                out.try_emplace(py::str(key).cast<std::string>(), to_json(obj[key], refs));
            }

            refs.erase(insert_ret.first);

            return out;
        }

        throw std::runtime_error("to_json not implemented for this type of object: " + py::repr(obj).cast<std::string>());
    }

    inline json to_json(const py::handle& obj)
    {
        std::set<const PyObject*> refs;
        return to_json(obj, refs);
    }
}

/*
namespace pybind11 { namespace detail {
    template <> struct type_caster<json> {
    public:
        PYBIND11_TYPE_CASTER(json, _("json"));

        bool load(handle src, bool)
        {
            try
            {
                value = pyjson::to_json(src);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        static handle cast(json src, return_value_policy policy, handle parent)
        {
            (void)policy;
            (void)parent;
            object obj = pyjson::from_json(src);
            return obj.release();
        }
    };
}} // namespace pybind11::detail
*/

/**
 * A REPL (Read-Eval-Print Loop) for evaluating JMESPath expressions on JSON data.
 */
struct JsonQueryRepl {
    JsonQueryRepl(): doc(json::null()), debug(false) { }
    /**
     * Constructor for JsonQueryRepl.
     * @param jsontext JSON text to be parsed
     * @param debug Whether to enable debug mode
     */
    JsonQueryRepl(const std::string &jsontext, bool debug = false): doc(json::parse(jsontext)), debug(debug) { }

    /**
     * Evaluate a JMESPath expression against the JSON document.
     * @param expr_text JMESPath expression
     * @return Result of the evaluation as a string
     */
    std::string eval(const std::string &expr_text) const {
        auto expr = jmespath::make_expression<json>(expr_text);
        auto result = expr.evaluate(doc, params_);
        if (debug) {
            std::cerr << pretty_print(result) << std::endl;
        }
        return result.to_string();
    }

    /**
     * Add parameters for JMESPath evaluation.
     * @param key Parameter key
     * @param value Parameter value as JSON string
     */
    void add_params(const std::string &key, const std::string &value) {
        params_[key] = json::parse(value);
    }

    json doc;
    bool debug = false;
    private:
    std::map<std::string, json> params_;
};

/**
 * A class for filtering and transforming JSON data using JMESPath expressions.
 */
struct JsonQuery {
    /**
     * Constructor for JsonQuery.
     */
    JsonQuery() {}

    /**
     * Set up the predicate expression used for filtering.
     * @param predicate JMESPath predicate expression
     */
    void setup_predicate(const std::string &predicate) {
        predicate_expr_ = std::make_unique<jmespath::jmespath_expression<json>>(jmespath::make_expression<json>(predicate));
        predicate_ = predicate;
    }

    /**
     * Set up transform expressions used for data transformation.
     * @param transforms List of JMESPath transform expressions
     */
    void setup_transforms(const std::vector<std::string> &transforms) {
        transforms_expr_.clear();
        transforms_expr_.reserve(transforms.size());
        for (auto &t: transforms) {
            transforms_expr_.push_back(std::make_unique<jmespath::jmespath_expression<json>>(jmespath::make_expression<json>(t)));
        }
        transforms_ = transforms;
    }

    /**
     * Add parameters for JMESPath evaluation.
     * @param key Parameter key
     * @param value Parameter value as JSON string
     */
    void add_params(const std::string &key, const std::string &value) {
        params_[key] = json::parse(value);
    }

    /**
     * Check if a MessagePack message matches the predicate.
     * @param msg MessagePack data as string
     * @return True if the message matches, false otherwise
     */
    bool matches(const std::string &msg) const {
        if (!predicate_expr_) {
            return false;
        }
        auto doc = msgpack::decode_msgpack<json>(msg);
        return __matches(doc);
    }

    /**
     * Check if a JSON document matches the predicate.
     * @param doc JSON document
     * @return True if the document matches, false otherwise
     */
    bool matches_json(const json &doc) const {
        if (!predicate_expr_) {
            return false;
        }
        return __matches(doc);
    }

    /**
     * Process a MessagePack message with predicate matching and transformation.
     * @param msg MessagePack data as string
     * @param skip_predicate Whether to skip predicate matching
     * @param raise_error Whether to raise errors during transformation
     * @return True if processing succeeded, false otherwise
     */
    bool process(const std::string &msg, bool skip_predicate = false, bool raise_error = false) {
        auto doc = msgpack::decode_msgpack<json>(msg);
        return process_json(doc, skip_predicate, raise_error);
    }

    /**
     * Process a JSON document with predicate matching and transformation.
     * @param doc JSON document
     * @param skip_predicate Whether to skip predicate matching
     * @param raise_error Whether to raise errors during transformation
     * @return True if processing succeeded, false otherwise
     */
    bool process_json(const json &doc, bool skip_predicate = false, bool raise_error = false) {
        if (!predicate_expr_) {
            skip_predicate = true;
        }
        if (!skip_predicate && !__matches(doc)) {
            return false;
        }
        if (transforms_expr_.empty()) {
            throw std::runtime_error("No transform expressions set");
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

    /**
     * Export the processed data as JSON.
     * @return JSON array of processed data
     */
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

    /**
     * Export the processed data as MessagePack.
     * @return Binary data containing the MessagePack representation
     */
    std::vector<uint8_t> export_() const {
        std::vector<uint8_t> output;
        msgpack::encode_msgpack(export_json(), output);
        return output;
    }

    /**
     * Clear all processed data.
     */
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

    py::class_<json>(m, "Json", py::module_local(), py::dynamic_attr()) //
    .def(py::init<>(), R"pbdoc(
        Create a new Json object.
    )pbdoc")
    // from/to_python
    .def("from_python", [](json &self, const py::handle &obj) -> json & {
        self = pyjson::to_json(obj);
        return self;
    }, "object"_a, rvp::reference_internal, R"pbdoc(
        Convert a Python object to a JSON object.

        This method converts various Python types to their JSON equivalents:
        - None -> null
        - bool -> boolean
        - int -> integer
        - float -> number
        - str -> string
        - list/tuple -> array
        - dict -> object

        Args:
            object: Python object to convert

        Returns:
            Json: Reference to self with converted data

        Raises:
            RuntimeError: If the Python object contains circular references or unsupported types
    )pbdoc")
    .def("to_python", [](const json &self) -> py::handle {
        py::object obj = pyjson::from_json(self);
        return obj.release();
    }, R"pbdoc(
        Convert a JSON object to a Python object.

        This method converts JSON types to their Python equivalents:
        - null -> None
        - boolean -> bool
        - integer -> int
        - number -> float
        - string -> str
        - array -> list
        - object -> dict

        Returns:
            object: Python object representation of the JSON data
    )pbdoc")

    // from/to_json
    .def("from_json", [](json &self, const std::string &input) -> json & {
        self = json::parse(input);
        return self;
    }, "json_string"_a, rvp::reference_internal, R"pbdoc(
        Parse JSON from a string.

        Args:
            json_string: JSON string to parse

        Returns:
            Json: Reference to self
    )pbdoc")
    .def("to_json", [](const json &self) {
        return self.to_string();
    }, R"pbdoc(
        Convert the JSON object to a string.

        Returns:
            str: JSON string representation
    )pbdoc")
    // from/to_msgpack
    .def("from_msgpack", [](json &self, const std::string &input) -> json & {
        self = msgpack::decode_msgpack<json>(input);
        return self;
    }, "msgpack_bytes"_a, rvp::reference_internal, R"pbdoc(
        Parse MessagePack binary data into a JSON object.

        Args:
            msgpack_bytes: MessagePack binary data

        Returns:
            Json: Reference to self
    )pbdoc")
    .def("to_msgpack", [](const json &self) {
        std::vector<uint8_t> output;
        msgpack::encode_msgpack(self, output);
        return py::bytes(reinterpret_cast<const char *>(output.data()), output.size());
    }, R"pbdoc(
        Convert the JSON object to MessagePack binary data.

        Returns:
            bytes: MessagePack binary data
    )pbdoc")
    //
    ;

    py::class_<JsonQueryRepl>(m, "JsonQueryRepl", py::module_local(), py::dynamic_attr()) //
        .def(py::init<>())
        .def(py::init<const std::string &, bool>(), "json"_a, "debug"_a = false, R"pbdoc(
            Create a new JsonQueryRepl instance.

            Args:
                json: JSON text to be parsed
                debug: Whether to enable debug mode (default: False)
        )pbdoc")
        .def("eval", &JsonQueryRepl::eval, "expr"_a, R"pbdoc(
            Evaluate a JMESPath expression against the JSON document.

            Args:
                expr: JMESPath expression

            Returns:
                str: Result of the evaluation as a string
        )pbdoc")
        .def("add_params", &JsonQueryRepl::add_params, "key"_a, "value"_a, R"pbdoc(
            Add parameters for JMESPath evaluation.

            Args:
                key: Parameter key
                value: Parameter value as JSON string
        )pbdoc")
        .def_readwrite("doc", &JsonQueryRepl::doc, R"pbdoc(
            The JSON document being queried. This is the data that JMESPath expressions will be evaluated against.
        )pbdoc")
        .def_readwrite("debug", &JsonQueryRepl::debug, R"pbdoc(
            Debug mode flag. When True, evaluation results will be printed to stderr.
        )pbdoc")
        //
        ;

    py::class_<JsonQuery>(m, "JsonQuery", py::module_local(), py::dynamic_attr()) //
        .def(py::init<>(), R"pbdoc(
            Create a new JsonQuery instance.
        )pbdoc")
        .def("setup_predicate", &JsonQuery::setup_predicate, "predicate"_a, R"pbdoc(
            Set up the predicate expression used for filtering.

            Args:
                predicate: JMESPath predicate expression
        )pbdoc")
        .def("setup_transforms", &JsonQuery::setup_transforms, "transforms"_a, R"pbdoc(
            Set up transform expressions used for data transformation.

            Args:
                transforms: List of JMESPath transform expressions
        )pbdoc")
        .def("add_params", &JsonQuery::add_params, "key"_a, "value"_a, R"pbdoc(
            Add parameters for JMESPath evaluation.

            Args:
                key: Parameter key
                value: Parameter value as JSON string
        )pbdoc")
        .def("matches", &JsonQuery::matches, "msgpack"_a, R"pbdoc(
            Check if a MessagePack message matches the predicate.

            Args:
                msgpack: MessagePack data as bytes

            Returns:
                bool: True if the message matches, False otherwise
        )pbdoc")
        .def("matches_json", &JsonQuery::matches_json, "json"_a, R"pbdoc(
            Check if a JSON document matches the predicate.

            Args:
                json: JSON document

            Returns:
                bool: True if the document matches, False otherwise
        )pbdoc")
        .def("process", &JsonQuery::process, "msgpack"_a, py::kw_only(), "skip_predicate"_a = false, "raise_error"_a = false, R"pbdoc(
            Process a MessagePack message with predicate matching and transformation.

            Args:
                msgpack: MessagePack data as bytes
                skip_predicate: Whether to skip predicate matching (default: False)
                raise_error: Whether to raise errors during transformation (default: False)

            Returns:
                bool: True if processing succeeded, False otherwise
        )pbdoc")
        .def("process_json", &JsonQuery::process_json, "msgpack"_a, py::kw_only(), "skip_predicate"_a = false, "raise_error"_a = false, R"pbdoc(
            Process a JSON document with predicate matching and transformation.

            Args:
                json: JSON document
                skip_predicate: Whether to skip predicate matching (default: False)
                raise_error: Whether to raise errors during transformation (default: False)

            Returns:
                bool: True if processing succeeded, False otherwise
        )pbdoc")
        .def("export", [](const JsonQuery& self) {
            auto output = self.export_();
            return py::bytes(reinterpret_cast<const char *>(output.data()), output.size());
        }, R"pbdoc(
            Export the processed data as MessagePack.

            Returns:
                bytes: MessagePack binary data containing the processed results
        )pbdoc")
        .def("export_json", &JsonQuery::export_json, R"pbdoc(
            Export the processed data as JSON.

            Returns:
                Json: JSON array of processed data
        )pbdoc")
        .def_readwrite("debug", &JsonQuery::debug, R"pbdoc(
            Debug mode flag.
        )pbdoc")
        //
        ;


    m.def("msgpack_encode", [](const std::string &input) {
        std::vector<uint8_t> output;
        msgpack::encode_msgpack(json::parse(input), output);
        return py::bytes(reinterpret_cast<const char *>(output.data()), output.size());
    }, "json_string"_a, R"pbdoc(
        Convert a JSON string to MessagePack binary format.

        Args:
            json_string: JSON string to encode

        Returns:
            bytes: MessagePack binary data
    )pbdoc");

    m.def("msgpack_decode", [](const std::string &input) {
        auto doc = msgpack::decode_msgpack<json>(input);
        return doc.to_string();
    }, "msgpack_bytes"_a, R"pbdoc(
        Convert MessagePack binary data to a JSON string.

        Args:
            msgpack_bytes: MessagePack binary data

        Returns:
            str: JSON string representation
    )pbdoc");

    // m.def("dumps", [](const json &json_val) -> std::string {
    //     return json_val.to_string();
    // });
    // m.def("loads", [](const std::string &json_text) -> json {
    //     return json::parse(json_text);
    // });

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
