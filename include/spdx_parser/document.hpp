#pragma once

#include "core.hpp"
#include "file.hpp"
#include "package.hpp"

namespace ts {
    struct Document {
        std::string spdxVersion;

        std::string dataLicense;

        std::string spdxId;

        std::string name;

        std::string documentNamespace;

        CreationInfo creationInfo;

        std::optional<std::string> comment;

        std::vector<std::string> documentDescribes;

        std::vector<ExternalDocumentRef> externalDocumentRefs;

        std::vector<Package> packages;

        std::vector<SpdxFile> files;

        std::vector<Snippet> snippets;

        std::vector<Relationship> relationships;

        std::vector<Annotation> annotations;

        std::vector<ExtractedLicensingInfo> hasExtractedLicensingInfos;
    };

}  // namespace ts
