#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ts {

    // abs class that all predicates inherit from.
    class [[nodiscard]] Predicate {
    public:
        Predicate() = default;
        virtual ~Predicate() = default;

        virtual auto PredicateBody() -> void = 0;

        auto Run() -> void;
    };

    // abs factory interface that creates instances of a predicate object.
    class [[nodiscard]] PredicateFactoryBase {
    public:
        virtual ~PredicateFactoryBase() = default;
        virtual auto Create() -> Predicate* = 0;
    };

    template <typename PredicateClass>
    class [[nodiscard]] PredicateFactoryImpl : public PredicateFactoryBase {
    public:
        auto Create() -> Predicate* override { return new PredicateClass; }
    };

    // holds the metadata + factory needed to construct and run a single
    // predicate.
    class [[nodiscard]] PredicateInfo {
    public:
        PredicateInfo( std::string suite_name, std::string name, std::unique_ptr<PredicateFactoryBase> factory )
            : suite_name_( std::move( suite_name ) ), name_( std::move( name ) ), factory_( std::move( factory ) ) {}

        PredicateInfo( const PredicateInfo& ) = delete;
        auto operator=( const PredicateInfo& ) -> PredicateInfo& = delete;

        auto Run() -> void {
            // owns the instance for the duration of the call so it's cleaned up
            // regardless of what PredicateBody() does.
            std::unique_ptr<Predicate> predicate( factory_->Create() );
            predicate->PredicateBody();
        }

    private:
        std::string suite_name_;
        std::string name_;
        std::unique_ptr<PredicateFactoryBase> factory_;
    };

    // a named collection of PredicateInfo objects sharing a suite name.
    class [[nodiscard]] PredicateSuite {
    public:
        explicit PredicateSuite( std::string name ) : name_( std::move( name ) ) {}

        auto Run() -> void {
            for( auto& predicate_info : predicate_info_list_ ) {
                predicate_info->Run();
            }
        }

        auto name() const -> const std::string& { return name_; }

    private:
        friend class Registry;

        std::string name_;
        std::vector<std::shared_ptr<PredicateInfo>> predicate_info_list_;
    };

    // global registry of all predicate suites, keyed by suite name.
    class [[nodiscard]] Registry {
    public:
        static auto GetInstance() -> Registry& { // to combat https://en.cppreference.com/cpp/language/siof
            static Registry reg;
            return reg;
        }

        auto RegisterPredicate( std::string suite_name,
                                std::string name,
                                std::unique_ptr<PredicateFactoryBase> factory ) -> std::shared_ptr<PredicateInfo> {
            auto predicate_info = std::make_shared<PredicateInfo>( suite_name, name, std::move( factory ) );

            auto found =
                std::find_if( suites_.begin(), suites_.end(), [&]( const std::shared_ptr<PredicateSuite>& suite ) {
                    return suite_name == suite->name();
                } );

            if( found != suites_.end() ) {
                ( *found )->predicate_info_list_.push_back( predicate_info );
            } else {
                auto suite = std::make_shared<PredicateSuite>( suite_name );
                suite->predicate_info_list_.push_back( predicate_info );
                suites_.push_back( std::move( suite ) );
            }

            return predicate_info;
        }

        auto RunAll() -> void {
            for( auto& suite : suites_ ) {
                suite->Run();
            }
        }

    private:
        Registry() = default;

        std::vector<std::shared_ptr<PredicateSuite>> suites_;
    };

}  // namespace ts

#define TS_PREDICATE_NAME( library, name ) library##_##name##_predicate
#define TS_STRINGIZE( x ) #x

// clangd emits a spurious warning for sizeof(TS_STRINGIZE(x)) used directly
// at namespace scope; i don't quite know why ...
#define TS_DECL_PREDICATE( library, name )                                                                             \
    static_assert( sizeof( TS_STRINGIZE( library ) ) > 0, "library must have non-zero length" );                       \
    static_assert( sizeof( TS_STRINGIZE( name ) ) > 0, "name must have non-zero length" );                             \
    class [[nodiscard]] TS_PREDICATE_NAME( library, name ) : public ts::Predicate {                                    \
    public:                                                                                                            \
        TS_PREDICATE_NAME( library, name )() = default;                                                                \
        TS_PREDICATE_NAME( library, name )( const TS_PREDICATE_NAME( library, name ) & ) = delete;                     \
        TS_PREDICATE_NAME( library, name )( TS_PREDICATE_NAME( library, name ) && ) = delete;                          \
        auto operator=( const TS_PREDICATE_NAME( library, name ) & ) -> TS_PREDICATE_NAME( library, name ) & = delete; \
        auto operator=( TS_PREDICATE_NAME( library, name ) && ) -> TS_PREDICATE_NAME( library, name ) & = delete;      \
                                                                                                                       \
        auto PredicateBody() -> void override;                                                                         \
                                                                                                                       \
    private:                                                                                                           \
        [[maybe_unused]] static std::shared_ptr<ts::PredicateInfo> predicate_info_;                                    \
    };                                                                                                                 \
    std::shared_ptr<ts::PredicateInfo> TS_PREDICATE_NAME( library, name )::predicate_info_ =                           \
        ts::Registry::GetInstance().RegisterPredicate(                                                                 \
            TS_STRINGIZE( library ),                                                                                   \
            TS_STRINGIZE( name ),                                                                                      \
            std::make_unique<ts::PredicateFactoryImpl<TS_PREDICATE_NAME( library, name )>>() );                        \
    auto TS_PREDICATE_NAME( library, name )::PredicateBody() -> void

TS_DECL_PREDICATE( LGPL, IsStaticallyLinked ) { std::cout << "Testing" << std::endl; }
