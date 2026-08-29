#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

extern "C" {
#include "cfrds.h"
}

namespace cfrds {

// ------------------------------------------------------------------------------------------------
// Typedef Aliases for RAII Managed Pointers
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// C++ OO API
// ------------------------------------------------------------------------------------------------
    /**
     * @brief Exception thrown by the cfrds C++ API on failure.
     */
    class cfrds_exception : public std::runtime_error {
    public:
        cfrds_exception(cfrds_status status, std::string message) 
            : std::runtime_error(std::move(message)), _status(status) {}

        cfrds_status status() const noexcept { return _status; }

    private:
        cfrds_status _status;
    };

    // ------------------------------------------------------------------------------------------------
    // C++ OO API
    // ------------------------------------------------------------------------------------------------

    class BrowseDir {
    public:
        struct Item {
            std::string_view name;
            char kind;
        };

        class Iterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = Item;
            using difference_type = std::ptrdiff_t;
            using pointer = void;
            using reference = Item;

            Iterator() noexcept = default;
            Iterator(const BrowseDir* parent, size_t index) noexcept
                : _parent(parent), _index(index) {}

            Item operator*() const noexcept {
                return _parent ? _parent->item(_index) : Item{};
            }

            Iterator& operator++() noexcept { ++_index; return *this; }
            Iterator operator++(int) noexcept { Iterator tmp = *this; ++_index; return tmp; }
            Iterator& operator--() noexcept { --_index; return *this; }
            Iterator operator--(int) noexcept { Iterator tmp = *this; --_index; return tmp; }

            Iterator& operator+=(difference_type n) noexcept { _index += n; return *this; }
            Iterator& operator-=(difference_type n) noexcept { _index -= n; return *this; }
            friend Iterator operator+(Iterator it, difference_type n) noexcept { it += n; return it; }
            friend Iterator operator+(difference_type n, Iterator it) noexcept { it += n; return it; }
            friend Iterator operator-(Iterator it, difference_type n) noexcept { it -= n; return it; }
            friend difference_type operator-(const Iterator& a, const Iterator& b) noexcept {
                return static_cast<difference_type>(a._index) - static_cast<difference_type>(b._index);
            }

            Item operator[](difference_type n) const noexcept { return *(*this + n); }

            friend bool operator==(const Iterator& a, const Iterator& b) noexcept {
                return a._parent == b._parent && a._index == b._index;
            }
            friend auto operator<=>(const Iterator& a, const Iterator& b) noexcept {
                return a._index <=> b._index;
            }

        private:
            const BrowseDir* _parent{nullptr};
            size_t _index{0};
        };

        explicit BrowseDir(::cfrds_browse_dir* ptr) noexcept : _ptr(ptr) {}
        ~BrowseDir() { if (_ptr) ::cfrds_browse_dir_free(_ptr); }
        BrowseDir(const BrowseDir&) = delete;
        BrowseDir& operator=(const BrowseDir&) = delete;
        BrowseDir(BrowseDir&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        BrowseDir& operator=(BrowseDir&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_browse_dir_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept { return ::cfrds_browse_dir_count(_ptr); }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        char item_kind(size_t index) const noexcept {
            return ::cfrds_browse_dir_item_get_kind(_ptr, static_cast<int>(index));
        }

        std::string_view item_name(size_t index) const noexcept {
            const char* name = ::cfrds_browse_dir_item_get_name(_ptr, static_cast<int>(index));
            return name ? std::string_view(name) : std::string_view{};
        }

        Item item(size_t index) const noexcept {
            return Item{item_name(index), item_kind(index)};
        }

        Item operator[](size_t index) const noexcept { return item(index); }

        Iterator begin() const noexcept { return Iterator(this, 0); }
        Iterator end() const noexcept { return Iterator(this, count()); }
        Iterator cbegin() const noexcept { return begin(); }
        Iterator cend() const noexcept { return end(); }

        ::cfrds_browse_dir* get() const noexcept { return _ptr; }
    private:
        ::cfrds_browse_dir* _ptr{nullptr};
    };

    class FileContent {
    public:
        explicit FileContent(::cfrds_file_content* ptr) noexcept : _ptr(ptr) {}
        ~FileContent() { if (_ptr) ::cfrds_file_content_free(_ptr); }
        FileContent(const FileContent&) = delete;
        FileContent& operator=(const FileContent&) = delete;
        FileContent(FileContent&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        FileContent& operator=(FileContent&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_file_content_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        std::span<const std::byte> bytes() const noexcept {
            return {
                reinterpret_cast<const std::byte*>(::cfrds_file_content_get_data(_ptr)),
                ::cfrds_file_content_get_size(_ptr)
            };
        }

        const std::byte* data() const noexcept {
            return reinterpret_cast<const std::byte*>(::cfrds_file_content_get_data(_ptr));
        }

        size_t size() const noexcept { return ::cfrds_file_content_get_size(_ptr); }
        bool empty() const noexcept { return size() == 0; }
        ::cfrds_file_content* get() const noexcept { return _ptr; }
    private:
        ::cfrds_file_content* _ptr{nullptr};
    };

    class SqlResultSet {
    public:
        class Row {
        public:
            Row(const SqlResultSet* parent, size_t row_idx) noexcept
                : _parent(parent), _row_idx(row_idx) {}

            size_t col_count() const noexcept {
                return _parent ? _parent->col_count() : 0;
            }
            size_t size() const noexcept { return col_count(); }

            std::string_view column_name(size_t col) const noexcept {
                return _parent ? _parent->column_name(col) : std::string_view{};
            }

            std::string_view get(size_t col) const noexcept {
                return _parent ? _parent->get_value(_row_idx, col) : std::string_view{};
            }

            std::string_view operator[](size_t col) const noexcept {
                return get(col);
            }

        private:
            const SqlResultSet* _parent{nullptr};
            size_t _row_idx{0};
        };

        class Iterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = Row;
            using difference_type = std::ptrdiff_t;
            using pointer = void;
            using reference = Row;

            Iterator() noexcept = default;
            Iterator(const SqlResultSet* parent, size_t row) noexcept
                : _parent(parent), _row(row) {}

            Row operator*() const noexcept { return Row(_parent, _row); }

            Iterator& operator++() noexcept { ++_row; return *this; }
            Iterator operator++(int) noexcept { Iterator tmp = *this; ++_row; return tmp; }
            Iterator& operator--() noexcept { --_row; return *this; }
            Iterator operator--(int) noexcept { Iterator tmp = *this; --_row; return tmp; }

            Iterator& operator+=(difference_type n) noexcept { _row += n; return *this; }
            Iterator& operator-=(difference_type n) noexcept { _row -= n; return *this; }
            friend Iterator operator+(Iterator it, difference_type n) noexcept { it += n; return it; }
            friend Iterator operator+(difference_type n, Iterator it) noexcept { it += n; return it; }
            friend Iterator operator-(Iterator it, difference_type n) noexcept { it -= n; return it; }
            friend difference_type operator-(const Iterator& a, const Iterator& b) noexcept {
                return static_cast<difference_type>(a._row) - static_cast<difference_type>(b._row);
            }

            Row operator[](difference_type n) const noexcept { return *(*this + n); }

            friend bool operator==(const Iterator& a, const Iterator& b) noexcept {
                return a._parent == b._parent && a._row == b._row;
            }
            friend auto operator<=>(const Iterator& a, const Iterator& b) noexcept {
                return a._row <=> b._row;
            }

        private:
            const SqlResultSet* _parent{nullptr};
            size_t _row{0};
        };

        explicit SqlResultSet(::cfrds_sql_resultset* ptr) noexcept : _ptr(ptr) {}
        ~SqlResultSet() { if (_ptr) ::cfrds_sql_resultset_free(_ptr); }
        SqlResultSet(const SqlResultSet&) = delete;
        SqlResultSet& operator=(const SqlResultSet&) = delete;
        SqlResultSet(SqlResultSet&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        SqlResultSet& operator=(SqlResultSet&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_sql_resultset_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t row_count() const noexcept { return ::cfrds_sql_resultset_rows(_ptr); }
        size_t size() const noexcept { return row_count(); }
        bool empty() const noexcept { return row_count() == 0; }
        size_t col_count() const noexcept { return ::cfrds_sql_resultset_columns(_ptr); }

        std::string_view column_name(size_t col) const noexcept {
            const char* name = ::cfrds_sql_resultset_column_name(_ptr, col);
            return name ? std::string_view(name) : std::string_view{};
        }

        std::string_view get_value(size_t row, size_t col) const noexcept {
            const char* val = ::cfrds_sql_resultset_value(_ptr, static_cast<int>(row), static_cast<int>(col));
            return val ? std::string_view(val) : std::string_view{};
        }

        Row row(size_t index) const noexcept { return Row(this, index); }
        Row operator[](size_t index) const noexcept { return row(index); }

        Iterator begin() const noexcept { return Iterator(this, 0); }
        Iterator end() const noexcept { return Iterator(this, row_count()); }
        Iterator cbegin() const noexcept { return begin(); }
        Iterator cend() const noexcept { return end(); }

        ::cfrds_sql_resultset* get() const noexcept { return _ptr; }
    private:
        ::cfrds_sql_resultset* _ptr{nullptr};
    };

    class Server {
    public:
        // Type aliases for convenience and backwards compatibility
        using BrowseDir = cfrds::BrowseDir;
        using FileContent = cfrds::FileContent;
        using SqlResultSet = cfrds::SqlResultSet;

        // Disable copy, allow move
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        
        Server(Server&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        
        Server& operator=(Server&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_server_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        explicit Server(::cfrds_server* ptr) noexcept : _ptr(ptr) {}

        ~Server() { if (_ptr) ::cfrds_server_free(_ptr); }

        // Accessors
        int port() const noexcept { return ::cfrds_server_get_port(_ptr); }
        std::string_view host() const noexcept {
            const char* str = ::cfrds_server_get_host(_ptr);
            return str ? std::string_view(str) : std::string_view{};
        }
        std::string_view username() const noexcept {
            const char* str = ::cfrds_server_get_username(_ptr);
            return str ? std::string_view(str) : std::string_view{};
        }
        std::string_view password() const noexcept {
            const char* str = ::cfrds_server_get_password(_ptr);
            return str ? std::string_view(str) : std::string_view{};
        }
        std::string_view error() const noexcept {
            const char* str = ::cfrds_server_get_error(_ptr);
            return str ? std::string_view(str) : std::string_view{};
        }
        void clear_error() noexcept { ::cfrds_server_clear_error(_ptr); }

        ::cfrds_server* get() const noexcept { return _ptr; }

        // Commands
        BrowseDir browse_dir(std::string_view path) {
            std::string null_term_path(path);
            ::cfrds_browse_dir* raw = nullptr;
            cfrds_status st = ::cfrds_command_browse_dir(_ptr, null_term_path.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during browse_dir");
            }
            return BrowseDir(raw);
        }

        FileContent read_file(std::string_view path) {
            std::string null_term_path(path);
            ::cfrds_file_content* raw = nullptr;
            cfrds_status st = ::cfrds_command_file_read(_ptr, null_term_path.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during read_file");
            }
            return FileContent(raw);
        }

        SqlResultSet execute_sql(std::string_view connection_name, std::string_view sql) {
            std::string null_term_conn(connection_name);
            std::string null_term_sql(sql);
            ::cfrds_sql_resultset* raw = nullptr;
            cfrds_status st = ::cfrds_command_sql_sqlstmnt(_ptr, null_term_conn.c_str(), null_term_sql.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during execute_sql");
            }
            return SqlResultSet(raw);
        }

    private:
        ::cfrds_server* _ptr{nullptr};
    };

    /**
     * @brief Factory function to initialize a ColdFusion server connection.
     * @throws cfrds_exception if initialization fails.
     */
    inline Server server(std::string_view host, int port, std::string_view user, std::string_view pass) {
        std::string null_term_host(host);
        std::string null_term_user(user);
        std::string null_term_pass(pass);
        ::cfrds_server* raw = nullptr;
        bool ok = ::cfrds_server_init(&raw, null_term_host.c_str(), static_cast<uint16_t>(port), null_term_user.c_str(), null_term_pass.c_str());
        if (!ok) {
            throw cfrds_exception(CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "Failed to initialize server connection");
        }
        return Server(raw);
    }

} // namespace cfrds
