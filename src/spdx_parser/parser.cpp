#include "parser.hpp"

#include <nlohmann/json.hpp>

namespace ts {

    auto from_json( const json& j, Checksum& c ) -> void {
        c.algorithm = require<std::string>( j, "algorithm", "Checksum" );
        c.checksumValue = require<std::string>( j, "checksumValue", "Checksum" );
    }

    auto from_json( const json& j, PackageVerificationCode& p ) -> void {
        p.value = require<std::string>( j, "packageVerificationCodeValue", "PackageVerificationCode" );
        p.excludedFiles = vec<std::string>( j, "packageVerificationCodeExcludedFiles", "PackageVerificationCode" );
    }

    auto from_json( const json& j, ExternalRef& r ) -> void {
        r.referenceCategory = require<std::string>( j, "referenceCategory", "ExternalRef" );
        r.referenceType = require<std::string>( j, "referenceType", "ExternalRef" );
        r.referenceLocator = require<std::string>( j, "referenceLocator", "ExternalRef" );
        r.comment = opt<std::string>( j, "comment", "ExternalRef" );
    }

    auto from_json( const json& j, ExternalDocumentRef& r ) -> void {
        r.externalDocumentId = require<std::string>( j, "externalDocumentId", "ExternalDocumentRef" );
        r.checksum = require<Checksum>( j, "checksum", "ExternalDocumentRef" );
        r.spdxDocument = require<std::string>( j, "spdxDocument", "ExternalDocumentRef" );
    }

    auto from_json( const json& j, ExtractedLicensingInfo& e ) -> void {
        e.licenseId = opt<std::string>( j, "licenseId", "ExtractedLicensingInfo" );
        e.extractedText = opt<std::string>( j, "extractedText", "ExtractedLicensingInfo" );
        e.name = opt<std::string>( j, "name", "ExtractedLicensingInfo" );
        e.seeAlsos = vec<std::string>( j, "seeAlsos", "ExtractedLicensingInfo" );
        e.comment = opt<std::string>( j, "comment", "ExtractedLicensingInfo" );
    }

    auto from_json( const json& j, CreationInfo& c ) -> void {
        c.creators = vec<std::string>( j, "creators", "CreationInfo" );
        c.created = require<std::string>( j, "created", "CreationInfo" );
        c.licenseListVersion = opt<std::string>( j, "licenseListVersion", "CreationInfo" );
        c.comment = opt<std::string>( j, "comment", "CreationInfo" );
    }

    auto from_json( const json& j, Annotation& a ) -> void {
        a.annotator = require<std::string>( j, "annotator", "Annotation" );
        a.annotationDate = require<std::string>( j, "annotationDate", "Annotation" );
        a.annotationType = require<std::string>( j, "annotationType", "Annotation" );
        a.spdxId = require<std::string>( j, "SPDXID", "Annotation" );
        a.annotationComment = require<std::string>( j, "annotationComment", "Annotation" );
    }

    auto from_json( const json& j, Relationship& r ) -> void {
        r.spdxElementId = require<std::string>( j, "spdxElementId", "Relationship" );
        r.relatedSpdxElement = require<std::string>( j, "relatedSpdxElement", "Relationship" );
        r.relationshipType = require<std::string>( j, "relationshipType", "Relationship" );
        r.comment = opt<std::string>( j, "comment", "Relationship" );
    }

    auto from_json( const json& j, SnippetPointer& p ) -> void {
        p.reference = require<std::string>( j, "reference", "SnippetPointer" );
        p.offset = opt<long long>( j, "offset", "SnippetPointer" );
        p.lineNumber = opt<long long>( j, "lineNumber", "SnippetPointer" );
    }

    auto from_json( const json& j, SnippetRange& r ) -> void {
        r.startPointer = require<SnippetPointer>( j, "startPointer", "SnippetRange" );
        r.endPointer = require<SnippetPointer>( j, "endPointer", "SnippetRange" );
    }

    auto from_json( const json& j, Snippet& s ) -> void {
        s.spdxId = require<std::string>( j, "SPDXID", "Snippet" );
        s.snippetFromFile = require<std::string>( j, "snippetFromFile", "Snippet" );
        s.ranges = vec<SnippetRange>( j, "ranges", "Snippet" );
        s.licenseConcluded = opt<std::string>( j, "licenseConcluded", "Snippet" );
        s.licenseInfoInSnippets = vec<std::string>( j, "licenseInfoInSnippets", "Snippet" );
        s.licenseComments = opt<std::string>( j, "licenseComments", "Snippet" );
        s.copyrightText = opt<std::string>( j, "copyrightText", "Snippet" );
        s.comment = opt<std::string>( j, "comment", "Snippet" );
        s.name = opt<std::string>( j, "name", "Snippet" );
        s.attributionTexts = vec<std::string>( j, "attributionTexts", "Snippet" );
    }

    auto from_json( const json& j, SpdxFile& f ) -> void {
        f.spdxId = require<std::string>( j, "SPDXID", "File" );
        f.fileName = require<std::string>( j, "fileName", "File" );
        f.fileTypes = vec<std::string>( j, "fileTypes", "File" );
        f.checksums = vec<Checksum>( j, "checksums", "File" );
        f.licenseConcluded = opt<std::string>( j, "licenseConcluded", "File" );
        f.licenseInfoInFiles = vec<std::string>( j, "licenseInfoInFiles", "File" );
        f.licenseComments = opt<std::string>( j, "licenseComments", "File" );
        f.copyrightText = opt<std::string>( j, "copyrightText", "File" );
        f.comment = opt<std::string>( j, "comment", "File" );
        f.noticeText = opt<std::string>( j, "noticeText", "File" );
        f.fileContributors = vec<std::string>( j, "fileContributors", "File" );
        f.attributionTexts = vec<std::string>( j, "attributionTexts", "File" );
    }

    auto from_json( const json& j, Package& p ) -> void {
        p.spdxId = require<std::string>( j, "SPDXID", "Package" );
        p.name = require<std::string>( j, "name", "Package" );
        p.versionInfo = opt<std::string>( j, "versionInfo", "Package" );
        p.packageFileName = opt<std::string>( j, "packageFileName", "Package" );
        p.supplier = opt<std::string>( j, "supplier", "Package" );
        p.originator = opt<std::string>( j, "originator", "Package" );
        p.downloadLocation = require<std::string>( j, "downloadLocation", "Package" );
        p.filesAnalyzed = opt<bool>( j, "filesAnalyzed", "Package" );
        p.packageVerificationCode = opt<PackageVerificationCode>( j, "packageVerificationCode", "Package" );
        p.checksums = vec<Checksum>( j, "checksums", "Package" );
        p.homepage = opt<std::string>( j, "homepage", "Package" );
        p.sourceInfo = opt<std::string>( j, "sourceInfo", "Package" );
        p.licenseConcluded = opt<std::string>( j, "licenseConcluded", "Package" );
        p.licenseInfoFromFiles = vec<std::string>( j, "licenseInfoFromFiles", "Package" );
        p.licenseDeclared = opt<std::string>( j, "licenseDeclared", "Package" );
        p.licenseComments = opt<std::string>( j, "licenseComments", "Package" );
        p.copyrightText = opt<std::string>( j, "copyrightText", "Package" );
        p.summary = opt<std::string>( j, "summary", "Package" );
        p.description = opt<std::string>( j, "description", "Package" );
        p.comment = opt<std::string>( j, "comment", "Package" );
        p.externalRefs = vec<ExternalRef>( j, "externalRefs", "Package" );
        p.attributionTexts = vec<std::string>( j, "attributionTexts", "Package" );
        p.primaryPackagePurpose = opt<std::string>( j, "primaryPackagePurpose", "Package" );
        p.releaseDate = opt<std::string>( j, "releaseDate", "Package" );
        p.builtDate = opt<std::string>( j, "builtDate", "Package" );
        p.validUntilDate = opt<std::string>( j, "validUntilDate", "Package" );
        p.hasFiles = vec<std::string>( j, "hasFiles", "Package" );
    }

    auto from_json( const json& j, Document& d ) -> void {
        d.spdxVersion = require<std::string>( j, "spdxVersion", "Document" );
        d.dataLicense = require<std::string>( j, "dataLicense", "Document" );
        d.spdxId = require<std::string>( j, "SPDXID", "Document" );
        d.name = require<std::string>( j, "name", "Document" );
        d.documentNamespace = require<std::string>( j, "documentNamespace", "Document" );
        d.creationInfo = require<CreationInfo>( j, "creationInfo", "Document" );
        d.comment = opt<std::string>( j, "comment", "Document" );
        d.documentDescribes = vec<std::string>( j, "documentDescribes", "Document" );
        d.externalDocumentRefs = vec<ExternalDocumentRef>( j, "externalDocumentRefs", "Document" );
        d.packages = vec<Package>( j, "packages", "Document" );
        d.files = vec<SpdxFile>( j, "files", "Document" );
        d.snippets = vec<Snippet>( j, "snippets", "Document" );
        d.relationships = vec<Relationship>( j, "relationships", "Document" );
        d.annotations = vec<Annotation>( j, "annotations", "Document" );
        d.hasExtractedLicensingInfos = vec<ExtractedLicensingInfo>( j, "hasExtractedLicensingInfos", "Document" );
    }

    // _entry

    [[nodiscard]]
    auto parse( const json& j ) -> Document {
        if( !j.is_object() ) {
            throw ParseError( "SPDX document root must be a JSON object" );
        }

        Document doc;

        from_json( j, doc );

        if( !doc.spdxVersion.starts_with( "SPDX-2." ) ) {
            throw ParseError( "unsupported spdxVersion \"" + doc.spdxVersion + "\" (expected an SPDX-2.x document)" );
        }

        return doc;
    }

    [[nodiscard]]
    auto parse_string( const std::string& text ) -> Document {
        json j;

        try {
            j = json::parse( text );
        } catch( const json::parse_error& e ) {
            throw ParseError( std::string( "invalid JSON: " ) + e.what() );
        }

        return parse( j );
    }

    [[nodiscard]]
    auto parse_file( const std::filesystem::path& path ) -> Document {
        std::ifstream in( path, std::ios::binary );

        if( !in ) {
            throw ParseError( "could not open file: " + path.string() );
        }

        std::ostringstream ss;

        ss << in.rdbuf();

        return parse_string( ss.str() );
    }

}  // namespace ts
