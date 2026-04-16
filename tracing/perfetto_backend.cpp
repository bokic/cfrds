#include "perfetto.h"
#include "tracing_backend.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cxxabi.h>
#include <memory>
#ifdef __linux__
#include <link.h>
#include <elf.h>
#endif
#include <cstring>
#include <cstdint>
#include <sys/mman.h>
#include <sys/stat.h>
#include <map>
#include <string>
#include <mutex>

// Define categories used in the project
PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("function_call").SetDescription("Automatic function tracing"),
    perfetto::Category("delay").SetDescription("Manual delay events"),
    perfetto::Category("net").SetDescription("Network I/O events")
);
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

static perfetto::TracingSession* g_session = nullptr;
static int trace_fd = -1;

// ---------------------------------------------------------------------------
// Symbol resolver: resolves both exported and static/local function symbols.
//
// Strategy:
//   1. dladdr()   — fast path, covers exported (non-static) symbols via .dynsym
//   2. On-disk ELF .symtab parse — covers static/local symbols in debug builds
//
// The .symtab section (unlike .dynsym / DT_SYMTAB) is only present in the
// on-disk ELF file, not memory-mapped into the process. We mmap each DSO
// once and cache an address→name map keyed by (base+st_value).
// ---------------------------------------------------------------------------

struct DsoCache {
    std::map<uintptr_t, std::string> sym_map; // addr → mangled name
    bool                             loaded = false;
};

static std::mutex              g_cache_mutex;
static std::map<std::string, DsoCache> g_dso_cache; // path → cache

#ifdef __linux__
static void load_symtab(const char* path, uintptr_t load_bias, DsoCache& cache)
{
    cache.loaded = true;
    if (!path || path[0] == '\0') return;

    int fd = open(path, O_RDONLY);
    if (fd < 0) return;

    struct stat st;
    if (fstat(fd, &st) < 0 || st.st_size < (off_t)sizeof(ElfW(Ehdr))) {
        close(fd);
        return;
    }

    void* map = mmap(nullptr, static_cast<size_t>(st.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (map == MAP_FAILED) return;

    auto* ehdr = static_cast<const ElfW(Ehdr)*>(map);
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        munmap(map, static_cast<size_t>(st.st_size));
        return;
    }

    const ElfW(Shdr)* shdrs = reinterpret_cast<const ElfW(Shdr)*>(
        static_cast<const char*>(map) + ehdr->e_shoff);
    const char* shstrtab = ehdr->e_shstrndx != SHN_XINDEX
        ? static_cast<const char*>(map) + shdrs[ehdr->e_shstrndx].sh_offset
        : nullptr;

    const ElfW(Sym)* symtab  = nullptr;
    size_t           nsyms   = 0;
    const char*      strtab  = nullptr;

    for (ElfW(Half) i = 0; i < ehdr->e_shnum; ++i) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            symtab = reinterpret_cast<const ElfW(Sym)*>(
                static_cast<const char*>(map) + shdrs[i].sh_offset);
            nsyms  = shdrs[i].sh_size / shdrs[i].sh_entsize;
            // Linked section is the string table
            if (shdrs[i].sh_link < ehdr->e_shnum)
                strtab = static_cast<const char*>(map) + shdrs[shdrs[i].sh_link].sh_offset;
        }
    }
    (void)shstrtab;

    if (symtab && strtab) {
        for (size_t i = 0; i < nsyms; ++i) {
            const ElfW(Sym)& sym = symtab[i];
            if (ELF64_ST_TYPE(sym.st_info) != STT_FUNC) continue;
            if (sym.st_value == 0) continue;
            uintptr_t addr = load_bias + sym.st_value;
            const char* name = strtab + sym.st_name;
            if (name && name[0] != '\0') {
                cache.sym_map.emplace(addr, name);
            }
        }
    }

    munmap(map, static_cast<size_t>(st.st_size));
}

struct ScanCtx {
    uintptr_t    addr;
    std::string  result;
};

static int scan_dsos_cb(struct dl_phdr_info* info, size_t /*size*/, void* data)
{
    auto* ctx = static_cast<ScanCtx*>(data);
    uintptr_t base = static_cast<uintptr_t>(info->dlpi_addr);

    // Check if addr falls within a PT_LOAD segment of this DSO.
    for (int i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr)& ph = info->dlpi_phdr[i];
        if (ph.p_type != PT_LOAD) continue;
        uintptr_t seg_start = base + ph.p_vaddr;
        uintptr_t seg_end   = seg_start + ph.p_memsz;
        if (ctx->addr >= seg_start && ctx->addr < seg_end) {
            goto found_dso;
        }
    }
    return 0; // not our DSO, keep iterating

found_dso:;
    const char* path = (info->dlpi_name && info->dlpi_name[0]) ? info->dlpi_name : nullptr;
    std::string key  = path ? std::string(path) : std::string("<main>");

    // Re-use /proc/self/exe for the main executable (empty name).
    std::string real_path;
    if (!path) {
        char buf[4096];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len > 0) { buf[len] = '\0'; real_path = buf; }
    } else {
        real_path = path;
    }

    std::unique_lock<std::mutex> lock(g_cache_mutex);
    DsoCache& cache = g_dso_cache[key];
    if (!cache.loaded) {
        load_symtab(real_path.empty() ? nullptr : real_path.c_str(), base, cache);
    }

    // Find closest symbol with address <= ctx->addr.
    auto it = cache.sym_map.upper_bound(ctx->addr);
    if (it != cache.sym_map.begin()) {
        --it;
        ctx->result = it->second;
    }

    return 1; // stop iteration
}
#endif

static const char* resolve_sym(void* addr)
{
    // Fast path via dynamic symbol table
    Dl_info info;
    if (dladdr(addr, &info) && info.dli_sname) {
        return info.dli_sname;
    }

#ifdef __linux__
    // Slow path: parse on-disk ELF .symtab
    // Use a thread-local buffer to avoid allocations on the hot path.
    static thread_local std::string tl_name;
    ScanCtx ctx;
    ctx.addr = reinterpret_cast<uintptr_t>(addr);

    TRACE_EVENT_BEGIN("delay", "dl_iterate_phdr");
    dl_iterate_phdr(scan_dsos_cb, &ctx);
    TRACE_EVENT_END("delay");

    if (!ctx.result.empty()) {
        tl_name = std::move(ctx.result);
        return tl_name.c_str();
    }
#endif
    return nullptr;
}

extern "C" {

void perf_backend_init() {
    perfetto::TracingInitArgs args;
    args.backends = perfetto::kInProcessBackend;
    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();
}

void perf_backend_start(const char* filename) {
    perfetto::TraceConfig cfg;
    cfg.add_buffers()->set_size_kb(1024);
    auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");

    cfg.set_write_into_file(true);
    cfg.set_file_write_period_ms(50);
    cfg.set_flush_period_ms(50);

    auto session = perfetto::Tracing::NewTrace();
    g_session = session.release(); // Keep it alive until we manually destroy it
    trace_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (trace_fd < 0) {
        std::cerr << "Failed to open trace file: " << filename << " (" << strerror(errno) << ")" << std::endl;
        return;
    }
    g_session->Setup(cfg, trace_fd);
    g_session->StartBlocking();
    std::cerr << "Perfetto tracing started: " << filename << " [fd=" << trace_fd << "]" << std::endl;
}

void perf_backend_stop() {
    if (g_session) {
        std::cerr << "Flushing and stopping Perfetto tracing session..." << std::endl;
        g_session->FlushBlocking();
        g_session->StopBlocking();
        
        delete g_session;
        g_session = nullptr;
    } else {
        std::cerr << "Warning: perf_backend_stop called but g_session is NULL" << std::endl;
    }
    if (trace_fd >= 0) {
        fsync(trace_fd);
        if (close(trace_fd) == 0) {
            std::cerr << "Trace file closed [fd=" << trace_fd << "]" << std::endl;
        } else {
            std::cerr << "Failed to close trace file [fd=" << trace_fd << "]: " << strerror(errno) << std::endl;
        }
        trace_fd = -1;
    }
    std::cerr << "Perfetto tracing stopped." << std::endl;
}

void perf_backend_on_func_enter(void* this_fn, void* /*call_site*/) {
    const char* raw = resolve_sym(this_fn);
    if (raw) {
        int status;
        char* demangled = abi::__cxa_demangle(raw, nullptr, nullptr, &status);
        if (status == 0 && demangled) {
            TRACE_EVENT_BEGIN("function_call", perfetto::DynamicString{demangled});
            free(demangled);
        } else {
            TRACE_EVENT_BEGIN("function_call", perfetto::DynamicString{raw});
        }
    } else {
        TRACE_EVENT_BEGIN("function_call", "unknown_function");
    }
}

void perf_backend_on_func_exit(void* /*this_fn*/, void* /*call_site*/) {
    TRACE_EVENT_END("function_call");
}

void perf_backend_trace_delay_start(const char* name) {
    TRACE_EVENT_BEGIN("delay", perfetto::DynamicString{name});
}

void perf_backend_trace_delay_end() {
    TRACE_EVENT_END("delay");
}

void perf_backend_trace_net_start(const char* name) {
    TRACE_EVENT_BEGIN("net", perfetto::DynamicString{name});
}

void perf_backend_trace_net_end() {
    TRACE_EVENT_END("net");
}

}
