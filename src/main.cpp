#include "document.hpp"
#include "get_args.hpp"

#include "lgpl-inl.hpp"

#include "parser.hpp"

#include "predicate_executor.hpp"
#include "predicate.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

// Placeholder checks applied to packages with no more specific predicate
// suite (NOASSERTION). Real license-specific predicates, such as LGPL's
// IsDynamicallyLinked in lgpl-inl.hpp, register under their own SPDX
// identifier via TS_DECL_PREDICATE_FOR instead.

TS_DECL_PREDICATE(NOASSERTION, IsStaticallyLinked) {
  TS_EXPECT_TRUE(Context().package != nullptr);
  // placeholder condition we can swap for something else later
  TS_ASSERT_TRUE(true);
}

TS_DECL_PREDICATE(NOASSERTION, HasDefinedSource) { TS_ASSERT_TRUE(true); }


/* Date Time */ 
/* Package Name */ 
/* Package Version */ 
/* Predicate */ 
/* Pass/Failed? */ 

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
	ts::Executor e; // default ctor calls everything ...
	auto results = e.ExecuteAll();
	return EXIT_SUCCESS;
}
