#pragma once

#include "core.hpp"
#include "document.hpp"
#include "file.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace ts {

// NOTE: changed to out-params ...
auto from_json(const json &j, Checksum &c) -> void;
auto from_json(const json &j, PackageVerificationCode &p) -> void;
auto from_json(const json &j, ExternalRef &r) -> void;
auto from_json(const json &j, ExternalDocumentRef &r) -> void;
auto from_json(const json &j, ExtractedLicensingInfo &e) -> void;
auto from_json(const json &j, CreationInfo &c) -> void;
auto from_json(const json &j, Annotation &a) -> void;
auto from_json(const json &j, Relationship &r) -> void;
auto from_json(const json &j, SnippetPointer &p) -> void;
auto from_json(const json &j, SnippetRange &r) -> void;
auto from_json(const json &j, Snippet &s) -> void;
auto from_json(const json &j, SpdxFile &f) -> void;
auto from_json(const json &j, Package &p) -> void;
auto from_json(const json &j, Document &d) -> void;

[[nodiscard]]
auto parse(const json &j) -> Document;

[[nodiscard]]
auto parse_string(const std::string &text) -> Document;

[[nodiscard]]
auto parse_file(const std::filesystem::path &path) -> Document;

} // namespace ts
