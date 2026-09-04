/**
 * @file temp_dir.h
 * @brief Test-only RAII owner for a short unique temporary directory under `/tmp`.
 */
#pragma once

#include <unistd.h>

#include <array>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace edge::test {

    /**
     * @brief Creates and recursively removes one unique short-path temporary directory.
     *
     * @details The `/tmp/edge-XXXXXX` template is deliberately short so Unix-domain socket paths
     * built beneath it remain comfortably below Linux `sockaddr_un::sun_path` limits. Destruction
     * removes all contents best-effort without throwing, which makes it safe for test teardown.
     */
    class Temp_Dir final {
    public:
        /** @brief Creates a unique directory with mkdtemp(). @throws std::runtime_error on failure. */
        Temp_Dir() {
            static constexpr auto TEMPLATE = std::to_array( "/tmp/edge-XXXXXX" );
            std::vector<char> buffer( TEMPLATE.begin(), TEMPLATE.end() );
            if( ::mkdtemp( buffer.data() ) == nullptr ) {
                throw std::runtime_error( "Temp_Dir: mkdtemp() failed" );
            }
            m_path.assign( buffer.data() );
        }

        /** @brief Recursively removes the temporary directory and ignores teardown errors. */
        ~Temp_Dir() {
            std::error_code error_code;
            std::filesystem::remove_all( m_path, error_code );
        }

        Temp_Dir( const Temp_Dir& ) = delete;
        auto operator=( const Temp_Dir& ) -> Temp_Dir& = delete;
        Temp_Dir( Temp_Dir&& ) = delete;
        auto operator=( Temp_Dir&& ) -> Temp_Dir& = delete;

        /** @brief Returns the owned directory's absolute path. */
        [[nodiscard]] auto Path() const noexcept -> const std::string& { return m_path; }

    private:
        /** Absolute path created by mkdtemp(). */
        std::string m_path;
    };

}  // namespace edge::test
