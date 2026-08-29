#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

extern "C" {
#include "cfrds.h"
}

namespace cfrds {

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
    // RAII Result Wrappers
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

        uint8_t item_permissions(size_t index) const noexcept {
            return ::cfrds_browse_dir_item_get_permissions(_ptr, static_cast<int>(index));
        }

        size_t item_size(size_t index) const noexcept {
            return ::cfrds_browse_dir_item_get_size(_ptr, static_cast<int>(index));
        }

        uint64_t item_modified(size_t index) const noexcept {
            return ::cfrds_browse_dir_item_get_modified(_ptr, static_cast<int>(index));
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

        std::string_view text() const noexcept {
            const char* d = reinterpret_cast<const char*>(::cfrds_file_content_get_data(_ptr));
            return d ? std::string_view(d, ::cfrds_file_content_get_size(_ptr)) : std::string_view{};
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

    class DsnInfo {
    public:
        struct Item {
            std::string_view name;
        };

        explicit DsnInfo(::cfrds_sql_dsninfo* ptr) noexcept : _ptr(ptr) {}
        ~DsnInfo() { if (_ptr) ::cfrds_sql_dsninfo_free(_ptr); }
        DsnInfo(const DsnInfo&) = delete;
        DsnInfo& operator=(const DsnInfo&) = delete;
        DsnInfo(DsnInfo&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        DsnInfo& operator=(DsnInfo&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_sql_dsninfo_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept { return ::cfrds_sql_dsninfo_count(_ptr); }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        std::string_view name(size_t index) const noexcept {
            const char* s = ::cfrds_sql_dsninfo_item_get_name(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        Item item(size_t index) const noexcept {
            return Item{name(index)};
        }

        Item operator[](size_t index) const noexcept { return item(index); }
        ::cfrds_sql_dsninfo* get() const noexcept { return _ptr; }

    private:
        ::cfrds_sql_dsninfo* _ptr{nullptr};
    };

    class TableInfo {
    public:
        struct Item {
            std::string_view name;
            std::string_view type;
        };

        explicit TableInfo(::cfrds_sql_tableinfo* ptr) noexcept : _ptr(ptr) {}
        ~TableInfo() { if (_ptr) ::cfrds_sql_tableinfo_free(_ptr); }
        TableInfo(const TableInfo&) = delete;
        TableInfo& operator=(const TableInfo&) = delete;
        TableInfo(TableInfo&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        TableInfo& operator=(TableInfo&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_sql_tableinfo_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept { return ::cfrds_sql_tableinfo_count(_ptr); }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        std::string_view name(size_t index) const noexcept {
            const char* s = ::cfrds_sql_tableinfo_get_column_name(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view type(size_t index) const noexcept {
            const char* s = ::cfrds_sql_tableinfo_get_column_type(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        Item item(size_t index) const noexcept {
            return Item{name(index), type(index)};
        }

        Item operator[](size_t index) const noexcept { return item(index); }
        ::cfrds_sql_tableinfo* get() const noexcept { return _ptr; }

    private:
        ::cfrds_sql_tableinfo* _ptr{nullptr};
    };

    class ColumnInfo {
    public:
        struct Item {
            std::string_view name;
            std::string_view type_str;
            int type;
            int precision;
            int scale;
            int radix;
            int nullable;
        };

        explicit ColumnInfo(::cfrds_sql_columninfo* ptr) noexcept : _ptr(ptr) {}
        ~ColumnInfo() { if (_ptr) ::cfrds_sql_columninfo_free(_ptr); }
        ColumnInfo(const ColumnInfo&) = delete;
        ColumnInfo& operator=(const ColumnInfo&) = delete;
        ColumnInfo(ColumnInfo&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        ColumnInfo& operator=(ColumnInfo&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_sql_columninfo_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept { return ::cfrds_sql_columninfo_count(_ptr); }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        std::string_view name(size_t index) const noexcept {
            const char* s = ::cfrds_sql_columninfo_get_name(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view type_str(size_t index) const noexcept {
            const char* s = ::cfrds_sql_columninfo_get_typeStr(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        int type(size_t index) const noexcept {
            return ::cfrds_sql_columninfo_get_type(_ptr, index);
        }

        int precision(size_t index) const noexcept {
            return ::cfrds_sql_columninfo_get_precision(_ptr, index);
        }

        int scale(size_t index) const noexcept {
            return ::cfrds_sql_columninfo_get_scale(_ptr, index);
        }

        int radix(size_t index) const noexcept {
            return ::cfrds_sql_columninfo_get_radix(_ptr, index);
        }

        int nullable(size_t index) const noexcept {
            return ::cfrds_sql_columninfo_get_nullable(_ptr, index);
        }

        Item item(size_t index) const noexcept {
            return Item{name(index), type_str(index), type(index), precision(index), scale(index), radix(index), nullable(index)};
        }

        Item operator[](size_t index) const noexcept { return item(index); }
        ::cfrds_sql_columninfo* get() const noexcept { return _ptr; }

    private:
        ::cfrds_sql_columninfo* _ptr{nullptr};
    };

    class PrimaryKeys {
    public:
        struct Item {
            std::string_view catalog;
            std::string_view owner;
            std::string_view table;
            std::string_view column;
            int key_sequence;
        };

        explicit PrimaryKeys(::cfrds_sql_primarykeys* ptr) noexcept : _ptr(ptr) {}
        ~PrimaryKeys() { if (_ptr) ::cfrds_sql_primarykeys_free(_ptr); }
        PrimaryKeys(const PrimaryKeys&) = delete;
        PrimaryKeys& operator=(const PrimaryKeys&) = delete;
        PrimaryKeys(PrimaryKeys&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        PrimaryKeys& operator=(PrimaryKeys&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_sql_primarykeys_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept { return ::cfrds_sql_primarykeys_count(_ptr); }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        std::string_view catalog(size_t index) const noexcept {
            const char* s = ::cfrds_sql_primarykeys_get_catalog(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view owner(size_t index) const noexcept {
            const char* s = ::cfrds_sql_primarykeys_get_owner(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view table(size_t index) const noexcept {
            const char* s = ::cfrds_sql_primarykeys_get_table(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view column(size_t index) const noexcept {
            const char* s = ::cfrds_sql_primarykeys_get_column(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        int key_sequence(size_t index) const noexcept {
            return ::cfrds_sql_primarykeys_get_key_sequence(_ptr, index);
        }

        Item item(size_t index) const noexcept {
            return Item{catalog(index), owner(index), table(index), column(index), key_sequence(index)};
        }

        Item operator[](size_t index) const noexcept { return item(index); }
        ::cfrds_sql_primarykeys* get() const noexcept { return _ptr; }

    private:
        ::cfrds_sql_primarykeys* _ptr{nullptr};
    };

    class ForeignKeys {
    public:
        struct Item {
            std::string_view pk_catalog;
            std::string_view pk_owner;
            std::string_view pk_table;
            std::string_view pk_column;
            std::string_view fk_catalog;
            std::string_view fk_owner;
            std::string_view fk_table;
            std::string_view fk_column;
            int key_sequence;
            int update_rule;
            int delete_rule;
        };

        explicit ForeignKeys(::cfrds_sql_foreignkeys* ptr) noexcept : _ptr(ptr) {}
        ~ForeignKeys() { if (_ptr) ::cfrds_sql_foreignkeys_free(_ptr); }
        ForeignKeys(const ForeignKeys&) = delete;
        ForeignKeys& operator=(const ForeignKeys&) = delete;
        ForeignKeys(ForeignKeys&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        ForeignKeys& operator=(ForeignKeys&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_sql_foreignkeys_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept { return ::cfrds_sql_foreignkeys_count(_ptr); }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        std::string_view pk_catalog(size_t index) const noexcept {
            const char* s = ::cfrds_sql_foreignkeys_get_pkcatalog(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view pk_owner(size_t index) const noexcept {
            const char* s = ::cfrds_sql_foreignkeys_get_pkowner(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view pk_table(size_t index) const noexcept {
            const char* s = ::cfrds_sql_foreignkeys_get_pktable(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view pk_column(size_t index) const noexcept {
            const char* s = ::cfrds_sql_foreignkeys_get_pkcolumn(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_catalog(size_t index) const noexcept {
            const char* s = ::cfrds_sql_foreignkeys_get_fkcatalog(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_owner(size_t index) const noexcept {
            const char* s = ::cfrds_sql_foreignkeys_get_fkowner(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_table(size_t index) const noexcept {
            const char* s = ::cfrds_sql_foreignkeys_get_fktable(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_column(size_t index) const noexcept {
            const char* s = ::cfrds_sql_foreignkeys_get_fkcolumn(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        int key_sequence(size_t index) const noexcept {
            return ::cfrds_sql_foreignkeys_get_key_sequence(_ptr, index);
        }

        int update_rule(size_t index) const noexcept {
            return ::cfrds_sql_foreignkeys_get_updaterule(_ptr, index);
        }

        int delete_rule(size_t index) const noexcept {
            return ::cfrds_sql_foreignkeys_get_deleterule(_ptr, index);
        }

        Item item(size_t index) const noexcept {
            return Item{pk_catalog(index), pk_owner(index), pk_table(index), pk_column(index),
                        fk_catalog(index), fk_owner(index), fk_table(index), fk_column(index),
                        key_sequence(index), update_rule(index), delete_rule(index)};
        }

        Item operator[](size_t index) const noexcept { return item(index); }
        ::cfrds_sql_foreignkeys* get() const noexcept { return _ptr; }

    private:
        ::cfrds_sql_foreignkeys* _ptr{nullptr};
    };

    class ImportedKeys {
    public:
        struct Item {
            std::string_view pk_catalog;
            std::string_view pk_owner;
            std::string_view pk_table;
            std::string_view pk_column;
            std::string_view fk_catalog;
            std::string_view fk_owner;
            std::string_view fk_table;
            std::string_view fk_column;
            int key_sequence;
            int update_rule;
            int delete_rule;
        };

        explicit ImportedKeys(::cfrds_sql_importedkeys* ptr) noexcept : _ptr(ptr) {}
        ~ImportedKeys() { if (_ptr) ::cfrds_sql_importedkeys_free(_ptr); }
        ImportedKeys(const ImportedKeys&) = delete;
        ImportedKeys& operator=(const ImportedKeys&) = delete;
        ImportedKeys(ImportedKeys&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        ImportedKeys& operator=(ImportedKeys&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_sql_importedkeys_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept { return ::cfrds_sql_importedkeys_count(_ptr); }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        std::string_view pk_catalog(size_t index) const noexcept {
            const char* s = ::cfrds_sql_importedkeys_get_pkcatalog(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view pk_owner(size_t index) const noexcept {
            const char* s = ::cfrds_sql_importedkeys_get_pkowner(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view pk_table(size_t index) const noexcept {
            const char* s = ::cfrds_sql_importedkeys_get_pktable(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view pk_column(size_t index) const noexcept {
            const char* s = ::cfrds_sql_importedkeys_get_pkcolumn(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_catalog(size_t index) const noexcept {
            const char* s = ::cfrds_sql_importedkeys_get_fkcatalog(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_owner(size_t index) const noexcept {
            const char* s = ::cfrds_sql_importedkeys_get_fkowner(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_table(size_t index) const noexcept {
            const char* s = ::cfrds_sql_importedkeys_get_fktable(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_column(size_t index) const noexcept {
            const char* s = ::cfrds_sql_importedkeys_get_fkcolumn(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        int key_sequence(size_t index) const noexcept {
            return ::cfrds_sql_importedkeys_get_key_sequence(_ptr, index);
        }

        int update_rule(size_t index) const noexcept {
            return ::cfrds_sql_importedkeys_get_updaterule(_ptr, index);
        }

        int delete_rule(size_t index) const noexcept {
            return ::cfrds_sql_importedkeys_get_deleterule(_ptr, index);
        }

        Item item(size_t index) const noexcept {
            return Item{pk_catalog(index), pk_owner(index), pk_table(index), pk_column(index),
                        fk_catalog(index), fk_owner(index), fk_table(index), fk_column(index),
                        key_sequence(index), update_rule(index), delete_rule(index)};
        }

        Item operator[](size_t index) const noexcept { return item(index); }
        ::cfrds_sql_importedkeys* get() const noexcept { return _ptr; }

    private:
        ::cfrds_sql_importedkeys* _ptr{nullptr};
    };

    class ExportedKeys {
    public:
        struct Item {
            std::string_view pk_catalog;
            std::string_view pk_owner;
            std::string_view pk_table;
            std::string_view pk_column;
            std::string_view fk_catalog;
            std::string_view fk_owner;
            std::string_view fk_table;
            std::string_view fk_column;
            int key_sequence;
            int update_rule;
            int delete_rule;
        };

        explicit ExportedKeys(::cfrds_sql_exportedkeys* ptr) noexcept : _ptr(ptr) {}
        ~ExportedKeys() { if (_ptr) ::cfrds_sql_exportedkeys_free(_ptr); }
        ExportedKeys(const ExportedKeys&) = delete;
        ExportedKeys& operator=(const ExportedKeys&) = delete;
        ExportedKeys(ExportedKeys&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        ExportedKeys& operator=(ExportedKeys&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_sql_exportedkeys_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept { return ::cfrds_sql_exportedkeys_count(_ptr); }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        std::string_view pk_catalog(size_t index) const noexcept {
            const char* s = ::cfrds_sql_exportedkeys_get_pkcatalog(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view pk_owner(size_t index) const noexcept {
            const char* s = ::cfrds_sql_exportedkeys_get_pkowner(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view pk_table(size_t index) const noexcept {
            const char* s = ::cfrds_sql_exportedkeys_get_pktable(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view pk_column(size_t index) const noexcept {
            const char* s = ::cfrds_sql_exportedkeys_get_pkcolumn(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_catalog(size_t index) const noexcept {
            const char* s = ::cfrds_sql_exportedkeys_get_fkcatalog(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_owner(size_t index) const noexcept {
            const char* s = ::cfrds_sql_exportedkeys_get_fkowner(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_table(size_t index) const noexcept {
            const char* s = ::cfrds_sql_exportedkeys_get_fktable(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view fk_column(size_t index) const noexcept {
            const char* s = ::cfrds_sql_exportedkeys_get_fkcolumn(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        int key_sequence(size_t index) const noexcept {
            return ::cfrds_sql_exportedkeys_get_key_sequence(_ptr, index);
        }

        int update_rule(size_t index) const noexcept {
            return ::cfrds_sql_exportedkeys_get_updaterule(_ptr, index);
        }

        int delete_rule(size_t index) const noexcept {
            return ::cfrds_sql_exportedkeys_get_deleterule(_ptr, index);
        }

        Item item(size_t index) const noexcept {
            return Item{pk_catalog(index), pk_owner(index), pk_table(index), pk_column(index),
                        fk_catalog(index), fk_owner(index), fk_table(index), fk_column(index),
                        key_sequence(index), update_rule(index), delete_rule(index)};
        }

        Item operator[](size_t index) const noexcept { return item(index); }
        ::cfrds_sql_exportedkeys* get() const noexcept { return _ptr; }

    private:
        ::cfrds_sql_exportedkeys* _ptr{nullptr};
    };

    class SqlMetadata {
    public:
        struct Item {
            std::string_view name;
            std::string_view type;
            std::string_view jtype;
        };

        explicit SqlMetadata(::cfrds_sql_metadata* ptr) noexcept : _ptr(ptr) {}
        ~SqlMetadata() { if (_ptr) ::cfrds_sql_metadata_free(_ptr); }
        SqlMetadata(const SqlMetadata&) = delete;
        SqlMetadata& operator=(const SqlMetadata&) = delete;
        SqlMetadata(SqlMetadata&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        SqlMetadata& operator=(SqlMetadata&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_sql_metadata_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept { return ::cfrds_sql_metadata_count(_ptr); }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        std::string_view name(size_t index) const noexcept {
            const char* s = ::cfrds_sql_metadata_get_name(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view type(size_t index) const noexcept {
            const char* s = ::cfrds_sql_metadata_get_type(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view jtype(size_t index) const noexcept {
            const char* s = ::cfrds_sql_metadata_get_jtype(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        Item item(size_t index) const noexcept {
            return Item{name(index), type(index), jtype(index)};
        }

        Item operator[](size_t index) const noexcept { return item(index); }
        ::cfrds_sql_metadata* get() const noexcept { return _ptr; }

    private:
        ::cfrds_sql_metadata* _ptr{nullptr};
    };

    class SupportedCommands {
    public:
        explicit SupportedCommands(::cfrds_sql_supportedcommands* ptr) noexcept : _ptr(ptr) {}
        ~SupportedCommands() { if (_ptr) ::cfrds_sql_supportedcommands_free(_ptr); }
        SupportedCommands(const SupportedCommands&) = delete;
        SupportedCommands& operator=(const SupportedCommands&) = delete;
        SupportedCommands(SupportedCommands&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        SupportedCommands& operator=(SupportedCommands&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_sql_supportedcommands_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept { return ::cfrds_sql_supportedcommands_count(_ptr); }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        std::string_view get(size_t index) const noexcept {
            const char* s = ::cfrds_sql_supportedcommands_get(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view operator[](size_t index) const noexcept { return get(index); }
        ::cfrds_sql_supportedcommands* raw() const noexcept { return _ptr; }

    private:
        ::cfrds_sql_supportedcommands* _ptr{nullptr};
    };

    class CustomTagPaths {
    public:
        explicit CustomTagPaths(::cfrds_adminapi_customtagpaths* ptr) noexcept : _ptr(ptr) {}
        ~CustomTagPaths() { if (_ptr) ::cfrds_adminapi_customtagpaths_free(_ptr); }
        CustomTagPaths(const CustomTagPaths&) = delete;
        CustomTagPaths& operator=(const CustomTagPaths&) = delete;
        CustomTagPaths(CustomTagPaths&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        CustomTagPaths& operator=(CustomTagPaths&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_adminapi_customtagpaths_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept {
            int c = ::cfrds_adminapi_customtagpaths_count(_ptr);
            return c > 0 ? static_cast<size_t>(c) : 0;
        }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        std::string_view at(size_t index) const noexcept {
            const char* s = ::cfrds_adminapi_customtagpaths_at(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view operator[](size_t index) const noexcept { return at(index); }
        ::cfrds_adminapi_customtagpaths* get() const noexcept { return _ptr; }

    private:
        ::cfrds_adminapi_customtagpaths* _ptr{nullptr};
    };

    class Mappings {
    public:
        struct Item {
            std::string_view key;
            std::string_view value;
        };

        explicit Mappings(::cfrds_adminapi_mappings* ptr) noexcept : _ptr(ptr) {}
        ~Mappings() { if (_ptr) ::cfrds_adminapi_mappings_free(_ptr); }
        Mappings(const Mappings&) = delete;
        Mappings& operator=(const Mappings&) = delete;
        Mappings(Mappings&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        Mappings& operator=(Mappings&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_adminapi_mappings_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        size_t count() const noexcept {
            int c = ::cfrds_adminapi_mappings_count(_ptr);
            return c > 0 ? static_cast<size_t>(c) : 0;
        }
        size_t size() const noexcept { return count(); }
        bool empty() const noexcept { return count() == 0; }

        std::string_view key(size_t index) const noexcept {
            const char* s = ::cfrds_adminapi_mappings_key(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view value(size_t index) const noexcept {
            const char* s = ::cfrds_adminapi_mappings_value(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        Item item(size_t index) const noexcept {
            return Item{key(index), value(index)};
        }

        Item operator[](size_t index) const noexcept { return item(index); }
        ::cfrds_adminapi_mappings* get() const noexcept { return _ptr; }

    private:
        ::cfrds_adminapi_mappings* _ptr{nullptr};
    };

    class SecurityAnalyzerResult {
    public:
        struct ErrorItem {
            std::string_view type;
            std::string_view filename;
            int begin_line;
            int end_line;
            int begin_column;
            int end_column;
            std::string_view error;
            std::string_view error_message;
            std::string_view reference_type;
        };

        struct ScannedItem {
            std::string_view result;
            std::string_view filename;
        };

        struct NotScannedItem {
            std::string_view reason;
            std::string_view filename;
        };

        explicit SecurityAnalyzerResult(::cfrds_security_analyzer_result* ptr) noexcept : _ptr(ptr) {}
        ~SecurityAnalyzerResult() { if (_ptr) ::cfrds_security_analyzer_result_free(_ptr); }
        SecurityAnalyzerResult(const SecurityAnalyzerResult&) = delete;
        SecurityAnalyzerResult& operator=(const SecurityAnalyzerResult&) = delete;
        SecurityAnalyzerResult(SecurityAnalyzerResult&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        SecurityAnalyzerResult& operator=(SecurityAnalyzerResult&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_security_analyzer_result_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        std::string_view status() const noexcept {
            const char* s = ::cfrds_security_analyzer_result_status(_ptr);
            return s ? std::string_view(s) : std::string_view{};
        }

        int total_files() const noexcept { return ::cfrds_security_analyzer_result_totalfiles(_ptr); }
        int files_visited_count() const noexcept { return ::cfrds_security_analyzer_result_filesvisitedcount(_ptr); }
        int percentage() const noexcept { return ::cfrds_security_analyzer_result_percentage(_ptr); }
        int64_t last_updated() const noexcept { return ::cfrds_security_analyzer_result_lastupdated(_ptr); }

        std::string_view executor_service() const noexcept {
            const char* s = ::cfrds_security_analyzer_result_executorservice(_ptr);
            return s ? std::string_view(s) : std::string_view{};
        }

        size_t scanned_count() const noexcept {
            int c = ::cfrds_security_analyzer_result_filesscanned_count(_ptr);
            return c > 0 ? static_cast<size_t>(c) : 0;
        }

        ScannedItem scanned_item(size_t index) const noexcept {
            const char* res = ::cfrds_security_analyzer_result_filesscanned_item_result(_ptr, index);
            const char* fn = ::cfrds_security_analyzer_result_filesscanned_item_filename(_ptr, index);
            return ScannedItem{res ? std::string_view(res) : std::string_view{},
                               fn ? std::string_view(fn) : std::string_view{}};
        }

        size_t not_scanned_count() const noexcept {
            int c = ::cfrds_security_analyzer_result_filesnotscanned_count(_ptr);
            return c > 0 ? static_cast<size_t>(c) : 0;
        }

        NotScannedItem not_scanned_item(size_t index) const noexcept {
            const char* r = ::cfrds_security_analyzer_result_filesnotscanned_item_reason(_ptr, index);
            const char* fn = ::cfrds_security_analyzer_result_filesnotscanned_item_filename(_ptr, index);
            return NotScannedItem{r ? std::string_view(r) : std::string_view{},
                                  fn ? std::string_view(fn) : std::string_view{}};
        }

        size_t errors_count() const noexcept {
            int c = ::cfrds_security_analyzer_result_errors_count(_ptr);
            return c > 0 ? static_cast<size_t>(c) : 0;
        }

        ErrorItem error_item(size_t index) const noexcept {
            const char* t = ::cfrds_security_analyzer_result_errors_item_type(_ptr, index);
            const char* fn = ::cfrds_security_analyzer_result_errors_item_filename(_ptr, index);
            const char* err = ::cfrds_security_analyzer_result_errors_item_error(_ptr, index);
            const char* msg = ::cfrds_security_analyzer_result_errors_item_errormessage(_ptr, index);
            const char* ref = ::cfrds_security_analyzer_result_errors_item_referencetype(_ptr, index);
            return ErrorItem{
                t ? std::string_view(t) : std::string_view{},
                fn ? std::string_view(fn) : std::string_view{},
                ::cfrds_security_analyzer_result_errors_item_beginline(_ptr, index),
                ::cfrds_security_analyzer_result_errors_item_endline(_ptr, index),
                ::cfrds_security_analyzer_result_errors_item_begincolumn(_ptr, index),
                ::cfrds_security_analyzer_result_errors_item_endcolumn(_ptr, index),
                err ? std::string_view(err) : std::string_view{},
                msg ? std::string_view(msg) : std::string_view{},
                ref ? std::string_view(ref) : std::string_view{}
            };
        }

        ::cfrds_security_analyzer_result* get() const noexcept { return _ptr; }

    private:
        ::cfrds_security_analyzer_result* _ptr{nullptr};
    };

    class DebuggerEvent {
    public:
        explicit DebuggerEvent(::cfrds_debugger_event* ptr) noexcept : _ptr(ptr) {}
        ~DebuggerEvent() { if (_ptr) ::cfrds_debugger_event_free(_ptr); }
        DebuggerEvent(const DebuggerEvent&) = delete;
        DebuggerEvent& operator=(const DebuggerEvent&) = delete;
        DebuggerEvent(DebuggerEvent&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        DebuggerEvent& operator=(DebuggerEvent&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_debugger_event_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        int event_type() const noexcept { return ::cfrds_debugger_event_get_type(_ptr); }

        std::string_view breakpoint_source() const noexcept {
            const char* s = ::cfrds_debugger_event_breakpoint_get_source(_ptr);
            return s ? std::string_view(s) : std::string_view{};
        }

        int breakpoint_line() const noexcept {
            return ::cfrds_debugger_event_breakpoint_get_line(_ptr);
        }

        std::string_view breakpoint_thread_name() const noexcept {
            const char* s = ::cfrds_debugger_event_breakpoint_get_thread_name(_ptr);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view breakpoint_set_pathname() const noexcept {
            const char* s = ::cfrds_debugger_event_breakpoint_set_get_pathname(_ptr);
            return s ? std::string_view(s) : std::string_view{};
        }

        int breakpoint_set_req_line() const noexcept {
            return ::cfrds_debugger_event_breakpoint_set_get_req_line(_ptr);
        }

        int breakpoint_set_act_line() const noexcept {
            return ::cfrds_debugger_event_breakpoint_set_get_act_line(_ptr);
        }

        size_t scopes_count() const noexcept {
            int c = ::cfrds_debugger_event_get_scopes_count(_ptr);
            return c > 0 ? static_cast<size_t>(c) : 0;
        }

        std::string_view scopes_item_name(size_t index) const noexcept {
            const char* s = ::cfrds_debugger_event_get_scopes_item_name(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        size_t threads_count() const noexcept {
            int c = ::cfrds_debugger_event_get_threads_count(_ptr);
            return c > 0 ? static_cast<size_t>(c) : 0;
        }

        std::string_view threads_item_name(size_t index) const noexcept {
            const char* s = ::cfrds_debugger_event_get_threads_item_name(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        std::string_view threads_item_state(size_t index) const noexcept {
            const char* s = ::cfrds_debugger_event_get_threads_item_state(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        size_t watch_count() const noexcept {
            int c = ::cfrds_debugger_event_get_watch_count(_ptr);
            return c > 0 ? static_cast<size_t>(c) : 0;
        }

        std::string_view watch_item(size_t index) const noexcept {
            const char* s = ::cfrds_debugger_event_get_watch_item(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        size_t cf_trace_count() const noexcept {
            int c = ::cfrds_debugger_event_get_cf_trace_count(_ptr);
            return c > 0 ? static_cast<size_t>(c) : 0;
        }

        std::string_view cf_trace_item(size_t index) const noexcept {
            const char* s = ::cfrds_debugger_event_get_cf_trace_item(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        size_t java_trace_count() const noexcept {
            int c = ::cfrds_debugger_event_get_java_trace_count(_ptr);
            return c > 0 ? static_cast<size_t>(c) : 0;
        }

        std::string_view java_trace_item(size_t index) const noexcept {
            const char* s = ::cfrds_debugger_event_get_java_trace_item(_ptr, index);
            return s ? std::string_view(s) : std::string_view{};
        }

        ::cfrds_debugger_event* get() const noexcept { return _ptr; }

    private:
        ::cfrds_debugger_event* _ptr{nullptr};
    };

    class GraphingBuffer {
    public:
        explicit GraphingBuffer(::cfrds_buffer* ptr) noexcept : _ptr(ptr) {}
        ~GraphingBuffer() { if (_ptr) ::cfrds_buffer_free(_ptr); }
        GraphingBuffer(const GraphingBuffer&) = delete;
        GraphingBuffer& operator=(const GraphingBuffer&) = delete;
        GraphingBuffer(GraphingBuffer&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
        GraphingBuffer& operator=(GraphingBuffer&& other) noexcept {
            if (this != &other) {
                if (_ptr) ::cfrds_buffer_free(_ptr);
                _ptr = std::exchange(other._ptr, nullptr);
            }
            return *this;
        }

        std::span<const std::byte> bytes() const noexcept {
            if (!_ptr) return {};
            return {
                reinterpret_cast<const std::byte*>(::cfrds_buffer_data(_ptr)),
                ::cfrds_buffer_data_size(_ptr)
            };
        }

        const std::byte* data() const noexcept {
            return _ptr ? reinterpret_cast<const std::byte*>(::cfrds_buffer_data(_ptr)) : nullptr;
        }

        size_t size() const noexcept { return _ptr ? ::cfrds_buffer_data_size(_ptr) : 0; }
        bool empty() const noexcept { return size() == 0; }
        ::cfrds_buffer* get() const noexcept { return _ptr; }

    private:
        ::cfrds_buffer* _ptr{nullptr};
    };

    struct IdeInfo {
        int code{0};
        std::string server_version;
        std::string client_version;
        int num2{0};
        int num3{0};
    };

    struct SecurityAnalyzerStatus {
        int total_files{0};
        int files_visited_count{0};
        int percentage{0};
        int64_t last_updated{0};
    };

    // ------------------------------------------------------------------------------------------------
    // Server Class
    // ------------------------------------------------------------------------------------------------

    class Server {
    public:
        // Type aliases
        using BrowseDir = cfrds::BrowseDir;
        using FileContent = cfrds::FileContent;
        using SqlResultSet = cfrds::SqlResultSet;
        using DsnInfo = cfrds::DsnInfo;
        using TableInfo = cfrds::TableInfo;
        using ColumnInfo = cfrds::ColumnInfo;
        using PrimaryKeys = cfrds::PrimaryKeys;
        using ForeignKeys = cfrds::ForeignKeys;
        using ImportedKeys = cfrds::ImportedKeys;
        using ExportedKeys = cfrds::ExportedKeys;
        using SqlMetadata = cfrds::SqlMetadata;
        using SupportedCommands = cfrds::SupportedCommands;
        using CustomTagPaths = cfrds::CustomTagPaths;
        using Mappings = cfrds::Mappings;
        using SecurityAnalyzerResult = cfrds::SecurityAnalyzerResult;
        using DebuggerEvent = cfrds::DebuggerEvent;
        using GraphingBuffer = cfrds::GraphingBuffer;

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

        // ====================================================================
        // File Operations
        // ====================================================================

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

        void write_file(std::string_view path, std::span<const std::byte> content) {
            std::string null_term_path(path);
            cfrds_status st = ::cfrds_command_file_write(_ptr, null_term_path.c_str(), content.data(), content.size());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during write_file");
            }
        }

        void write_file(std::string_view path, std::string_view content) {
            std::string null_term_path(path);
            cfrds_status st = ::cfrds_command_file_write(_ptr, null_term_path.c_str(), content.data(), content.size());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during write_file");
            }
        }

        void rename_file(std::string_view source_path, std::string_view dest_path) {
            std::string src(source_path);
            std::string dst(dest_path);
            cfrds_status st = ::cfrds_command_file_rename(_ptr, src.c_str(), dst.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during rename_file");
            }
        }

        void remove_file(std::string_view path) {
            std::string null_term_path(path);
            cfrds_status st = ::cfrds_command_file_remove_file(_ptr, null_term_path.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during remove_file");
            }
        }

        void remove_dir(std::string_view path) {
            std::string null_term_path(path);
            cfrds_status st = ::cfrds_command_file_remove_dir(_ptr, null_term_path.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during remove_dir");
            }
        }

        bool file_exists(std::string_view path) {
            std::string null_term_path(path);
            bool exists = false;
            cfrds_status st = ::cfrds_command_file_exists(_ptr, null_term_path.c_str(), &exists);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during file_exists");
            }
            return exists;
        }

        void create_dir(std::string_view path) {
            std::string null_term_path(path);
            cfrds_status st = ::cfrds_command_file_create_dir(_ptr, null_term_path.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during create_dir");
            }
        }

        std::string get_root_dir() {
            char* raw = nullptr;
            cfrds_status st = ::cfrds_command_file_get_root_dir(_ptr, &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during get_root_dir");
            }
            std::string res(raw ? raw : "");
            if (raw) std::free(raw);
            return res;
        }

        // ====================================================================
        // SQL Operations
        // ====================================================================

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

        DsnInfo sql_dsninfo() {
            ::cfrds_sql_dsninfo* raw = nullptr;
            cfrds_status st = ::cfrds_command_sql_dsninfo(_ptr, &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during sql_dsninfo");
            }
            return DsnInfo(raw);
        }

        TableInfo sql_tableinfo(std::string_view dsn) {
            std::string null_term_dsn(dsn);
            ::cfrds_sql_tableinfo* raw = nullptr;
            cfrds_status st = ::cfrds_command_sql_tableinfo(_ptr, null_term_dsn.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during sql_tableinfo");
            }
            return TableInfo(raw);
        }

        ColumnInfo sql_columninfo(std::string_view dsn, std::string_view table) {
            std::string null_term_dsn(dsn);
            std::string null_term_tbl(table);
            ::cfrds_sql_columninfo* raw = nullptr;
            cfrds_status st = ::cfrds_command_sql_columninfo(_ptr, null_term_dsn.c_str(), null_term_tbl.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during sql_columninfo");
            }
            return ColumnInfo(raw);
        }

        PrimaryKeys sql_primarykeys(std::string_view dsn, std::string_view table) {
            std::string null_term_dsn(dsn);
            std::string null_term_tbl(table);
            ::cfrds_sql_primarykeys* raw = nullptr;
            cfrds_status st = ::cfrds_command_sql_primarykeys(_ptr, null_term_dsn.c_str(), null_term_tbl.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during sql_primarykeys");
            }
            return PrimaryKeys(raw);
        }

        ForeignKeys sql_foreignkeys(std::string_view dsn, std::string_view table) {
            std::string null_term_dsn(dsn);
            std::string null_term_tbl(table);
            ::cfrds_sql_foreignkeys* raw = nullptr;
            cfrds_status st = ::cfrds_command_sql_foreignkeys(_ptr, null_term_dsn.c_str(), null_term_tbl.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during sql_foreignkeys");
            }
            return ForeignKeys(raw);
        }

        ImportedKeys sql_importedkeys(std::string_view dsn, std::string_view table) {
            std::string null_term_dsn(dsn);
            std::string null_term_tbl(table);
            ::cfrds_sql_importedkeys* raw = nullptr;
            cfrds_status st = ::cfrds_command_sql_importedkeys(_ptr, null_term_dsn.c_str(), null_term_tbl.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during sql_importedkeys");
            }
            return ImportedKeys(raw);
        }

        ExportedKeys sql_exportedkeys(std::string_view dsn, std::string_view table) {
            std::string null_term_dsn(dsn);
            std::string null_term_tbl(table);
            ::cfrds_sql_exportedkeys* raw = nullptr;
            cfrds_status st = ::cfrds_command_sql_exportedkeys(_ptr, null_term_dsn.c_str(), null_term_tbl.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during sql_exportedkeys");
            }
            return ExportedKeys(raw);
        }

        SqlMetadata sql_metadata(std::string_view dsn, std::string_view sql) {
            std::string null_term_dsn(dsn);
            std::string null_term_sql(sql);
            ::cfrds_sql_metadata* raw = nullptr;
            cfrds_status st = ::cfrds_command_sql_sqlmetadata(_ptr, null_term_dsn.c_str(), null_term_sql.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during sql_metadata");
            }
            return SqlMetadata(raw);
        }

        SupportedCommands sql_supportedcommands() {
            ::cfrds_sql_supportedcommands* raw = nullptr;
            cfrds_status st = ::cfrds_command_sql_getsupportedcommands(_ptr, &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during sql_supportedcommands");
            }
            return SupportedCommands(raw);
        }

        std::string sql_dbdescription(std::string_view dsn) {
            std::string null_term_dsn(dsn);
            char* raw = nullptr;
            cfrds_status st = ::cfrds_command_sql_dbdescription(_ptr, null_term_dsn.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during sql_dbdescription");
            }
            std::string res(raw ? raw : "");
            if (raw) std::free(raw);
            return res;
        }

        // ====================================================================
        // Debugger Operations
        // ====================================================================

        std::string debugger_start() {
            char* session_id = nullptr;
            cfrds_status st = ::cfrds_command_debugger_start(_ptr, &session_id);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_start");
            }
            std::string res(session_id ? session_id : "");
            if (session_id) std::free(session_id);
            return res;
        }

        void debugger_stop(std::string_view session_id) {
            std::string sid(session_id);
            cfrds_status st = ::cfrds_command_debugger_stop(_ptr, sid.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_stop");
            }
        }

        void debugger_server_stop(std::string_view session_id) {
            std::string sid(session_id);
            cfrds_status st = ::cfrds_command_debugger_server_stop(_ptr, sid.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_server_stop");
            }
        }

        uint16_t debugger_get_server_info(std::string_view session_id) {
            std::string sid(session_id);
            uint16_t port = 0;
            cfrds_status st = ::cfrds_command_debugger_get_server_info(_ptr, sid.c_str(), &port);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_get_server_info");
            }
            return port;
        }

        void debugger_breakpoint_on_exception(std::string_view session_id, bool value) {
            std::string sid(session_id);
            cfrds_status st = ::cfrds_command_debugger_breakpoint_on_exception(_ptr, sid.c_str(), value);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_breakpoint_on_exception");
            }
        }

        void debugger_global_breakpoint_on_exception(std::string_view session_id, bool value) {
            std::string sid(session_id);
            cfrds_status st = ::cfrds_command_debugger_global_breakpoint_on_exception(_ptr, sid.c_str(), value);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_global_breakpoint_on_exception");
            }
        }

        void debugger_breakpoint(std::string_view session_id, std::string_view filepath, int line, bool enable) {
            std::string sid(session_id);
            std::string fp(filepath);
            cfrds_status st = ::cfrds_command_debugger_breakpoint(_ptr, sid.c_str(), fp.c_str(), line, enable);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_breakpoint");
            }
        }

        void debugger_clear_all_breakpoints(std::string_view session_id) {
            std::string sid(session_id);
            cfrds_status st = ::cfrds_command_debugger_clear_all_breakpoints(_ptr, sid.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_clear_all_breakpoints");
            }
        }

        DebuggerEvent debugger_get_debug_events(std::string_view session_id) {
            std::string sid(session_id);
            ::cfrds_debugger_event* raw = nullptr;
            cfrds_status st = ::cfrds_command_debugger_get_debug_events(_ptr, sid.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_get_debug_events");
            }
            return DebuggerEvent(raw);
        }

        DebuggerEvent debugger_all_fetch_flags_enabled(std::string_view session_id, bool threads, bool watch, bool scopes, bool cf_trace, bool java_trace) {
            std::string sid(session_id);
            ::cfrds_debugger_event* raw = nullptr;
            cfrds_status st = ::cfrds_command_debugger_all_fetch_flags_enabled(_ptr, sid.c_str(), threads, watch, scopes, cf_trace, java_trace, &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_all_fetch_flags_enabled");
            }
            return DebuggerEvent(raw);
        }

        void debugger_step_in(std::string_view session_id, std::string_view thread_name) {
            std::string sid(session_id);
            std::string tn(thread_name);
            cfrds_status st = ::cfrds_command_debugger_step_in(_ptr, sid.c_str(), tn.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_step_in");
            }
        }

        void debugger_step_over(std::string_view session_id, std::string_view thread_name) {
            std::string sid(session_id);
            std::string tn(thread_name);
            cfrds_status st = ::cfrds_command_debugger_step_over(_ptr, sid.c_str(), tn.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_step_over");
            }
        }

        void debugger_step_out(std::string_view session_id, std::string_view thread_name) {
            std::string sid(session_id);
            std::string tn(thread_name);
            cfrds_status st = ::cfrds_command_debugger_step_out(_ptr, sid.c_str(), tn.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_step_out");
            }
        }

        DebuggerEvent debugger_sync_step_in(std::string_view session_id, std::string_view thread_name) {
            std::string sid(session_id);
            std::string tn(thread_name);
            ::cfrds_debugger_event* raw = nullptr;
            cfrds_status st = ::cfrds_command_debugger_sync_step_in(_ptr, sid.c_str(), tn.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_sync_step_in");
            }
            return DebuggerEvent(raw);
        }

        DebuggerEvent debugger_sync_step_over(std::string_view session_id, std::string_view thread_name) {
            std::string sid(session_id);
            std::string tn(thread_name);
            ::cfrds_debugger_event* raw = nullptr;
            cfrds_status st = ::cfrds_command_debugger_sync_step_over(_ptr, sid.c_str(), tn.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_sync_step_over");
            }
            return DebuggerEvent(raw);
        }

        DebuggerEvent debugger_sync_step_out(std::string_view session_id, std::string_view thread_name) {
            std::string sid(session_id);
            std::string tn(thread_name);
            ::cfrds_debugger_event* raw = nullptr;
            cfrds_status st = ::cfrds_command_debugger_sync_step_out(_ptr, sid.c_str(), tn.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_sync_step_out");
            }
            return DebuggerEvent(raw);
        }

        void debugger_continue(std::string_view session_id, std::string_view thread_name) {
            std::string sid(session_id);
            std::string tn(thread_name);
            cfrds_status st = ::cfrds_command_debugger_continue(_ptr, sid.c_str(), tn.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_continue");
            }
        }

        DebuggerEvent debugger_get_cf_variables(std::string_view session_id, std::string_view thread_name) {
            std::string sid(session_id);
            std::string tn(thread_name);
            ::cfrds_debugger_event* raw = nullptr;
            cfrds_status st = ::cfrds_command_debugger_get_cf_variables(_ptr, sid.c_str(), tn.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_get_cf_variables");
            }
            return DebuggerEvent(raw);
        }

        void debugger_watch_expression(std::string_view session_id, std::string_view thread_name, std::string_view expression) {
            std::string sid(session_id);
            std::string tn(thread_name);
            std::string expr(expression);
            cfrds_status st = ::cfrds_command_debugger_watch_expression(_ptr, sid.c_str(), tn.c_str(), expr.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_watch_expression");
            }
        }

        void debugger_set_variable(std::string_view session_id, std::string_view thread_name, std::string_view variable, std::string_view value) {
            std::string sid(session_id);
            std::string tn(thread_name);
            std::string var(variable);
            std::string val(value);
            cfrds_status st = ::cfrds_command_debugger_set_variable(_ptr, sid.c_str(), tn.c_str(), var.c_str(), val.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_set_variable");
            }
        }

        void debugger_watch_variables(std::string_view session_id, std::string_view variables) {
            std::string sid(session_id);
            std::string vars(variables);
            cfrds_status st = ::cfrds_command_debugger_watch_variables(_ptr, sid.c_str(), vars.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_watch_variables");
            }
        }

        std::string debugger_get_output(std::string_view session_id, std::string_view thread_name) {
            std::string sid(session_id);
            std::string tn(thread_name);
            char* raw = nullptr;
            cfrds_status st = ::cfrds_command_debugger_get_output(_ptr, sid.c_str(), tn.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_get_output");
            }
            std::string res(raw ? raw : "");
            if (raw) std::free(raw);
            return res;
        }

        void debugger_set_scope_filter(std::string_view session_id, std::string_view filter) {
            std::string sid(session_id);
            std::string flt(filter);
            cfrds_status st = ::cfrds_command_debugger_set_scope_filter(_ptr, sid.c_str(), flt.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during debugger_set_scope_filter");
            }
        }

        // ====================================================================
        // Security Analyzer Operations
        // ====================================================================

        int security_analyzer_scan(std::string_view pathnames, bool recursively = true, int cores = 0) {
            std::string paths(pathnames);
            int command_id = 0;
            cfrds_status st = ::cfrds_command_security_analyzer_scan(_ptr, paths.c_str(), recursively, cores, &command_id);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during security_analyzer_scan");
            }
            return command_id;
        }

        void security_analyzer_cancel(int command_id) {
            cfrds_status st = ::cfrds_command_security_analyzer_cancel(_ptr, command_id);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during security_analyzer_cancel");
            }
        }

        SecurityAnalyzerStatus security_analyzer_status(int command_id) {
            SecurityAnalyzerStatus s;
            cfrds_status st = ::cfrds_command_security_analyzer_status(_ptr, command_id, &s.total_files, &s.files_visited_count, &s.percentage, &s.last_updated);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during security_analyzer_status");
            }
            return s;
        }

        SecurityAnalyzerResult security_analyzer_result(int command_id) {
            ::cfrds_security_analyzer_result* raw = nullptr;
            cfrds_status st = ::cfrds_command_security_analyzer_result(_ptr, command_id, &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during security_analyzer_result");
            }
            return SecurityAnalyzerResult(raw);
        }

        void security_analyzer_clean(int command_id) {
            cfrds_status st = ::cfrds_command_security_analyzer_clean(_ptr, command_id);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during security_analyzer_clean");
            }
        }

        // ====================================================================
        // IDE Default
        // ====================================================================

        IdeInfo ide_default(int version) {
            IdeInfo info;
            char* server_v = nullptr;
            char* client_v = nullptr;
            cfrds_status st = ::cfrds_command_ide_default(_ptr, version, &info.code, &server_v, &client_v, &info.num2, &info.num3);
            if (st != CFRDS_STATUS_OK) {
                if (server_v) std::free(server_v);
                if (client_v) std::free(client_v);
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during ide_default");
            }
            if (server_v) {
                info.server_version = server_v;
                std::free(server_v);
            }
            if (client_v) {
                info.client_version = client_v;
                std::free(client_v);
            }
            return info;
        }

        // ====================================================================
        // AdminAPI
        // ====================================================================

        std::string adminapi_debugging_getlogproperty(std::string_view logdirectory) {
            std::string dir(logdirectory);
            char* raw = nullptr;
            cfrds_status st = ::cfrds_command_adminapi_debugging_getlogproperty(_ptr, dir.c_str(), &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during adminapi_debugging_getlogproperty");
            }
            std::string res(raw ? raw : "");
            if (raw) std::free(raw);
            return res;
        }

        CustomTagPaths adminapi_extensions_getcustomtagpaths() {
            ::cfrds_adminapi_customtagpaths* raw = nullptr;
            cfrds_status st = ::cfrds_command_adminapi_extensions_getcustomtagpaths(_ptr, &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during adminapi_extensions_getcustomtagpaths");
            }
            return CustomTagPaths(raw);
        }

        void adminapi_extensions_setmapping(std::string_view name, std::string_view path) {
            std::string n(name);
            std::string p(path);
            cfrds_status st = ::cfrds_command_adminapi_extensions_setmapping(_ptr, n.c_str(), p.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during adminapi_extensions_setmapping");
            }
        }

        void adminapi_extensions_deletemapping(std::string_view mapping) {
            std::string m(mapping);
            cfrds_status st = ::cfrds_command_adminapi_extensions_deletemapping(_ptr, m.c_str());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during adminapi_extensions_deletemapping");
            }
        }

        Mappings adminapi_extensions_getmappings() {
            ::cfrds_adminapi_mappings* raw = nullptr;
            cfrds_status st = ::cfrds_command_adminapi_extensions_getmappings(_ptr, &raw);
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during adminapi_extensions_getmappings");
            }
            return Mappings(raw);
        }

        // ====================================================================
        // Graphing
        // ====================================================================

        GraphingBuffer graphing(std::string_view chart_attributes, const std::vector<std::string>& series_data) {
            std::string attr(chart_attributes);
            std::vector<const char*> c_series;
            c_series.reserve(series_data.size());
            for (const auto& s : series_data) {
                c_series.push_back(s.c_str());
            }
            ::cfrds_buffer* raw = nullptr;
            cfrds_status st = ::cfrds_command_graphing(_ptr, &raw, attr.c_str(), series_data.size(), c_series.empty() ? nullptr : c_series.data());
            if (st != CFRDS_STATUS_OK) {
                auto err = error();
                throw cfrds_exception(st, !err.empty() ? std::string(err) : "Unknown error during graphing");
            }
            return GraphingBuffer(raw);
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

