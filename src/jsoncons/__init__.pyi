"""
Pybind11 bindings for jsoncons
------------------------------

.. currentmodule:: pybind11_jsoncons

.. autosummary::
    :toctree: _generate

    Json
    JsonQuery
    JsonQueryRepl
    msgpack_decode
    msgpack_encode
"""

from __future__ import annotations

from typing import overload

__doc__: str
__version__: str

class Json:
    """
    A class for handling JSON data with conversion to/from JSON and MessagePack formats.
    """
    def __init__(self) -> None:
        """
        Create a new Json object.
        """

    def from_json(self, json_string: str) -> Json:
        """
        Parse JSON from a string.

        Args:
            json_string: JSON string to parse

        Returns:
            Json: Reference to self
        """

    def to_json(self) -> str:
        """
        Convert the JSON object to a string.

        Returns:
            str: JSON string representation
        """

    def from_msgpack(self, msgpack_bytes: bytes) -> Json:
        """
        Parse MessagePack binary data into a JSON object.

        Args:
            msgpack_bytes: MessagePack binary data

        Returns:
            Json: Reference to self
        """

    def to_msgpack(self) -> bytes:
        """
        Convert the JSON object to MessagePack binary data.

        Returns:
            bytes: MessagePack binary data
        """

class JsonQueryRepl:
    """
    A REPL (Read-Eval-Print Loop) for evaluating JMESPath expressions on JSON data.
    """

    doc: Json
    debug: bool

    @overload
    def __init__(self) -> None:
        """
        Create a new JsonQueryRepl instance with null document.
        """

    @overload
    def __init__(self, json: str, debug: bool = False) -> None:
        """
        Create a new JsonQueryRepl instance.

        Args:
            json: JSON text to be parsed
            debug: Whether to enable debug mode (default: False)
        """

    def eval(self, expr: str) -> str:
        """
        Evaluate a JMESPath expression against the JSON document.

        Args:
            expr: JMESPath expression

        Returns:
            str: Result of the evaluation as a string
        """

    def add_params(self, key: str, value: str) -> None:
        """
        Add parameters for JMESPath evaluation.

        Args:
            key: Parameter key
            value: Parameter value as JSON string
        """

class JsonQuery:
    """
    A class for filtering and transforming JSON data using JMESPath expressions.
    """

    debug: bool

    def __init__(self) -> None:
        """
        Create a new JsonQuery instance.
        """

    def setup_predicate(self, predicate: str) -> None:
        """
        Set up the predicate expression used for filtering.

        Args:
            predicate: JMESPath predicate expression
        """

    def setup_transforms(self, transforms: list[str]) -> None:
        """
        Set up transform expressions used for data transformation.

        Args:
            transforms: list of JMESPath transform expressions
        """

    def add_params(self, key: str, value: str) -> None:
        """
        Add parameters for JMESPath evaluation.

        Args:
            key: Parameter key
            value: Parameter value as JSON string
        """

    def matches(self, msgpack: bytes) -> bool:
        """
        Check if a MessagePack message matches the predicate.

        Args:
            msgpack: MessagePack data as bytes

        Returns:
            bool: True if the message matches, False otherwise
        """

    def matches_json(self, json: Json) -> bool:
        """
        Check if a JSON document matches the predicate.

        Args:
            json: JSON document

        Returns:
            bool: True if the document matches, False otherwise
        """

    def process(
        self, msgpack: bytes, *, skip_predicate: bool = False, raise_error: bool = False
    ) -> bool:
        """
        Process a MessagePack message with predicate matching and transformation.

        Args:
            msgpack: MessagePack data as bytes
            skip_predicate: Whether to skip predicate matching (default: False)
            raise_error: Whether to raise errors during transformation (default: False)

        Returns:
            bool: True if processing succeeded, False otherwise
        """

    def process_json(
        self, json: Json, *, skip_predicate: bool = False, raise_error: bool = False
    ) -> bool:
        """
        Process a JSON document with predicate matching and transformation.

        Args:
            json: JSON document
            skip_predicate: Whether to skip predicate matching (default: False)
            raise_error: Whether to raise errors during transformation (default: False)

        Returns:
            bool: True if processing succeeded, False otherwise
        """

    def export(self) -> bytes:
        """
        Export the processed data as MessagePack.

        Returns:
            bytes: MessagePack binary data containing the processed results
        """

    def export_json(self) -> Json:
        """
        Export the processed data as JSON.

        Returns:
            Json: JSON array of processed data
        """

    def clear(self) -> None:
        """
        Clear all processed data.
        """

def msgpack_decode(msgpack_bytes: bytes) -> str:
    """
    Convert MessagePack binary data to a JSON string.

    Args:
        msgpack_bytes: MessagePack binary data

    Returns:
        str: JSON string representation
    """

def msgpack_encode(json_string: str) -> bytes:
    """
    Convert a JSON string to MessagePack binary format.

    Args:
        json_string: JSON string to encode

    Returns:
        bytes: MessagePack binary data
    """
