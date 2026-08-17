#include <cfrds.hpp>
#include <cassert>
#include <iostream>
#include <utility>

static int s_custom_free_count = 0;
static void dummy_free(cfrds_server *s) {
    if (s) {
        s_custom_free_count++;
    }
}

int main() {
    // 1. Test default construction for all RAII wrappers
    {
        cfrds::Server s;
        assert(s.get() == nullptr);
        assert(!s);
        assert(static_cast<cfrds_server*>(s) == nullptr);

        cfrds::BrowseDir dir;
        assert(dir.get() == nullptr);
        assert(!dir);

        cfrds::FileContent fc;
        assert(fc.get() == nullptr);

        cfrds::SqlDsnInfo dsn;
        assert(dsn.get() == nullptr);

        cfrds::SqlTableInfo tbl;
        assert(tbl.get() == nullptr);

        cfrds::SqlColumnInfo col;
        assert(col.get() == nullptr);

        cfrds::SqlPrimaryKeys pk;
        assert(pk.get() == nullptr);

        cfrds::SqlForeignKeys fk;
        assert(fk.get() == nullptr);

        cfrds::SqlImportedKeys ik;
        assert(ik.get() == nullptr);

        cfrds::SqlExportedKeys ek;
        assert(ek.get() == nullptr);

        cfrds::SqlResultSet rs;
        assert(rs.get() == nullptr);

        cfrds::SqlMetadata meta;
        assert(meta.get() == nullptr);

        cfrds::SqlSupportedCommands sc;
        assert(sc.get() == nullptr);

        cfrds::DebuggerEvent dbg;
        assert(dbg.get() == nullptr);

        cfrds::SecurityAnalyzerResult sec;
        assert(sec.get() == nullptr);

        cfrds::AdminApiCustomTagPaths tag;
        assert(tag.get() == nullptr);

        cfrds::AdminApiMappings map;
        assert(map.get() == nullptr);
    }

    // 2. Test initialization using &object (operator&) and implicit conversion
    {
        cfrds::Server server;
        bool ok = cfrds_server_init(&server, "127.0.0.1", 8500, "admin", "secret");
        assert(ok);
        assert(server.get() != nullptr);
        assert(server);

        // Server can be passed directly as `server` (implicit conversion to cfrds_server*)
        assert(cfrds_server_get_port(server) == 8500);

        // BrowseDir can be passed directly as `&dir` for out-parameters
        cfrds::BrowseDir dir;
        cfrds_browse_dir **dir_out = &dir;
        assert(*dir_out == nullptr);

        // FileContent type compatibility with &fc
        cfrds::FileContent fc;
        cfrds_file_content **fc_out = &fc;
        assert(*fc_out == nullptr);
    }

    // 3. Test move semantics
    {
        cfrds::Server s1;
        bool ok = cfrds_server_init(&s1, "localhost", 8500, "", "");
        assert(ok);
        cfrds_server *raw = s1.get();

        // Move constructor
        cfrds::Server s2 = std::move(s1);
        assert(s1.get() == nullptr);
        assert(!s1);
        assert(s2.get() == raw);
        assert(s2);

        // Move assignment
        cfrds::Server s3;
        s3 = std::move(s2);
        assert(s2.get() == nullptr);
        assert(!s2);
        assert(s3.get() == raw);
        assert(s3);
    }

    // 4. Test release and reset
    {
        cfrds::Server s;
        bool ok = cfrds_server_init(&s, "localhost", 8500, "", "");
        assert(ok);
        cfrds_server *raw = s.release();
        assert(s.get() == nullptr);
        assert(raw != nullptr);

        s.reset(raw);
        assert(s.get() == raw);
    }

    // 5. Test custom free function execution
    {
        s_custom_free_count = 0;
        {
            cfrds_server *fake = reinterpret_cast<cfrds_server*>(static_cast<uintptr_t>(0x1234));
            cfrds::AutoFree<cfrds_server, dummy_free> custom(fake);
            assert(custom.get() == fake);
        }
        assert(s_custom_free_count == 1);
    }

    std::cout << "All C++ cfrds.hpp tests passed!\n";
    return 0;
}
