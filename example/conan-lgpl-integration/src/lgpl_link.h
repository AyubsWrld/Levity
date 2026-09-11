#pragma once

namespace lgpl_link {

// Calls the real libiconv API (iconv_open/iconv_close). Only compiled and
// linked into the `myapp` target when the CMake option ENABLE_LGPL_LINK is
// ON -- see CMakeLists.txt. This is what forces the linker to keep
// libiconv's SONAME in DT_NEEDED instead of dropping it via --as-needed.
[[nodiscard]] auto ExerciseIconv() -> bool;

} // namespace lgpl_link
