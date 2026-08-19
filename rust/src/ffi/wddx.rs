//! C-ABI WDDX packet handling.
//!
//! Port of the exported `wddx_*` API from `src/wddx.c`. The safe Rust `Wddx`
//! tree is the source of truth; a C-layout node graph is materialized lazily
//! (and cached) whenever the FFI must hand out stable node pointers, matching
//! the C library's behaviour where node pointers stay valid for the lifetime of
//! the packet.

use std::cell::RefCell;
use std::ffi::{c_char, c_int, c_void, CStr, CString};
use std::mem::size_of;
use std::ptr;

use crate::ffi::cstr_to_str;
use crate::wddx::{format_g, is_string_numeric, split_segment, Wddx, WddxNode};

/// WDDX node type enum values (mirror the C `wddx_type`).
pub(crate) const WDDX_NULL: c_int = 0;
pub(crate) const WDDX_BOOLEAN: c_int = 1;
pub(crate) const WDDX_NUMBER: c_int = 2;
pub(crate) const WDDX_STRING: c_int = 3;
pub(crate) const WDDX_ARRAY: c_int = 4;
pub(crate) const WDDX_STRUCT: c_int = 5;

/// Maximum accepted array length (mirrors `WDDX_MAX_ARRAY_LENGTH`).
#[allow(dead_code)]
const WDDX_MAX_ARRAY_LENGTH: usize = 10_000;

/// C-layout WDDX node header; the type-specific payload follows inline.
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub(crate) struct WNode {
    node_type: c_int,
    cnt: c_int,
}

/// A single struct field: name pointer plus value node pointer.
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub(crate) struct WStructItem {
    name: *mut c_char,
    value: *mut WNode,
}

/// Size of the fixed header preceding every node's inline payload.
const NODE_TRAILING: usize = size_of::<WNode>();

/// Lazily-materialized C-layout tree for an `FfiWddx` packet.
struct Ctree {
    /// Arena of node/name buffers; dropped together with the packet.
    arena: Vec<Box<[u8]>>,
    header: *mut WNode,
    data: *mut WNode,
    xml: Option<CString>,
}

impl Ctree {
    fn new() -> Ctree {
        Ctree {
            arena: Vec::new(),
            header: ptr::null_mut(),
            data: ptr::null_mut(),
            xml: None,
        }
    }

    fn invalidate(&mut self) {
        self.arena.clear();
        self.header = ptr::null_mut();
        self.data = ptr::null_mut();
        self.xml = None;
    }
}

/// The `WDDX` handle type. `header`/`data`/`arena` are a lazy mirror of `rust`.
pub(crate) struct FfiWddx {
    pub(crate) rust: Wddx,
    ctree: RefCell<Ctree>,
}

impl FfiWddx {
    /// Wraps a safe Rust WDDX packet.
    pub(crate) fn from_rust(rust: Wddx) -> FfiWddx {
        FfiWddx {
            rust,
            ctree: RefCell::new(Ctree::new()),
        }
    }

    /// Materializes the C-layout tree from the Rust tree if not already done.
    fn ensure_ctree(&self) {
        let mut ctree = self.ctree.borrow_mut();
        if !ctree.arena.is_empty() {
            return;
        }
        if let Some(header) = &self.rust.header {
            ctree.header = rust_to_ctree(&mut ctree, header);
        }
        if let Some(data) = &self.rust.data {
            ctree.data = rust_to_ctree(&mut ctree, data);
        }
    }

    /// Invalidates the cached C tree (called before mutating the Rust tree).
    fn invalidate(&self) {
        self.ctree.borrow_mut().invalidate();
    }
}

/// Computes the inline payload length for a node, given its type and count.
fn trailing_len(node: *const WNode) -> usize {
    unsafe {
        match (*node).node_type {
            WDDX_NULL => 0,
            WDDX_BOOLEAN | WDDX_STRING => (*node).cnt as usize + 1,
            WDDX_NUMBER => 8 + (*node).cnt as usize + 1,
            WDDX_ARRAY => (*node).cnt as usize * size_of::<*mut WNode>(),
            WDDX_STRUCT => (*node).cnt as usize * size_of::<WStructItem>(),
            _ => 0,
        }
    }
}

/// Allocates a zeroed node buffer with the given inline payload size.
fn alloc_node(ctree: &mut Ctree, node_type: c_int, trailing: usize) -> *mut WNode {
    let total = NODE_TRAILING + trailing;
    let mut buf = vec![0u8; total].into_boxed_slice();
    let node = buf.as_mut_ptr() as *mut WNode;
    unsafe {
        (*node).node_type = node_type;
        (*node).cnt = 0;
    }
    ctree.arena.push(buf);
    node
}

/// Allocates a zeroed, NUL-terminated name buffer.
fn alloc_name(ctree: &mut Ctree, s: &str) -> *mut c_char {
    let mut buf = vec![0u8; s.len() + 1].into_boxed_slice();
    buf[..s.len()].copy_from_slice(s.as_bytes());
    let p = buf.as_mut_ptr() as *mut c_char;
    ctree.arena.push(buf);
    p
}

/// Returns the inline payload as a mutable byte slice.
fn trailing_mut(node: *mut WNode) -> &'static mut [u8] {
    unsafe {
        std::slice::from_raw_parts_mut((node as *mut u8).add(NODE_TRAILING), trailing_len(node))
    }
}

/// Writes an inline string (setting `cnt` to the byte length).
fn set_string(node: *mut WNode, s: &str) {
    unsafe {
        (*node).cnt = s.len() as c_int;
    }
    let buf = trailing_mut(node);
    buf[..s.len()].copy_from_slice(s.as_bytes());
    buf[s.len()] = 0;
}

/// Writes a number as both an `f64` and its `%.16g` string form.
fn set_number(node: *mut WNode, value: f64, s: &str) {
    unsafe {
        (*node).cnt = s.len() as c_int;
    }
    let buf = trailing_mut(node);
    unsafe {
        *(buf.as_mut_ptr() as *mut f64) = value;
    }
    buf[8..8 + s.len()].copy_from_slice(s.as_bytes());
    buf[8 + s.len()] = 0;
}

/// Returns a pointer to the inline string of a node.
fn node_string_ptr(node: *const WNode) -> *const c_char {
    unsafe {
        let base = (node as *const u8).add(NODE_TRAILING);
        match (*node).node_type {
            WDDX_NUMBER => base.add(8) as *const c_char,
            _ => base as *const c_char,
        }
    }
}

/// Returns the `f64` stored at the start of a number node's payload.
fn node_number(node: *const WNode) -> f64 {
    unsafe { *((node as *const u8).add(NODE_TRAILING) as *const f64) }
}

/// Returns the item pointer array of an array node.
fn node_items<'a>(node: *const WNode) -> &'a [*mut WNode] {
    unsafe {
        std::slice::from_raw_parts(
            (node as *const u8).add(NODE_TRAILING) as *const *mut WNode,
            (*node).cnt as usize,
        )
    }
}

/// Returns the item pointer array of an array node (mutable).
fn node_items_mut<'a>(node: *mut WNode) -> &'a mut [*mut WNode] {
    unsafe {
        std::slice::from_raw_parts_mut(
            (node as *mut u8).add(NODE_TRAILING) as *mut *mut WNode,
            (*node).cnt as usize,
        )
    }
}

/// Returns the struct field array of a struct node.
fn node_struct_items<'a>(node: *const WNode) -> &'a [WStructItem] {
    unsafe {
        std::slice::from_raw_parts(
            (node as *const u8).add(NODE_TRAILING) as *const WStructItem,
            (*node).cnt as usize,
        )
    }
}

/// Returns the struct field array of a struct node (mutable).
fn node_struct_items_mut<'a>(node: *mut WNode) -> &'a mut [WStructItem] {
    unsafe {
        std::slice::from_raw_parts_mut(
            (node as *mut u8).add(NODE_TRAILING) as *mut WStructItem,
            (*node).cnt as usize,
        )
    }
}

/// Converts a safe Rust WDDX node into the C-layout tree.
fn rust_to_ctree(ctree: &mut Ctree, node: &WddxNode) -> *mut WNode {
    match node {
        WddxNode::Null => alloc_node(ctree, WDDX_NULL, 0),
        WddxNode::Boolean(value) => {
            let s = if *value { "true" } else { "false" };
            let n = alloc_node(ctree, WDDX_BOOLEAN, s.len() + 1);
            set_string(n, s);
            n
        }
        WddxNode::Number(value) => {
            let s = format_g(*value, 16);
            let n = alloc_node(ctree, WDDX_NUMBER, 8 + s.len() + 1);
            set_number(n, *value, &s);
            n
        }
        WddxNode::String(s) => {
            let n = alloc_node(ctree, WDDX_STRING, s.len() + 1);
            set_string(n, s);
            n
        }
        WddxNode::Array(items) => {
            let n = alloc_node(ctree, WDDX_ARRAY, items.len() * size_of::<*mut WNode>());
            unsafe {
                (*n).cnt = items.len() as c_int;
            }
            for (i, item) in items.iter().enumerate() {
                if let Some(child) = item {
                    node_items_mut(n)[i] = rust_to_ctree(ctree, child);
                }
            }
            n
        }
        WddxNode::Struct(fields) => {
            let n = alloc_node(ctree, WDDX_STRUCT, fields.len() * size_of::<WStructItem>());
            unsafe {
                (*n).cnt = fields.len() as c_int;
            }
            for (i, (name, value)) in fields.iter().enumerate() {
                let item = &mut node_struct_items_mut(n)[i];
                item.name = alloc_name(ctree, name);
                item.value = match value {
                    Some(child) => rust_to_ctree(ctree, child),
                    None => ptr::null_mut(),
                };
            }
            n
        }
    }
}

/// Recursively traverses the tree following a comma-separated path.
fn c_get_var(wddx: &FfiWddx, path: &str) -> *const WNode {
    let ctree = wddx.ctree.borrow();
    c_recursively_get(ctree.data, path)
}

fn c_recursively_get(node: *const WNode, path: &str) -> *const WNode {
    if path.is_empty() {
        return node;
    }
    let (seg, rest) = split_segment(path);

    let target = if is_string_numeric(seg) {
        if node.is_null() {
            return ptr::null();
        }
        unsafe {
            if (*node).node_type != WDDX_ARRAY {
                return ptr::null();
            }
            let idx: usize = match seg.parse() {
                Ok(i) => i,
                Err(_) => return ptr::null(),
            };
            if idx >= (*node).cnt as usize {
                return ptr::null();
            }
            node_items(node)[idx]
        }
    } else {
        if node.is_null() {
            return ptr::null();
        }
        unsafe {
            if (*node).node_type != WDDX_STRUCT {
                return ptr::null();
            }
            let mut found = ptr::null();
            for item in node_struct_items(node) {
                if !item.name.is_null() && CStr::from_ptr(item.name).to_bytes() == seg.as_bytes() {
                    found = item.value;
                    break;
                }
            }
            found
        }
    };

    if rest.is_empty() {
        target
    } else {
        c_recursively_get(target, rest)
    }
}

// ---------------------------------------------------------------------------
// Exported WDDX functions
// ---------------------------------------------------------------------------

/// Inserts a boolean value into the WDDX data at the given path.
#[no_mangle]
pub unsafe extern "C" fn wddx_put_bool(
    dest: *mut FfiWddx,
    path: *const c_char,
    value: bool,
) -> bool {
    if dest.is_null() || path.is_null() {
        return false;
    }
    let wddx = &mut *dest;
    wddx.invalidate();
    wddx.rust.put_bool(&cstr_to_str(path), value)
}

/// Inserts a numeric value into the WDDX data at the given path.
#[no_mangle]
pub unsafe extern "C" fn wddx_put_number(
    dest: *mut FfiWddx,
    path: *const c_char,
    value: f64,
) -> bool {
    if dest.is_null() || path.is_null() {
        return false;
    }
    let wddx = &mut *dest;
    wddx.invalidate();
    wddx.rust.put_number(&cstr_to_str(path), value)
}

/// Parses WDDX XML into a packet, or NULL on failure.
#[no_mangle]
pub unsafe extern "C" fn wddx_from_xml(xml: *const c_char) -> *mut FfiWddx {
    if xml.is_null() {
        return ptr::null_mut();
    }
    match Wddx::from_xml(&cstr_to_str(xml)) {
        Some(wddx) => Box::into_raw(Box::new(FfiWddx::from_rust(wddx))),
        None => ptr::null_mut(),
    }
}

/// Retrieves the data root node of a WDDX packet.
#[no_mangle]
pub unsafe extern "C" fn wddx_data(src: *const c_void) -> *const WNode {
    if src.is_null() {
        return ptr::null();
    }
    let wddx = &*(src as *const FfiWddx);
    wddx.ensure_ctree();
    wddx.ctree.borrow().data
}

/// Returns the type of a WDDX node.
#[no_mangle]
pub unsafe extern "C" fn wddx_node_type(value: *const c_void) -> c_int {
    if value.is_null() {
        return WDDX_NULL;
    }
    (*(value as *const WNode)).node_type
}

/// Extracts the string pointer from a WDDX_STRING node.
#[no_mangle]
pub unsafe extern "C" fn wddx_node_string(value: *const WNode) -> *const c_char {
    if value.is_null() {
        return ptr::null();
    }
    node_string_ptr(value)
}

/// Returns the element count of a node.
///
/// Mirrors the C implementation, which returns the node's `cnt` field without
/// a type check (callers pass array nodes).
#[no_mangle]
pub unsafe extern "C" fn wddx_node_array_size(value: *const c_void) -> c_int {
    if value.is_null() {
        return 0;
    }
    (*(value as *const WNode)).cnt
}

/// Retrieves an element of a node by index.
///
/// Mirrors the C implementation, which bounds-checks `cnt` without a type
/// check (callers pass array nodes).
#[no_mangle]
pub unsafe extern "C" fn wddx_node_array_at(value: *const c_void, cnt: usize) -> *const WNode {
    if value.is_null() {
        return ptr::null();
    }
    let node = value as *const WNode;
    if cnt >= (*node).cnt as usize {
        return ptr::null();
    }
    node_items(node)[cnt]
}

/// Returns the number of fields of a node.
///
/// Mirrors the C implementation, which returns the node's `cnt` field without
/// a type check (callers pass struct nodes).
#[no_mangle]
pub unsafe extern "C" fn wddx_node_struct_size(value: *const c_void) -> c_int {
    if value.is_null() {
        return 0;
    }
    (*(value as *const WNode)).cnt
}

/// Retrieves a structural field of a node by index.
///
/// Mirrors the C implementation, which bounds-checks `cnt` without a type
/// check (callers pass struct nodes).
#[no_mangle]
pub unsafe extern "C" fn wddx_node_struct_at(
    value: *const c_void,
    cnt: usize,
    name: *mut *const c_char,
) -> *const WNode {
    if value.is_null() {
        return ptr::null();
    }
    let node = value as *const WNode;
    if cnt >= (*node).cnt as usize {
        return ptr::null();
    }
    let item = &node_struct_items(node)[cnt];
    if !name.is_null() {
        *name = item.name;
    }
    item.value
}

/// Query helper to retrieve a number at a path.
#[no_mangle]
pub unsafe extern "C" fn wddx_get_number(
    src: *const c_void,
    path: *const c_char,
    ok: *mut bool,
) -> f64 {
    if src.is_null() {
        if !ok.is_null() {
            *ok = false;
        }
        return 0.0;
    }
    let wddx = &*(src as *const FfiWddx);
    wddx.ensure_ctree();
    let node = c_get_var(wddx, &cstr_to_str(path));
    if node.is_null() || (*node).node_type != WDDX_NUMBER {
        if !ok.is_null() {
            *ok = false;
        }
        return 0.0;
    }
    if !ok.is_null() {
        *ok = true;
    }
    node_number(node)
}

/// Query helper to retrieve a string at a path.
#[no_mangle]
pub unsafe extern "C" fn wddx_get_string(src: *const c_void, path: *const c_char) -> *const c_char {
    if src.is_null() {
        return ptr::null();
    }
    let wddx = &*(src as *const FfiWddx);
    wddx.ensure_ctree();
    let node = c_get_var(wddx, &cstr_to_str(path));
    if node.is_null() || (*node).node_type != WDDX_STRING {
        return ptr::null();
    }
    node_string_ptr(node)
}

/// Query helper to retrieve a node at a path.
#[no_mangle]
pub unsafe extern "C" fn wddx_get_var(src: *const c_void, path: *const c_char) -> *const WNode {
    if src.is_null() {
        return ptr::null();
    }
    let wddx = &*(src as *const FfiWddx);
    wddx.ensure_ctree();
    c_get_var(wddx, &cstr_to_str(path))
}

/// Recursively frees a WDDX packet and nullifies the caller's pointer.
#[no_mangle]
pub unsafe extern "C" fn wddx_cleanup(value: *mut c_void) {
    if value.is_null() {
        return;
    }
    let handle = value as *mut *mut FfiWddx;
    if !(*handle).is_null() {
        drop(Box::from_raw(*handle));
        *handle = ptr::null_mut();
    }
}

// ---------------------------------------------------------------------------
// Safe wrappers over the exported functions, for use by sibling FFI modules.
// ---------------------------------------------------------------------------

pub(crate) fn wddx_data_safe(src: *const c_void) -> *const WNode {
    unsafe { wddx_data(src) }
}

pub(crate) fn wddx_get_string_safe(src: *const c_void, path: *const c_char) -> *const c_char {
    unsafe { wddx_get_string(src, path) }
}

pub(crate) fn wddx_get_var_safe(src: *const c_void, path: *const c_char) -> *const WNode {
    unsafe { wddx_get_var(src, path) }
}

pub(crate) fn wddx_get_number_safe(src: *const c_void, path: *const c_char, ok: *mut bool) -> f64 {
    unsafe { wddx_get_number(src, path, ok) }
}

pub(crate) fn wddx_node_type_safe(value: *const c_void) -> c_int {
    unsafe { wddx_node_type(value) }
}

pub(crate) fn wddx_node_string_safe(value: *const WNode) -> *const c_char {
    unsafe { wddx_node_string(value) }
}

pub(crate) fn wddx_node_array_size_safe(value: *const c_void) -> c_int {
    unsafe { wddx_node_array_size(value) }
}

pub(crate) fn wddx_node_array_at_safe(value: *const c_void, cnt: usize) -> *const WNode {
    unsafe { wddx_node_array_at(value, cnt) }
}

pub(crate) fn wddx_node_struct_at_safe(
    value: *const c_void,
    cnt: usize,
    name: *mut *const c_char,
) -> *const WNode {
    unsafe { wddx_node_struct_at(value, cnt, name) }
}

pub(crate) fn wddx_node_struct_size_safe(value: *const c_void) -> c_int {
    unsafe { wddx_node_struct_size(value) }
}
