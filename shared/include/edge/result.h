/**
 * @file result.h
 * @brief Small C++20 success-or-error result type used at expected-failure boundaries.
 */
#pragma once

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace edge {

    /**
     * @brief Tag used to construct a Result directly in its error state.
     *
     * @details Prefer Make_Failure() for normal return statements. The tag exists primarily for
     * explicit construction in tests and generic code.
     */
    struct Failure_Tag final {};

    /**
     * @brief Global failure-construction tag analogous to an in-place error selector.
     * @details Intentionally lower_case to mirror standard-library tag objects such as
     * std::in_place and std::nullopt.
     */
    // NOLINTNEXTLINE(readability-identifier-naming)
    inline constexpr Failure_Tag failure {};

    /**
     * @brief Lightweight wrapper that carries an error into a Result return value.
     * @tparam E Error type.
     */
    template <typename E>
    struct Failure final {
        /** Error value to store in the destination Result. */
        E error;
    };

    /**
     * @brief Creates a failure wrapper whose error type is deduced from @p error.
     * @tparam E Deduced error argument type.
     * @param error Error value to propagate.
     * @return Failure wrapper convertible to a Result with the same error type.
     */
    template <typename E>
    [[nodiscard]] auto Make_Failure( E&& error ) -> Failure<std::decay_t<E>> {
        return Failure<std::decay_t<E>> { std::forward<E>( error ) };
    }

    /**
     * @brief Represents either a successful value of type @p T or an error of type @p E.
     * @tparam T Success value type.
     * @tparam E Error value type.
     *
     * @details Result is intentionally small and C++20-compatible. It provides the subset of
     * expected-style operations used by this repository without depending on the later-standard library expected-value
     * facility. Normal construction contains exactly one success value or one error value. As with `std::variant`, a
     * throwing user-defined assignment can make the underlying storage valueless; repository result payload/error types
     * are expected to have ordinary value semantics and callers should prefer constructing/returning Results over
     * mutating them.
     */
    template <typename T, typename E>
    class [[nodiscard]] Result final {
    public:
        /**
         * @brief Constructs a successful result by copying @p value.
         * @details Intentionally not `explicit`: implicit conversion from T is the intended
         * ergonomic contract, mirroring std::expected's implicit success conversion.
         */
        // NOLINTNEXTLINE(google-explicit-constructor)
        Result( const T& value ) : m_storage( std::in_place_index<0>, value ) {}

        /** @brief Constructs a successful result by moving @p value. Not `explicit`; see above. */
        // NOLINTNEXTLINE(google-explicit-constructor)
        Result( T&& value ) : m_storage( std::in_place_index<0>, std::move( value ) ) {}

        /**
         * @brief Constructs a failed result from a failure wrapper.
         * @details Not `explicit`: lets Make_Failure(...) be returned directly from a function
         * returning Result<T, E>.
         */
        // NOLINTNEXTLINE(google-explicit-constructor)
        Result( Failure<E> failed ) : m_storage( std::in_place_index<1>, std::move( failed.error ) ) {}

        /**
         * @brief Constructs a failed result by copying @p error.
         * @details The Failure_Tag parameter is intentionally unnamed; it exists only to
         * disambiguate this overload via tag dispatch.
         */
        // NOLINTNEXTLINE(readability-named-parameter)
        Result( Failure_Tag, const E& error ) : m_storage( std::in_place_index<1>, error ) {}

        /** @brief Constructs a failed result by moving @p error. See copying overload above. */
        // NOLINTNEXTLINE(readability-named-parameter)
        Result( Failure_Tag, E&& error ) : m_storage( std::in_place_index<1>, std::move( error ) ) {}

        Result( const Result& ) = default;
        Result( Result&& ) noexcept( std::is_nothrow_move_constructible_v<std::variant<T, E>> ) = default;
        auto operator=( const Result& ) -> Result& = default;
        auto operator=( Result&& ) noexcept( std::is_nothrow_move_assignable_v<std::variant<T, E>> )
            -> Result& = default;
        ~Result() = default;

        /**
         * @return `true` when this result contains a success value.
         * @details lower_case mirrors std::optional's has_value(), the documented public API
         * surface for this type.
         */
        // NOLINTNEXTLINE(readability-identifier-naming)
        [[nodiscard]] auto has_value() const noexcept -> bool { return m_storage.index() == 0; }

        /** @return `true` when this result contains a success value. */
        [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

        /**
         * @brief Accesses the stored success value.
         * @return Mutable lvalue reference to the success value.
         * @throws std::bad_variant_access if the result contains an error.
         * @details lower_case mirrors std::optional::value().
         */
        // NOLINTNEXTLINE(readability-identifier-naming)
        [[nodiscard]] auto value() & -> T& { return std::get<0>( m_storage ); }

        /**
         * @brief Accesses the stored success value.
         * @return Const lvalue reference to the success value.
         * @throws std::bad_variant_access if the result contains an error.
         */
        // NOLINTNEXTLINE(readability-identifier-naming)
        [[nodiscard]] auto value() const& -> const T& { return std::get<0>( m_storage ); }

        /**
         * @brief Moves the stored success value out of an rvalue Result.
         * @return Rvalue reference to the success value.
         * @throws std::bad_variant_access if the result contains an error.
         */
        // NOLINTNEXTLINE(readability-identifier-naming)
        [[nodiscard]] auto value() && -> T&& { return std::get<0>( std::move( m_storage ) ); }

        /**
         * @brief Accesses the stored error value.
         * @return Mutable lvalue reference to the error.
         * @throws std::bad_variant_access if the result contains a success value.
         * @details lower_case mirrors std::expected::error().
         */
        // NOLINTNEXTLINE(readability-identifier-naming)
        [[nodiscard]] auto error() & -> E& { return std::get<1>( m_storage ); }

        /**
         * @brief Accesses the stored error value.
         * @return Const lvalue reference to the error.
         * @throws std::bad_variant_access if the result contains a success value.
         */
        // NOLINTNEXTLINE(readability-identifier-naming)
        [[nodiscard]] auto error() const& -> const E& { return std::get<1>( m_storage ); }

        /**
         * @brief Moves the stored error value out of an rvalue Result.
         * @return Rvalue reference to the error.
         * @throws std::bad_variant_access if the result contains a success value.
         */
        // NOLINTNEXTLINE(readability-identifier-naming)
        [[nodiscard]] auto error() && -> E&& { return std::get<1>( std::move( m_storage ) ); }

        /** @return Mutable reference to the success value. */
        [[nodiscard]] auto operator*() & -> T& { return value(); }

        /** @return Const reference to the success value. */
        [[nodiscard]] auto operator*() const& -> const T& { return value(); }

        /** @return Rvalue reference to the success value. */
        [[nodiscard]] auto operator*() && -> T&& { return std::move( *this ).value(); }

        /** @return Pointer to the success value. */
        [[nodiscard]] auto operator->() -> T* { return &value(); }

        /** @return Const pointer to the success value. */
        [[nodiscard]] auto operator->() const -> const T* { return &value(); }

    private:
        /** Active success/error storage. Index 0 is success and index 1 is failure. */
        std::variant<T, E> m_storage;
    };

    /**
     * @brief `void` specialization representing success with no payload or an error of type @p E.
     * @tparam E Error value type.
     */
    template <typename E>
    class [[nodiscard]] Result<void, E> final {
    public:
        /** @brief Constructs a successful status. */
        Result() = default;

        /**
         * @brief Constructs a failed status from a failure wrapper.
         * @details Not `explicit`: lets Make_Failure(...) be returned directly from a function
         * returning Status<E>.
         */
        // NOLINTNEXTLINE(google-explicit-constructor)
        Result( Failure<E> failed ) : m_error( std::move( failed.error ) ) {}

        /**
         * @brief Constructs a failed status by copying @p error.
         * @details The Failure_Tag parameter is intentionally unnamed; it exists only to
         * disambiguate this overload via tag dispatch.
         */
        // NOLINTNEXTLINE(readability-named-parameter)
        Result( Failure_Tag, const E& error ) : m_error( error ) {}

        /** @brief Constructs a failed status by moving @p error. See copying overload above. */
        // NOLINTNEXTLINE(readability-named-parameter)
        Result( Failure_Tag, E&& error ) : m_error( std::move( error ) ) {}

        Result( const Result& ) = default;
        Result( Result&& ) noexcept( std::is_nothrow_move_constructible_v<std::optional<E>> ) = default;
        auto operator=( const Result& ) -> Result& = default;
        auto operator=( Result&& ) noexcept( std::is_nothrow_move_assignable_v<std::optional<E>> ) -> Result& = default;
        ~Result() = default;

        /**
         * @return `true` when the operation succeeded.
         * @details lower_case mirrors std::optional's has_value(), the documented public API
         * surface for this type.
         */
        // NOLINTNEXTLINE(readability-identifier-naming)
        [[nodiscard]] auto has_value() const noexcept -> bool { return !m_error.has_value(); }

        /** @return `true` when the operation succeeded. */
        [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

        /**
         * @brief Accesses the stored error.
         * @return Mutable reference to the error.
         * @throws std::bad_optional_access if the result represents success.
         * @details lower_case mirrors std::expected::error(); throwing on the wrong state is the
         * documented, intentional contract, so the caller is expected to check
         * has_value()/operator bool() first rather than have this accessor branch defensively.
         */
        // NOLINTNEXTLINE(readability-identifier-naming, bugprone-unchecked-optional-access)
        [[nodiscard]] auto error() & -> E& { return m_error.value(); }

        /**
         * @brief Accesses the stored error.
         * @return Const reference to the error.
         * @throws std::bad_optional_access if the result represents success.
         */
        // NOLINTNEXTLINE(readability-identifier-naming, bugprone-unchecked-optional-access)
        [[nodiscard]] auto error() const& -> const E& { return m_error.value(); }

        /**
         * @brief Moves the stored error out of an rvalue Result.
         * @return Rvalue reference to the error.
         * @throws std::bad_optional_access if the result represents success.
         */
        // NOLINTNEXTLINE(readability-identifier-naming)
        [[nodiscard]] auto error() && -> E&& { return std::move( m_error ).value(); }

    private:
        /** Empty on success; contains the failure value otherwise. */
        std::optional<E> m_error;
    };

    /** @brief Convenience name for an operation that returns only success or an error. */
    template <typename E>
    using Status = Result<void, E>;

}  // namespace edge
