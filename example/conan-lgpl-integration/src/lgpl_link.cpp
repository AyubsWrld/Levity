#include "lgpl_link.h"

#include <iconv.h>

#include <spdlog/spdlog.h>

namespace lgpl_link {

auto ExerciseIconv() -> bool {
    const iconv_t converter = iconv_open("UTF-8", "UTF-8");
    if (converter == reinterpret_cast<iconv_t>(-1)) {
        spdlog::error("iconv_open failed");
        return false;
    }

    iconv_close(converter);
    spdlog::info("libiconv exercised via iconv_open/iconv_close");
    return true;
}

} // namespace lgpl_link
