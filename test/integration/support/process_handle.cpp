/**
 * @file process_handle.cpp
 * @brief Test implementation for process handle behavior and regression coverage.
 *
 * @details Source path: `test/integration/support/process_handle.cpp`.
 */

#include "process_handle.h"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>

namespace edge::integration_test::support {
    namespace {

        // Bounds the graceful SIGTERM phase before escalation to SIGKILL. The later last-resort
        // waitpid() is intentionally allowed to block under pathological kernel/process states so a
        // normally killable child is reaped rather than left as a zombie.
        constexpr auto TERMINATE_GRACE_PERIOD = std::chrono::milliseconds( 2000 );
        // Bounded final reap window after SIGKILL; a killed process should exit essentially
        // instantly, so this only guards against extreme scheduler delay.
        constexpr auto KILL_REAP_TIMEOUT = std::chrono::milliseconds( 2000 );
        constexpr auto POLL_BACKOFF = std::chrono::milliseconds( 10 );
        /** Pipe read buffer size for draining captured child stdout/stderr output. */
        constexpr std::size_t PIPE_READ_BUFFER_SIZE = 4096;
        /** Conventional shell exit code meaning "found but could not be executed" (used here when
         * dup2() fails in the child before execve() is even attempted). */
        constexpr int EXIT_CODE_CHILD_SETUP_FAILED = 126;
        /** Conventional shell exit code meaning "command not found / exec failed". */
        constexpr int EXIT_CODE_EXEC_FAILED = 127;

    }  // namespace

    /** @copydoc Process_Handle::Process_Handle */
    Process_Handle::Process_Handle( const std::string& binary_path,
                                    const std::vector<Environment_Variable>& environment,
                                    std::string label )
        : m_label( std::move( label ) ) {
        // O_CLOEXEC matters because tests construct several `Process_Handle`s in sequence: without
        // it, each new child's `execve()` would inherit every earlier handle's still-open pipe read
        // end, leaking descriptors into the production binaries under test. The write end is
        // `dup2()`'d onto the child's stdout/stderr below, and `dup2()` clears FD_CLOEXEC on the new
        // descriptor it creates, so the child still gets working stdio without any extra `fcntl`.
        std::array<int, 2> pipe_fds { -1, -1 };
        if( ::pipe2( pipe_fds.data(), O_CLOEXEC ) != 0 ) {
            throw std::runtime_error( "Process_Handle: pipe2() failed for '" + m_label + "'" );
        }
        edge::platform::posix::Unique_Fd read_fd { pipe_fds[0] };
        edge::platform::posix::Unique_Fd write_fd { pipe_fds[1] };

        const int existing_flags = ::fcntl( read_fd.Get(), F_GETFL, 0 );  // NOLINT(cppcoreguidelines-pro-type-vararg)
        if( existing_flags < 0 ) {
            throw std::runtime_error( "Process_Handle: fcntl(F_GETFL) failed for '" + m_label +
                                      "': " + std::strerror( errno ) );
        }
        if( ::fcntl( read_fd.Get(), F_SETFL, existing_flags | O_NONBLOCK ) !=
            0 ) {  // NOLINT(cppcoreguidelines-pro-type-vararg)
            throw std::runtime_error( "Process_Handle: fcntl(F_SETFL) failed for '" + m_label +
                                      "': " + std::strerror( errno ) );
        }

        // Build the child's environment/argv as stable storage before fork: inherit the test
        // process environment so dynamic-loader/sanitizer/toolchain settings remain available, then
        // replace only the explicitly supplied variables. No C++ allocation occurs in the child.
        std::vector<std::string> env_storage;
        if( environ != nullptr ) {
            // POSIX `environ` is a plain null-terminated C array; pointer-walking it is the standard,
            // unavoidable idiom for enumerating the process environment.
            for( char** entry = environ; *entry != nullptr;
                 ++entry ) {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                env_storage.emplace_back( *entry );
            }
        }

        for( const auto& variable : environment ) {
            if( variable.name.empty() || variable.name.find( '=' ) != std::string::npos ) {
                throw std::runtime_error( "Process_Handle: invalid environment variable name for '" + m_label + "'" );
            }

            const std::string prefix = variable.name + "=";
            const auto existing =
                std::find_if( env_storage.begin(), env_storage.end(), [&]( const std::string& entry ) {
                    return entry.rfind( prefix, 0 ) == 0;
                } );
            const std::string replacement = prefix + variable.value;
            if( existing != env_storage.end() ) {
                *existing = replacement;
            } else {
                env_storage.push_back( replacement );
            }
        }

        std::vector<char*> envp;
        envp.reserve( env_storage.size() + 1 );
        for( auto& entry : env_storage ) {
            envp.push_back( entry.data() );
        }
        envp.push_back( nullptr );

        std::vector<char*> argv {
            const_cast<char*>( binary_path.c_str() ),  // NOLINT(cppcoreguidelines-pro-type-const-cast)
            nullptr };

        const pid_t pid = ::fork();
        if( pid < 0 ) {
            throw std::runtime_error( "Process_Handle: fork() failed for '" + m_label + "'" );
        }

        if( pid == 0 ) {
            // Child process: only async-signal-safe calls are permitted between fork() and
            // execve(). A failed execve() below falls through to `_exit()`, never `exit()` or any
            // C++ destructor/unwind.
            const int read_raw_fd = read_fd.Get();
            const int write_raw_fd = write_fd.Get();
            if( ::dup2( write_raw_fd, STDOUT_FILENO ) < 0 || ::dup2( write_raw_fd, STDERR_FILENO ) < 0 ) {
                _exit( EXIT_CODE_CHILD_SETUP_FAILED );
            }
            ::close( read_raw_fd );
            ::close( write_raw_fd );
            ::execve( binary_path.c_str(), argv.data(), envp.data() );
            _exit( EXIT_CODE_EXEC_FAILED );
        }

        write_fd.Reset();
        m_pid = pid;
        m_stdout_read_fd = std::move( read_fd );
        m_reaped = false;
    }

    /** @copydoc Process_Handle::~Process_Handle */
    Process_Handle::~Process_Handle() {
        if( !m_reaped ) {
            Terminate();
        }
    }

    /** @copydoc Process_Handle::Process_Handle */
    Process_Handle::Process_Handle( Process_Handle&& other ) noexcept
        : m_pid( other.m_pid ),
          m_stdout_read_fd( std::move( other.m_stdout_read_fd ) ),
          m_label( std::move( other.m_label ) ),
          m_captured_output( std::move( other.m_captured_output ) ),
          m_reaped( other.m_reaped ) {
        other.m_pid = -1;
        other.m_reaped = true;
    }

    /** @copydoc Process_Handle::operator= */
    auto Process_Handle::operator=( Process_Handle&& other ) noexcept -> Process_Handle& {
        if( this != &other ) {
            if( !m_reaped ) {
                Terminate();
            }
            m_pid = other.m_pid;
            m_stdout_read_fd = std::move( other.m_stdout_read_fd );
            m_label = std::move( other.m_label );
            m_captured_output = std::move( other.m_captured_output );
            m_reaped = other.m_reaped;

            other.m_pid = -1;
            other.m_reaped = true;
        }
        return *this;
    }

    /** @copydoc Process_Handle::Pump_Pipe */
    void Process_Handle::Pump_Pipe() {
        if( !m_stdout_read_fd ) {
            return;
        }

        std::array<char, PIPE_READ_BUFFER_SIZE> buffer {};
        for( ;; ) {
            const ssize_t bytes_read = ::read( m_stdout_read_fd.Get(), buffer.data(), buffer.size() );
            if( bytes_read > 0 ) {
                m_captured_output.append( buffer.data(), static_cast<std::size_t>( bytes_read ) );
                continue;
            }
            if( bytes_read == 0 ) {
                // EOF: the write end (the child, or its exec'd image) has closed.
                break;
            }
            if( errno == EINTR ) {
                continue;
            }
            // EAGAIN/EWOULDBLOCK (no data currently available) or another error: stop for now.
            break;
        }
    }

    /** @copydoc Process_Handle::Is_Alive */
    auto Process_Handle::Is_Alive() -> bool {
        if( m_reaped ) {
            return false;
        }

        Pump_Pipe();

        for( ;; ) {
            int status = 0;
            const pid_t result = ::waitpid( m_pid, &status, WNOHANG );
            if( result == 0 ) {
                return true;
            }
            if( result == m_pid ) {
                m_reaped = true;
                return false;
            }
            if( result < 0 && errno == EINTR ) {
                continue;
            }
            if( result < 0 && errno == ECHILD ) {
                m_reaped = true;
                return false;
            }

            // An unexpected waitpid error does not prove the child is dead. Keep ownership so a
            // later teardown pass can still terminate/reap it instead of silently orphaning it.
            return true;
        }
    }

    /** @copydoc Process_Handle::Reap_Blocking */
    void Process_Handle::Reap_Blocking() {
        if( m_reaped ) {
            return;
        }

        const auto deadline = std::chrono::steady_clock::now() + KILL_REAP_TIMEOUT;
        while( std::chrono::steady_clock::now() < deadline ) {
            int status = 0;
            const pid_t result = ::waitpid( m_pid, &status, WNOHANG );
            if( result == m_pid || ( result < 0 && errno == ECHILD ) ) {
                m_reaped = true;
                return;
            }
            if( result < 0 && errno != EINTR ) {
                // Unexpected waitpid failure: retain ownership and fall through to the final blocking
                // reap attempt rather than incorrectly claiming the child was reaped.
                break;
            }
            Pump_Pipe();
            std::this_thread::sleep_for( POLL_BACKOFF );
        }

        // Last-resort blocking wait: after SIGKILL a normal process is expected to become waitable
        // promptly. This favors reaping over a hard teardown deadline; pathological uninterruptible
        // kernel states can still delay this call, so do not describe destructor teardown as strictly
        // bounded.
        for( ;; ) {
            const pid_t result = ::waitpid( m_pid, nullptr, 0 );
            if( result == m_pid || ( result < 0 && errno == ECHILD ) ) {
                m_reaped = true;
                return;
            }
            if( result < 0 && errno == EINTR ) {
                continue;
            }

            // There is no safe recovery path for an unexpected waitpid error in a destructor-side
            // test helper. Keep the handle marked unreaped instead of making a false guarantee.
            return;
        }
    }

    /** @copydoc Process_Handle::Terminate */
    void Process_Handle::Terminate() {
        if( m_reaped ) {
            return;
        }

        ::kill( m_pid, SIGTERM );

        const auto deadline = std::chrono::steady_clock::now() + TERMINATE_GRACE_PERIOD;
        while( Is_Alive() && std::chrono::steady_clock::now() < deadline ) {
            std::this_thread::sleep_for( POLL_BACKOFF );
        }

        if( !m_reaped ) {
            ::kill( m_pid, SIGKILL );
            Reap_Blocking();
        }
    }

    /** @copydoc Process_Handle::Kill */
    void Process_Handle::Kill() {
        if( m_reaped ) {
            return;
        }

        ::kill( m_pid, SIGKILL );
        Reap_Blocking();
    }

    /** @copydoc Process_Handle::Wait_For_Exit */
    auto Process_Handle::Wait_For_Exit( std::chrono::steady_clock::time_point deadline ) -> bool {
        while( Is_Alive() ) {
            if( std::chrono::steady_clock::now() >= deadline ) {
                return false;
            }
            std::this_thread::sleep_for( POLL_BACKOFF );
        }
        return true;
    }

    /** @copydoc Process_Handle::Drain_Output */
    auto Process_Handle::Drain_Output() -> const std::string& {
        Pump_Pipe();
        return m_captured_output;
    }

    /** @copydoc Process_Handle::Pid */
    auto Process_Handle::Pid() const noexcept -> pid_t { return m_pid; }

    /** @copydoc Process_Handle::Label */
    auto Process_Handle::Label() const noexcept -> const std::string& { return m_label; }

}  // namespace edge::integration_test::support
