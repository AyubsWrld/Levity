/**
 * @file gimbal_controller.h
 * @brief Runtime composition and lifecycle root for the Gimbal Controller executable.
 */
#pragma once

#include <edge/gimbal_controller/runtime/config.h>

#include <memory>

namespace edge::platform::logging {
    class Logger;
}  // namespace edge::platform::logging

namespace edge::gimbal_controller::runtime {

    /**
     * @brief Owns and wires the Gimbal Controller core, inbound command adapter, and output adapter.
     *
     * @details Runtime is intentionally the only production layer aware of both concrete adapters
     * and concrete core services. The core itself depends only on ports. The Logger is non-owning and
     * must outlive this object; all other dependencies are owned by the private implementation.
     */
    class Gimbal_Controller final {
    public:
        /**
         * @brief Constructs and starts the selected controller adapters and core service.
         * @param config Fully resolved startup configuration.
         * @param logger Process logger that must outlive this object.
         * @throws std::exception Failures from configuration-selected adapter/resource initialization.
         */
        Gimbal_Controller( const Gimbal_Controller_Config& config, edge::platform::logging::Logger& logger );

        /** @brief Stops the command server and releases owned runtime resources. */
        ~Gimbal_Controller();

        Gimbal_Controller( const Gimbal_Controller& ) = delete;
        auto operator=( const Gimbal_Controller& ) -> Gimbal_Controller& = delete;
        Gimbal_Controller( Gimbal_Controller&& ) = delete;
        auto operator=( Gimbal_Controller&& ) -> Gimbal_Controller& = delete;

        /** @brief Idempotently stops inbound command processing before destruction. */
        void Stop() noexcept;

    private:
        /** Private concrete dependency graph. */
        struct Impl;
        /** Sole owner of all runtime components. */
        std::unique_ptr<Impl> m_impl;
    };

}  // namespace edge::gimbal_controller::runtime
