#include <cfrds.hpp>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <memory>
#include <print>

static int s_custom_free_count = 0;
static void dummy_free(cfrds_server *s) {
    if (s) {
        s_custom_free_count++;
    }
}

int main() {
    // 1. Test server factory and basics
    {
        try {
            auto server = cfrds::server("127.0.0.1", 8500, "admin", "secret");
            assert(server.port() == 8500);
        } catch (const cfrds::cfrds_exception& e) {
            // Expected failure if server not present
            std::println("Caught server init error: {} (status: {})", e.what(), static_cast<int>(e.status()));
            return EXIT_FAILURE;
        }
    }

    // 2. Test initialization and API using OO syntax
    {
        try {
            auto server = cfrds::server("127.0.0.1", 8500, "admin", "secret");

            // Server accessors
            assert(server.port() == 8500);

            // Test other C++ API wrappers (smoke test: in unit testing without a live RDS server, network calls throw cfrds_exception)
            try {
                auto dir = server.browse_dir("/");
                (void)dir;
            } catch (const cfrds::cfrds_exception& e) {
                // Expected when no live CF server is running
                assert(e.status() != CFRDS_STATUS_OK);
            }

            try {
                auto fc = server.read_file("test.cfm");
                (void)fc;
            } catch (const cfrds::cfrds_exception& e) {
                // Expected when no live CF server is running
                assert(e.status() != CFRDS_STATUS_OK);
            }

            try {
                auto rs = server.execute_sql("default", "SELECT 1");
                (void)rs;
            } catch (const cfrds::cfrds_exception& e) {
                // Expected when no live CF server is running
                assert(e.status() != CFRDS_STATUS_OK);
            }
        } catch (const cfrds::cfrds_exception& e) {
            std::println("Caught server init error: {} (status: {})", e.what(), static_cast<int>(e.status()));
            return EXIT_FAILURE;
        }
    }

    // 3. Test move semantics
    {
        try {
            auto s1 = cfrds::server("localhost", 8500, "", "");

            auto s2 = std::move(s1);
            assert(s1.get() == nullptr);
            assert(s2.get() != nullptr);
        } catch (const cfrds::cfrds_exception& e) {
            std::println("Caught server init error: {} (status: {})", e.what(), static_cast<int>(e.status()));
            return EXIT_FAILURE;
        }
    }

    // 4. Test RAII lifetime destruction
    {
        try {
            auto s = cfrds::server("localhost", 8500, "", "");
            (void)s;
        } catch (const cfrds::cfrds_exception& e) {
            std::println("Caught server init error: {} (status: {})", e.what(), static_cast<int>(e.status()));
            return EXIT_FAILURE;
        }
    }

    // 5. Test custom free function execution
    {
        s_custom_free_count = 0;
        {
            cfrds_server *fake = reinterpret_cast<cfrds_server*>(static_cast<uintptr_t>(0x1234));
            std::unique_ptr<cfrds_server, void(*)(cfrds_server*)> custom(fake, dummy_free);
            assert(custom.get() == fake);
        }
        assert(s_custom_free_count == 1);
    }

    std::println("All C++ cfrds.hpp tests passed!");
    return 0;
}
