#pragma once

// due to C++ preprocessor weirdness, we need double indirection to
// concatenate two tokens when one of them is __LINE__.  Writing
//
//   foo ## __LINE__
//
// will result in the token foo__LINE__, instead of foo followed by

// the current line number.  For more details, see

// https://www.parashift.com/c++-faq-lite/misc-technical-issues.html#faq-39.6

#define TS_CONCAT_TOKEN_(foo, bar) TS_CONCAT_TOKEN_IMPL_(foo, bar)

#define TS_CONCAT_TOKEN_IMPL_(foo, bar) foo##bar

#define MAKE_PREDICATE(predicate_class)                                        \
  ts::PredicateFactoryImpl<TestPredicate>{}.CreatePredicate();
