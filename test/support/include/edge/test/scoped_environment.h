/**
 * @file scoped_environment.h
 * @brief Test-only RAII helper for temporary process-environment overrides.
 */
#pragma once

#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace edge::test {

    /**
     * @brief Saves one environment variable and restores its original state on destruction.
     *
     * @details Tests use this class to isolate runtime-config parsing without leaking environment
     * changes into later tests. The object is intentionally non-movable so exactly one scope owns the
     * restoration responsibility.
     *
     * @warning Environment variables are process-global. Do not mutate the same variable concurrently
     * from multiple test threads.
     */
    class Scoped_Environment final {
    public:
        /**
         * @brief Captures the current state of @p name without changing it.
         * @param name Environment-variable name whose state will be restored at scope exit.
         */
        explicit Scoped_Environment( std::string name ) : m_name( std::move( name ) ) {
            if( const char* current = std::getenv( m_name.c_str() ); current != nullptr ) {
                m_previous_value = current;
            }
        }

        /**
         * @brief Best-effort restores the variable to its pre-construction value/absence.
         * @note POSIX restoration failures cannot be reported safely from the destructor and are ignored.
         */
        ~Scoped_Environment() { Restore(); }

        Scoped_Environment( const Scoped_Environment& ) = delete;
        auto operator=( const Scoped_Environment& ) -> Scoped_Environment& = delete;
        Scoped_Environment( Scoped_Environment&& ) = delete;
        auto operator=( Scoped_Environment&& ) -> Scoped_Environment& = delete;

        /**
         * @brief Sets/overwrites the scoped variable.
         * @param value New value to expose through getenv().
         * @throws std::runtime_error if setenv() fails.
         */
        void Set( std::string_view value ) const {
            if( ::setenv( m_name.c_str(), std::string( value ).c_str(), 1 ) != 0 ) {
                throw std::runtime_error( "Scoped_Environment: setenv() failed for " + m_name );
            }
        }

        /**
         * @brief Removes the scoped variable from the process environment.
         * @throws std::runtime_error if unsetenv() fails.
         */
        void Unset() const {
            if( ::unsetenv( m_name.c_str() ) != 0 ) {
                throw std::runtime_error( "Scoped_Environment: unsetenv() failed for " + m_name );
            }
        }

    private:
        /** @brief Best-effort restoration used by the noexcept destructor path. */
        void Restore() noexcept {
            if( m_previous_value.has_value() ) {
                ::setenv( m_name.c_str(), m_previous_value->c_str(), 1 );
            } else {
                ::unsetenv( m_name.c_str() );
            }
        }

        /** Environment-variable name controlled by this scope. */
        std::string m_name;
        /** Original value, or std::nullopt when the variable was originally absent. */
        std::optional<std::string> m_previous_value;
    };

}  // namespace edge::test
