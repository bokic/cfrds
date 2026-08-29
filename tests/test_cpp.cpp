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
    // 0. Test version functions
    {
        assert(cfrds_version() != nullptr);
        assert(std::string(cfrds_version()) == CFRDS_VERSION);
        assert(cfrds_version_major() == CFRDS_VERSION_MAJOR);
        assert(cfrds_version_minor() == CFRDS_VERSION_MINOR);
        assert(cfrds_version_patch() == CFRDS_VERSION_PATCH);
        assert(cfrds_version_int() == CFRDS_VERSION_INT);
    }

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
            // File operations
            try { (void)server.browse_dir("/"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.read_file("test.cfm"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.write_file("test.cfm", "content"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.rename_file("a.cfm", "b.cfm"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.remove_file("test.cfm"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.remove_dir("/tmp/test"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.file_exists("test.cfm"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.create_dir("/tmp/newdir"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.get_root_dir(); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }

            // SQL operations
            try { (void)server.execute_sql("default", "SELECT 1"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.sql_dsninfo(); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.sql_tableinfo("artgallery"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.sql_columninfo("artgallery", "ARTISTS"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.sql_primarykeys("artgallery", "ARTISTS"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.sql_foreignkeys("artgallery", "ARTISTS"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.sql_importedkeys("artgallery", "ARTISTS"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.sql_exportedkeys("artgallery", "ARTISTS"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.sql_metadata("artgallery", "SELECT * FROM ARTISTS"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.sql_supportedcommands(); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.sql_dbdescription("artgallery"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }

            // Debugger operations
            try { (void)server.debugger_start(); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_stop("sess1"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_server_stop("sess1"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.debugger_get_server_info("sess1"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_breakpoint_on_exception("sess1", true); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_global_breakpoint_on_exception("sess1", true); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_breakpoint("sess1", "app.cfm", 10, true); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_clear_all_breakpoints("sess1"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.debugger_get_debug_events("sess1"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.debugger_all_fetch_flags_enabled("sess1", true, true, true, true, true); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_step_in("sess1", "main"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_step_over("sess1", "main"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_step_out("sess1", "main"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.debugger_sync_step_in("sess1", "main"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.debugger_sync_step_over("sess1", "main"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.debugger_sync_step_out("sess1", "main"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_continue("sess1", "main"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.debugger_get_cf_variables("sess1", "main"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_watch_expression("sess1", "main", "1+1"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_set_variable("sess1", "main", "x", "1"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_watch_variables("sess1", "x,y"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.debugger_get_output("sess1", "main"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.debugger_set_scope_filter("sess1", "filter"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }

            // Security analyzer operations
            try { (void)server.security_analyzer_scan("/var/www"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.security_analyzer_cancel(1); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.security_analyzer_status(1); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.security_analyzer_result(1); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.security_analyzer_clean(1); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }

            // IDE Default
            try { (void)server.ide_default(1); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }

            // AdminAPI
            try { (void)server.adminapi_debugging_getlogproperty("logdir"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.adminapi_extensions_getcustomtagpaths(); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.adminapi_extensions_setmapping("/app", "/var/www/app"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { server.adminapi_extensions_deletemapping("/app"); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
            try { (void)server.adminapi_extensions_getmappings(); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }

            // Graphing
            try { (void)server.graphing("<chart/>", {"series1"}); } catch (const cfrds::cfrds_exception& e) { assert(e.status() != CFRDS_STATUS_OK); }
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
