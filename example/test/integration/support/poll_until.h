/**
 * @file poll_until.h
 * @brief Bounded polling primitive used for observable integration-test readiness conditions.
 */
#pragma once

#include <chrono>
#include <thread>

namespace edge::integration_test::support {

    /**
     * @brief Repeatedly evaluates a predicate until success or a hard deadline.
     * @tparam Predicate Callable compatible with `bool()`.
     * @param deadline Absolute steady-clock deadline after which the wait fails.
     * @param backoff Delay between unsuccessful attempts.
     * @param predicate Observable readiness/condition check.
     * @return true as soon as the predicate succeeds; false after the deadline.
     *
     * @details The predicate is evaluated at least once, even when the deadline is already past. The
     * deadline is checked between predicate calls; a predicate that performs its own bounded I/O may
     * itself run past the outer deadline. Integration tests use this instead of blind fixed sleeps so
     * readiness remains observable and bounded by the combination of predicate and polling budgets.
     */
    template <typename Predicate>
    [[nodiscard]] auto Poll_Until( std::chrono::steady_clock::time_point deadline,
                                   std::chrono::milliseconds backoff,
                                   Predicate&& predicate ) -> bool {
        for( ;; ) {
            if( predicate() ) {
                return true;
            }
            if( std::chrono::steady_clock::now() >= deadline ) {
                return false;
            }
            std::this_thread::sleep_for( backoff );
        }
    }

}  // namespace edge::integration_test::support
