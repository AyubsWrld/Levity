/**
 * @file deterministic_targets.h
 * @brief Single source of truth for the fixed telemetry set emitted by the dummy monolith.
 */
#pragma once

#include <array>
#include <string_view>

namespace edge::integration_test::support {

    /** @brief Expected fields for one deterministic dummy-monolith target. */
    struct Deterministic_Target {
        /** Stable target UID. */
        std::string_view uid;
        /** Expected latitude in degrees. */
        double latitude_deg;
        /** Expected longitude in degrees. */
        double longitude_deg;
        /** Expected altitude in metres. */
        double altitude_m;
    };

    /**
     * @brief Exact target set/order expected from the Tier-1 dummy telemetry publisher.
     *
     * @details Both readiness polling and end-to-end assertions use this table to avoid duplicating
     * magic coordinates or accidentally accepting reordered/changed deterministic fixtures.
     */
    inline constexpr std::array<Deterministic_Target, 3> DETERMINISTIC_TARGETS { {
        { "drone_01", 34.05, -118.25, 150.0 },
        { "drone_02", 34.06, -118.26, 220.5 },
        { "ground_01", 34.04, -118.24, 12.0 },
    } };

}  // namespace edge::integration_test::support
