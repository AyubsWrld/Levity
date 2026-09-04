#pragma once

#include <string>
#include <string_view>

namespace ts 
{

#define SPDX_SOFTWARE_PURPOSE_ENTRIES(X) \
    X(Application, "application")        \
    X(Archive, "archive")                \
    X(Bom, "bom")                        \
    X(Configuration, "configuration")    \
    X(Container, "container")            \
    X(Data, "data")                      \
    X(Device, "device")                  \
    X(DeviceDriver, "deviceDriver")      \
    X(DiskImage, "diskImage")            \
    X(Documentation, "documentation")    \
    X(Evidence, "evidence")              \
    X(Executable, "executable")          \
    X(File, "file")                      \
    X(FilesystemImage, "filesystemImage")\
    X(Firmware, "firmware")              \
    X(Framework, "framework")            \
    X(Install, "install")                \
    X(Library, "library")                \
    X(Manifest, "manifest")              \
    X(Model, "model")                    \
    X(Module, "module")                  \
    X(OperatingSystem, "operatingSystem")\
    X(Other, "other")                    \
    X(Patch, "patch")                    \
    X(Platform, "platform")              \
    X(Requirement, "requirement")        \
    X(Source, "source")                  \
    X(Specification, "specification")    \
    X(Test, "test")

enum class SoftwarePurpose {
#define X(name, str) name,
    SPDX_SOFTWARE_PURPOSE_ENTRIES(X)
#undef X
    Unknown
};

std::string_view to_string(SoftwarePurpose v) noexcept;
SoftwarePurpose softwarePurposeFromString(std::string_view s) noexcept;

#define SPDX_FILE_KIND_ENTRIES(X) \
    X(Directory, "directory")     \
    X(RegularFile, "file")

enum class FileKindType {
#define X(name, str) name,
    SPDX_FILE_KIND_ENTRIES(X)
#undef X
    Unknown
};

std::string_view to_string(FileKindType v) noexcept;
FileKindType fileKindTypeFromString(std::string_view s) noexcept;

#define SPDX_SBOM_TYPE_ENTRIES(X) \
    X(Analyzed, "analyzed")       \
    X(Build, "build")             \
    X(Deployed, "deployed")       \
    X(Design, "design")           \
    X(Runtime, "runtime")         \
    X(Source, "source")

enum class SbomType {
#define X(name, str) name,
    SPDX_SBOM_TYPE_ENTRIES(X)
#undef X
    Unknown
};

std::string_view to_string(SbomType v) noexcept;
SbomType sbomTypeFromString(std::string_view s) noexcept;


#define SPDX_CONTENT_IDENTIFIER_TYPE_ENTRIES(X) \
    X(Gitoid, "gitoid")                         \
    X(Swhid, "swhid")

enum class ContentIdentifierType {
#define X(name, str) name,
    SPDX_CONTENT_IDENTIFIER_TYPE_ENTRIES(X)
#undef X
    Unknown
};

std::string_view to_string(ContentIdentifierType v) noexcept;
ContentIdentifierType contentIdentifierTypeFromString(std::string_view s) noexcept;

#define SPDX_EXTERNAL_IDENTIFIER_TYPE_ENTRIES(X) \
    X(Cpe22, "cpe22")                            \
    X(Cpe23, "cpe23")                            \
    X(Cve, "cve")                                \
    X(Email, "email")                            \
    X(Gitoid, "gitoid")                          \
    X(Other, "other")                            \
    X(PackageUrl, "packageUrl")                  \
    X(SecurityOther, "securityOther")            \
    X(Swhid, "swhid")                            \
    X(Swid, "swid")                              \
    X(UrlScheme, "urlScheme")

enum class ExternalIdentifierType {
#define X(name, str) name,
    SPDX_EXTERNAL_IDENTIFIER_TYPE_ENTRIES(X)
#undef X
    Unknown
};

std::string_view to_string(ExternalIdentifierType v) noexcept;
ExternalIdentifierType externalIdentifierTypeFromString(std::string_view s) noexcept;


#define SPDX_RELATIONSHIP_TYPE_ENTRIES(X)               \
    X(Affects, "affects")                                \
    X(AmendedBy, "amendedBy")                             \
    X(AncestorOf, "ancestorOf")                           \
    X(AvailableFrom, "availableFrom")                      \
    X(Configures, "configures")                            \
    X(Contains, "contains")                                 \
    X(CoordinatedBy, "coordinatedBy")                        \
    X(CopiedTo, "copiedTo")                                   \
    X(DelegatedTo, "delegatedTo")                              \
    X(DependsOn, "dependsOn")                                   \
    X(DescendantOf, "descendantOf")                              \
    X(Describes, "describes")                                     \
    X(DoesNotAffect, "doesNotAffect")                               \
    X(ExpandsTo, "expandsTo")                                        \
    X(ExploitCreatedBy, "exploitCreatedBy")                            \
    X(FixedBy, "fixedBy")                                                \
    X(FixedIn, "fixedIn")                                                 \
    X(FoundBy, "foundBy")                                                  \
    X(Generates, "generates")                                              \
    X(HasAddedFile, "hasAddedFile")                                         \
    X(HasAssessmentFor, "hasAssessmentFor")                                  \
    X(HasAssociatedVulnerability, "hasAssociatedVulnerability")               \
    X(HasConcludedLicense, "hasConcludedLicense")                              \
    X(HasDataFile, "hasDataFile")                                                \
    X(HasDeclaredLicense, "hasDeclaredLicense")                                   \
    X(HasDeletedFile, "hasDeletedFile")                                            \
    X(HasDependencyManifest, "hasDependencyManifest")                               \
    X(HasDistributionArtifact, "hasDistributionArtifact")                            \
    X(HasDocumentation, "hasDocumentation")                                           \
    X(HasDynamicLink, "hasDynamicLink")                                                \
    X(HasEvidence, "hasEvidence")                                                       \
    X(HasExample, "hasExample")                                                          \
    X(HasHost, "hasHost")                                                                 \
    X(HasInput, "hasInput")                                                                \
    X(HasMetadata, "hasMetadata")                                                           \
    X(HasOptionalComponent, "hasOptionalComponent")                                          \
    X(HasOptionalDependency, "hasOptionalDependency")                                         \
    X(HasOutput, "hasOutput")                                                                  \
    X(HasPrerequisite, "hasPrerequisite")                                                       \
    X(HasProvidedDependency, "hasProvidedDependency")                                            \
    X(HasRequirement, "hasRequirement")                                                           \
    X(HasSpecification, "hasSpecification")                                                        \
    X(HasStaticLink, "hasStaticLink")                                                                \
    X(HasTest, "hasTest")                                                                             \
    X(HasTestCase, "hasTestCase")                                                                      \
    X(HasVariant, "hasVariant")                                                                         \
    X(InvokedBy, "invokedBy")                                                                            \
    X(ModifiedBy, "modifiedBy")                                                                           \
    X(Other, "other")                                                                                      \
    X(PackagedBy, "packagedBy")                                                                             \
    X(PatchedBy, "patchedBy")                                                                                \
    X(PublishedBy, "publishedBy")                                                                             \
    X(ReportedBy, "reportedBy")                                                                                \
    X(RepublishedBy, "republishedBy")                                                                           \
    X(SerializedInArtifact, "serializedInArtifact")                                                              \
    X(TestedOn, "testedOn")                                                                                       \
    X(TrainedOn, "trainedOn")                                                                                      \
    X(UnderInvestigationFor, "underInvestigationFor")                                                               \
    X(UsesTool, "usesTool")

enum class RelationshipType {
#define X(name, str) name,
    SPDX_RELATIONSHIP_TYPE_ENTRIES(X)
#undef X
    Unknown
};

std::string_view to_string(RelationshipType v) noexcept;
RelationshipType relationshipTypeFromString(std::string_view s) noexcept;

}  

