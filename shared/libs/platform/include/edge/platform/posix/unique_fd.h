/**
 * @file unique_fd.h
 * @brief Move-only RAII ownership wrapper for POSIX file descriptors.
 */
#pragma once

#include <unistd.h>

#include <utility>

namespace edge::platform::posix {

    /**
     * @brief Owns at most one POSIX file descriptor and closes it on destruction/reset.
     *
     * @details `-1` represents no resource. Ownership is exclusive and movable, never copyable.
     * This utility is transport-neutral and can be reused by UDP/TCP/serial/test infrastructure.
     */
    class Unique_Fd final {
    public:
        /** @brief Constructs an empty descriptor owner. */
        Unique_Fd() noexcept = default;
        /** @brief Takes ownership of @p fd; pass -1 for an empty owner. */
        explicit Unique_Fd( int fd ) noexcept : m_fd( fd ) {}

        /** @brief Closes the owned descriptor when valid. */
        ~Unique_Fd() { Reset(); }

        Unique_Fd( const Unique_Fd& ) = delete;
        auto operator=( const Unique_Fd& ) -> Unique_Fd& = delete;

        /** @brief Transfers descriptor ownership from @p other, leaving it empty. */
        Unique_Fd( Unique_Fd&& other ) noexcept : m_fd( std::exchange( other.m_fd, -1 ) ) {}

        /** @brief Closes any current descriptor, then transfers ownership from @p other. */
        auto operator=( Unique_Fd&& other ) noexcept -> Unique_Fd& {
            if( this != &other ) {
                Reset( std::exchange( other.m_fd, -1 ) );
            }
            return *this;
        }

        /** @brief Returns the raw descriptor without releasing ownership. */
        [[nodiscard]] auto Get() const noexcept -> int { return m_fd; }
        /** @brief Returns true when this object owns a valid descriptor. */
        [[nodiscard]] explicit operator bool() const noexcept { return m_fd >= 0; }

        /**
         * @brief Releases ownership without closing and returns the raw descriptor.
         * @return Previously owned descriptor, or -1 when empty.
         */
        [[nodiscard]] auto Release() noexcept -> int { return std::exchange( m_fd, -1 ); }

        /**
         * @brief Closes the currently owned descriptor and optionally takes ownership of @p fd.
         * @param fd Replacement descriptor; defaults to -1 (empty). Passing the currently owned
         * descriptor is a no-op rather than closing it and retaining a stale descriptor number.
         */
        void Reset( int fd = -1 ) noexcept {
            if( fd == m_fd ) {
                return;
            }
            if( m_fd >= 0 ) {
                ::close( m_fd );
            }
            m_fd = fd;
        }

    private:
        /** Owned descriptor or -1 when empty. */
        int m_fd = -1;
    };

}  // namespace edge::platform::posix
