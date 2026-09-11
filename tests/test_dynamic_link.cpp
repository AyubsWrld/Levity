#include "dynamic_link.hpp"
#include "elf.hpp"
#include "micro_test.hpp"
#include "package.hpp"
#include "purl.hpp"

#include "fixture_paths.hpp"

#include <fstream>

namespace {

// Fake resolver so package-manager resolution (Conan/dpkg) doesn't need to
// be real for these tests -- it always returns whatever fixed set of
// artifact paths the test configured it with.
class FakeResolver : public ts::PackageResolver {
public:
  explicit FakeResolver(std::vector<std::filesystem::path> artifacts)
      : artifacts_(std::move(artifacts)) {}

  auto ResolveSharedLibraries(const ts::Purl &)
      -> std::vector<std::filesystem::path> override {
    return artifacts_;
  }

private:
  std::vector<std::filesystem::path> artifacts_;
};

auto MakePackageWithPurl(const std::string &locator) -> ts::Package {
  ts::Package package;
  package.name = "test-lgpl-package";
  ts::ExternalRef ref;
  ref.referenceCategory = "PACKAGE-MANAGER";
  ref.referenceType = "purl";
  ref.referenceLocator = locator;
  package.externalRefs.push_back(ref);
  return package;
}

} // namespace

// ---- Purl parsing ----

TEST(ParsePurl_Conan) {
  auto purl = ts::ParsePurl("pkg:conan/fmt@11.2.0");
  EXPECT_TRUE(purl.has_value());
  EXPECT_EQ(purl->type, std::string("conan"));
  EXPECT_EQ(purl->name, std::string("fmt"));
  EXPECT_TRUE(purl->version.has_value());
  EXPECT_EQ(*purl->version, std::string("11.2.0"));
  EXPECT_TRUE(!purl->ns.has_value());
}

TEST(ParsePurl_DebWithNamespace) {
  auto purl = ts::ParsePurl("pkg:deb/debian/libssl3@1.1.1n-0%2Bdeb11u5");
  EXPECT_TRUE(purl.has_value());
  EXPECT_EQ(purl->type, std::string("deb"));
  EXPECT_TRUE(purl->ns.has_value());
  EXPECT_EQ(*purl->ns, std::string("debian"));
  EXPECT_EQ(purl->name, std::string("libssl3"));
}

// ---- Case 1/2/3/4 from the spec: exact-match SONAME intersection ----

TEST(IntersectSonames_MatchingSoname_Passes) {
  auto matches = ts::IntersectSonames({"libfoo.so.1"}, {"libfoo.so.1"});
  EXPECT_EQ(matches.size(), 1u);
}

TEST(IntersectSonames_NoMatchingSoname_Fails) {
  auto matches = ts::IntersectSonames({"libfoo.so.1"}, {"libbar.so.1"});
  EXPECT_TRUE(matches.empty());
}

TEST(IntersectSonames_MultiplePackageLibraries_MatchesTheOneThatsNeeded) {
  auto matches = ts::IntersectSonames(
      {"libavcodec.so.61", "libavformat.so.61", "libavutil.so.59"},
      {"libc.so.6", "libavcodec.so.61"});
  EXPECT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches.front(), std::string("libavcodec.so.61"));
}

TEST(IntersectSonames_SimilarButDistinctName_DoesNotMatch) {
  // Must not use substring matching: libfoo.so.1 must not match
  // libfoobar.so.1.
  auto matches = ts::IntersectSonames({"libfoo.so.1"}, {"libfoobar.so.1"});
  EXPECT_TRUE(matches.empty());
}

// ---- Unresolved-evidence cases (never silently "not linked") ----

TEST(EvaluateDynamicLink_NoPurl_IsUnresolved) {
  ts::Package package;
  package.name = "no-purl-package";
  FakeResolver resolver({});
  auto result = ts::EvaluateDynamicLink(package, fixtures::kDynamicFixtureBinary,
                                        resolver);
  EXPECT_TRUE(result.status == ts::DynamicLinkStatus::Unresolved);
  EXPECT_TRUE(result.message.find("Unable to determine") != std::string::npos);
}

TEST(EvaluateDynamicLink_PackageOwnsNoSharedLibraries_IsUnresolved) {
  auto package = MakePackageWithPurl("pkg:conan/fmt@11.2.0");
  FakeResolver resolver({}); // resolves, but owns nothing
  auto result = ts::EvaluateDynamicLink(package, fixtures::kDynamicFixtureBinary,
                                        resolver);
  EXPECT_TRUE(result.status == ts::DynamicLinkStatus::Unresolved);
}

TEST(EvaluateDynamicLink_TargetMissing_IsUnresolved) {
  auto package = MakePackageWithPurl("pkg:conan/fmt@11.2.0");
  FakeResolver resolver({fixtures::kLgplFixtureLib});
  auto result = ts::EvaluateDynamicLink(
      package, "/nonexistent/path/to/binary", resolver);
  EXPECT_TRUE(result.status == ts::DynamicLinkStatus::Unresolved);
}

TEST(EvaluateDynamicLink_TargetNotElf_IsUnresolved) {
  std::filesystem::path not_elf =
      std::filesystem::path(fixtures::kDynamicFixtureBinary).parent_path() /
      "not_an_elf.txt";
  {
    std::ofstream out(not_elf);
    out << "not an ELF file";
  }

  auto package = MakePackageWithPurl("pkg:conan/fmt@11.2.0");
  FakeResolver resolver({fixtures::kLgplFixtureLib});
  auto result = ts::EvaluateDynamicLink(package, not_elf, resolver);
  EXPECT_TRUE(result.status == ts::DynamicLinkStatus::Unresolved);
}

// ---- Real ELF fixture integration test (Part 31) ----

TEST(Elf_GetSoName_ReadsRealFixtureLibrarySoname) {
  auto soname = ts::GetSoName(fixtures::kLgplFixtureLib);
  EXPECT_TRUE(soname.has_value());
  EXPECT_TRUE(soname->find("liblgpl_fixture.so") != std::string::npos);
}

TEST(Elf_GetNeededLibraries_DynamicFixtureNeedsTheLibrary) {
  auto needed = ts::GetNeededLibraries(fixtures::kDynamicFixtureBinary);
  auto soname = ts::GetSoName(fixtures::kLgplFixtureLib);
  EXPECT_TRUE(soname.has_value());
  bool found = false;
  for (const auto &n : needed) {
    if (n == *soname) {
      found = true;
    }
  }
  EXPECT_TRUE(found);
}

TEST(Elf_GetNeededLibraries_UnlinkedFixtureDoesNotNeedTheLibrary) {
  auto needed = ts::GetNeededLibraries(fixtures::kUnlinkedFixtureBinary);
  auto soname = ts::GetSoName(fixtures::kLgplFixtureLib);
  EXPECT_TRUE(soname.has_value());
  for (const auto &n : needed) {
    EXPECT_NE(n, *soname);
  }
}

TEST(EvaluateDynamicLink_RealFixture_DynamicallyLinkedBinaryPasses) {
  auto package = MakePackageWithPurl("pkg:conan/lgpl_fixture@1.0.0");
  FakeResolver resolver({fixtures::kLgplFixtureLib});
  auto result = ts::EvaluateDynamicLink(package, fixtures::kDynamicFixtureBinary,
                                        resolver);
  EXPECT_TRUE(result.status == ts::DynamicLinkStatus::Linked);
}

TEST(EvaluateDynamicLink_RealFixture_UnlinkedBinaryFails) {
  auto package = MakePackageWithPurl("pkg:conan/lgpl_fixture@1.0.0");
  FakeResolver resolver({fixtures::kLgplFixtureLib});
  auto result = ts::EvaluateDynamicLink(package, fixtures::kUnlinkedFixtureBinary,
                                        resolver);
  EXPECT_TRUE(result.status == ts::DynamicLinkStatus::NotLinked);
}
