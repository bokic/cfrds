//! WDDX packet parsing and serialization.
//!
//! Port of `src/wddx.c` from the C library. WDDX is the XML wire format used by
//! the RDS protocol. Parsing is delegated to `roxmltree`; serialization is
//! performed directly to a string, mirroring the libxml2 `xmlNodeDump` output.

use std::fmt::Write;

/// Maximum length accepted for WDDX array nodes (mirrors `WDDX_MAX_ARRAY_LENGTH`).
const WDDX_MAX_ARRAY_LENGTH: usize = 10_000;

/// The WDDX data type of a node.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum WddxType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Struct,
}

/// A single WDDX value node.
///
/// Array slots and struct values are `Option`s to faithfully mirror the C
/// representation, where partially-built trees may contain null children.
#[derive(Debug, Clone, PartialEq)]
pub enum WddxNode {
    Null,
    Boolean(bool),
    Number(f64),
    String(String),
    Array(Vec<Option<WddxNode>>),
    Struct(Vec<(String, Option<WddxNode>)>),
}

impl WddxNode {
    /// Returns the type of this node.
    pub fn node_type(&self) -> WddxType {
        match self {
            WddxNode::Null => WddxType::Null,
            WddxNode::Boolean(_) => WddxType::Boolean,
            WddxNode::Number(_) => WddxType::Number,
            WddxNode::String(_) => WddxType::String,
            WddxNode::Array(_) => WddxType::Array,
            WddxNode::Struct(_) => WddxType::Struct,
        }
    }

    /// Returns the string content of a `WddxNode::String` node.
    pub fn node_string(&self) -> Option<&str> {
        match self {
            WddxNode::String(s) => Some(s),
            _ => None,
        }
    }

    /// Returns the element count of an array node.
    pub fn node_array_size(&self) -> usize {
        match self {
            WddxNode::Array(items) => items.len(),
            _ => 0,
        }
    }

    /// Returns the element at `idx` of an array node.
    pub fn node_array_at(&self, idx: usize) -> Option<&WddxNode> {
        match self {
            WddxNode::Array(items) => items.get(idx).and_then(|x| x.as_ref()),
            _ => None,
        }
    }

    /// Returns the field count of a struct node.
    pub fn node_struct_size(&self) -> usize {
        match self {
            WddxNode::Struct(fields) => fields.len(),
            _ => 0,
        }
    }

    /// Returns the `(name, value)` field at `idx` of a struct node.
    pub fn node_struct_at(&self, idx: usize) -> Option<(&str, &WddxNode)> {
        match self {
            WddxNode::Struct(fields) => {
                let (name, value) = fields.get(idx)?;
                Some((name.as_str(), value.as_ref()?))
            }
            _ => None,
        }
    }
}

/// A parsed WDDX packet containing an optional header and data tree.
#[derive(Debug, Clone, PartialEq, Default)]
pub struct Wddx {
    pub(crate) header: Option<WddxNode>,
    pub(crate) data: Option<WddxNode>,
}

impl Wddx {
    /// Creates a new, empty WDDX packet.
    pub fn new() -> Wddx {
        Wddx {
            header: None,
            data: None,
        }
    }

    /// Returns a reference to the data root node, if any.
    pub fn data(&self) -> Option<&WddxNode> {
        self.data.as_ref()
    }

    /// Returns a reference to the header root node, if any.
    pub fn header(&self) -> Option<&WddxNode> {
        self.header.as_ref()
    }

    /// Inserts a boolean value at `path`.
    pub fn put_bool(&mut self, path: &str, value: bool) -> bool {
        self.put(
            path,
            if value { "true" } else { "false" },
            WddxType::Boolean,
        )
    }

    /// Inserts a number at `path`.
    pub fn put_number(&mut self, path: &str, value: f64) -> bool {
        self.put(path, &format_g(value, 16), WddxType::Number)
    }

    /// Inserts a string at `path`.
    pub fn put_string(&mut self, path: &str, value: &str) -> bool {
        self.put(path, value, WddxType::String)
    }

    fn put(&mut self, path: &str, value: &str, ty: WddxType) -> bool {
        let mut node = self.data.take();
        if !recursively_put(&mut node, path, value, ty) {
            self.data = node;
            return false;
        }
        let ok = node.is_some();
        self.data = node;
        ok
    }

    /// Serializes this packet to WDDX 1.0 XML.
    pub fn to_xml(&self) -> String {
        let mut out = String::new();
        out.push_str(r#"<wddxPacket version="1.0">"#);
        if let Some(header) = &self.header {
            out.push_str("<header>");
            render_node(&mut out, header);
            out.push_str("</header>");
        } else {
            out.push_str("<header/>");
        }
        if let Some(data) = &self.data {
            out.push_str("<data>");
            render_node(&mut out, data);
            out.push_str("</data>");
        } else {
            out.push_str("<data/>");
        }
        out.push_str("</wddxPacket>");
        out
    }

    /// Parses WDDX 1.0 XML into a packet, or `None` on failure.
    pub fn from_xml(xml: &str) -> Option<Wddx> {
        if xml.is_empty() {
            return None;
        }
        let doc = roxmltree::Document::parse(xml).ok()?;
        let root = doc.root_element();
        if root.tag_name().name() != "wddxPacket" {
            return None;
        }

        let elements: Vec<roxmltree::Node> = root.children().filter(|n| n.is_element()).collect();
        if elements.len() < 2 {
            return None;
        }
        if elements[0].tag_name().name() != "header" || elements[1].tag_name().name() != "data" {
            return None;
        }

        let mut wddx = Wddx::new();
        if let Some(child) = elements[0].first_child() {
            wddx.header = element_to_node(child);
        }
        if let Some(child) = elements[1].first_child() {
            wddx.data = element_to_node(child);
        }
        Some(wddx)
    }

    /// Retrieves the node at the given comma-separated path from the data tree.
    pub fn get_var(&self, path: &str) -> Option<&WddxNode> {
        recursively_get(self.data.as_ref(), path)
    }

    /// Retrieves a `WddxNode::Number` at `path`.
    pub fn get_number(&self, path: &str) -> Option<f64> {
        match self.get_var(path) {
            Some(WddxNode::Number(n)) => Some(*n),
            _ => None,
        }
    }

    /// Retrieves a `WddxNode::String` at `path`.
    pub fn get_string(&self, path: &str) -> Option<&str> {
        match self.get_var(path) {
            Some(WddxNode::String(s)) => Some(s),
            _ => None,
        }
    }
}

/// Splits a comma-separated path into its first segment and the remainder.
pub(crate) fn split_segment(path: &str) -> (&str, &str) {
    match path.find(',') {
        Some(pos) => (&path[..pos], &path[pos + 1..]),
        None => (path, ""),
    }
}

/// Checks whether a path segment consists entirely of ASCII digits.
pub(crate) fn is_string_numeric(seg: &str) -> bool {
    !seg.is_empty() && seg.len() <= 20 && seg.bytes().all(|b| b.is_ascii_digit())
}

/// Recursively places a leaf value into the tree at `path`.
///
/// Mirrors `wddx_recursively_put` from the C library. Numeric path segments
/// index arrays; other segments address struct fields. Returns `false` on a
/// type mismatch (in which case the tree is left as close to unchanged as the
/// C implementation leaves it).
fn recursively_put(node: &mut Option<WddxNode>, path: &str, value: &str, ty: WddxType) -> bool {
    if path.is_empty() {
        *node = Some(make_leaf(ty, value));
        return true;
    }

    let (seg, rest) = split_segment(path);
    if seg.is_empty() {
        return false;
    }

    if is_string_numeric(seg) {
        let parsed: i64 = seg.parse().unwrap_or(-1);
        if parsed < 0 || parsed >= WDDX_MAX_ARRAY_LENGTH as i64 {
            return false;
        }
        let idx = (parsed as usize) + 1;
        if !(1..=WDDX_MAX_ARRAY_LENGTH).contains(&idx) {
            return false;
        }

        if node.is_none() {
            *node = Some(WddxNode::Array(vec![None; idx]));
        }

        match node {
            Some(WddxNode::Array(items)) => {
                if items.len() < idx {
                    items.resize(idx, None);
                }
                let child_is_some = items[idx - 1].is_some();
                if !recursively_put(&mut items[idx - 1], rest, value, ty) && child_is_some {
                    return false;
                }
                true
            }
            _ => false,
        }
    } else {
        if node.is_none() {
            *node = Some(WddxNode::Struct(Vec::new()));
        }

        match node {
            Some(WddxNode::Struct(fields)) => {
                if let Some(pos) = fields.iter().position(|(k, _)| k == seg) {
                    let child_is_some = fields[pos].1.is_some();
                    if !recursively_put(&mut fields[pos].1, rest, value, ty) && child_is_some {
                        return false;
                    }
                    true
                } else {
                    let mut val = None;
                    if !recursively_put(&mut val, rest, value, ty) {
                        return false;
                    }
                    fields.push((seg.to_string(), val));
                    true
                }
            }
            _ => false,
        }
    }
}

/// Creates a leaf node of the given type holding `value`.
fn make_leaf(ty: WddxType, value: &str) -> WddxNode {
    match ty {
        WddxType::Boolean => WddxNode::Boolean(value == "true"),
        WddxType::Number => WddxNode::Number(value.parse().unwrap_or(0.0)),
        WddxType::String => WddxNode::String(value.to_string()),
        WddxType::Null => WddxNode::Null,
        WddxType::Array => WddxNode::Array(Vec::new()),
        WddxType::Struct => WddxNode::Struct(Vec::new()),
    }
}

/// Converts an XML element node into a WDDX node, mirroring
/// `wddx_from_xml_element` from the C library.
fn element_to_node(node: roxmltree::Node) -> Option<WddxNode> {
    if !node.is_element() {
        return None;
    }
    match node.tag_name().name() {
        "null" => Some(WddxNode::Null),
        "boolean" => {
            let value = node.attribute("value")?;
            Some(WddxNode::Boolean(value != "false"))
        }
        "number" => {
            let text = node.text()?;
            let parsed: f64 = text.parse().ok()?;
            Some(WddxNode::Number(parsed))
        }
        "string" => Some(WddxNode::String(node.text().unwrap_or("").to_string())),
        "array" => {
            let length: i64 = node.attribute("length")?.parse().ok()?;
            if length <= 0 || length > WDDX_MAX_ARRAY_LENGTH as i64 {
                return None;
            }
            let mut items = Vec::new();
            for child in node.children().filter(|n| n.is_element()) {
                if items.len() >= length as usize {
                    break;
                }
                items.push(element_to_node(child));
            }
            Some(WddxNode::Array(items))
        }
        "struct" => {
            let mut fields = Vec::new();
            for child in node
                .children()
                .filter(|n| n.is_element() && n.tag_name().name() == "var")
            {
                child.first_child()?;
                let key = child.attribute("name")?;
                let value = child.first_child().and_then(element_to_node);
                fields.push((key.to_string(), value));
            }
            Some(WddxNode::Struct(fields))
        }
        _ => None,
    }
}

/// Recursively traverses the tree following a comma-separated path.
fn recursively_get<'a>(node: Option<&'a WddxNode>, path: &str) -> Option<&'a WddxNode> {
    if path.is_empty() {
        return node;
    }
    let (seg, rest) = split_segment(path);
    let target = if is_string_numeric(seg) {
        let idx: usize = seg.parse().ok()?;
        match node? {
            WddxNode::Array(items) => items.get(idx).and_then(|x| x.as_ref()),
            _ => return None,
        }
    } else {
        match node? {
            WddxNode::Struct(fields) => fields
                .iter()
                .find(|(k, _)| k == seg)
                .and_then(|(_, v)| v.as_ref()),
            _ => return None,
        }
    };
    if rest.is_empty() {
        target
    } else {
        recursively_get(target, rest)
    }
}

/// Serializes a node into WDDX XML, mirroring `wddx_to_xml_node` and the
/// libxml2 `xmlNodeDump` text escaping behaviour.
fn render_node(out: &mut String, node: &WddxNode) {
    match node {
        WddxNode::Null => out.push_str("<null/>"),
        WddxNode::Boolean(value) => {
            let v = if *value { "true" } else { "false" };
            let _ = write!(out, r#"<boolean value="{v}"/>"#);
        }
        WddxNode::Number(n) => {
            let _ = write!(out, "<number>{}</number>", format_g(*n, 16));
        }
        WddxNode::String(s) => {
            out.push_str("<string>");
            out.push_str(&escape_text(s));
            out.push_str("</string>");
        }
        WddxNode::Array(items) => {
            let _ = write!(out, r#"<array length="{}">"#, items.len());
            for child in items.iter().flatten() {
                render_node(out, child);
            }
            out.push_str("</array>");
        }
        WddxNode::Struct(fields) => {
            out.push_str(r#"<struct type="java.util.HashMap">"#);
            for (name, value) in fields {
                out.push_str("<var name=\"");
                out.push_str(&escape_attr(name));
                out.push_str("\">");
                if let Some(child) = value {
                    render_node(out, child);
                }
                out.push_str("</var>");
            }
            out.push_str("</struct>");
        }
    }
}

/// Escapes a string for use as XML text content.
fn escape_text(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    for ch in s.chars() {
        match ch {
            '&' => out.push_str("&amp;"),
            '<' => out.push_str("&lt;"),
            '>' => out.push_str("&gt;"),
            _ => out.push(ch),
        }
    }
    out
}

/// Escapes a string for use as an XML attribute value.
fn escape_attr(s: &str) -> String {
    escape_text(s).replace('"', "&quot;")
}

/// Formats a double using C-style `%.Ng` semantics (used by `%.16g` in the C
/// library when serializing numbers).
pub(crate) fn format_g(value: f64, precision: usize) -> String {
    if value.is_nan() {
        return "nan".to_string();
    }
    if value.is_infinite() {
        return if value > 0.0 {
            "inf".to_string()
        } else {
            "-inf".to_string()
        };
    }
    if value == 0.0 {
        return if value.is_sign_negative() {
            "-0".to_string()
        } else {
            "0".to_string()
        };
    }

    let neg = value < 0.0;
    let abs = value.abs();
    let e = abs.log10().floor() as i64;
    let use_exp = e < -4 || e >= precision as i64;

    let s = if use_exp {
        let p = precision.saturating_sub(1);
        let mantissa = format!("{:.*}", p, abs / 10f64.powi(e as i32));
        let esign = if e < 0 { '-' } else { '+' };
        let edigits = e.unsigned_abs();
        let mut mantissa = mantissa;
        if let Some(_dot) = mantissa.find('.') {
            while mantissa.ends_with('0') {
                mantissa.pop();
            }
            if mantissa.ends_with('.') {
                mantissa.pop();
            }
        }
        format!("{}e{}{:02}", mantissa, esign, edigits)
    } else {
        let decimals = (precision as i64 - 1 - e) as usize;
        format!("{:.*}", decimals, abs)
    };

    // Trim trailing zeros from the formatted digits. The scientific branch
    // always contains an 'e'; the fixed branch never does.
    let s = match s.split_once('e') {
        Some((mant, exp)) => {
            let mut mant = mant.to_string();
            while mant.ends_with('0') {
                mant.pop();
            }
            if mant.ends_with('.') {
                mant.pop();
            }
            format!("{}e{}", mant, exp)
        }
        None => {
            let mut s = s;
            while s.ends_with('0') {
                s.pop();
            }
            if s.ends_with('.') {
                s.pop();
            }
            s
        }
    };

    if neg {
        format!("-{}", s)
    } else {
        s
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn build_and_serialize_struct() {
        let mut w = Wddx::new();
        assert!(w.put_bool("0,REMOTE_SESSION", true));
        let xml = w.to_xml();
        assert_eq!(
            xml,
            r#"<wddxPacket version="1.0"><header/><data><array length="1"><struct type="java.util.HashMap"><var name="REMOTE_SESSION"><boolean value="true"/></var></struct></array></data></wddxPacket>"#
        );
    }

    #[test]
    fn parse_roundtrip() {
        let xml = r#"<wddxPacket version="1.0"><header/><data><struct type="java.util.HashMap"><var name="STATUS"><string>RDS_OK</string></var><var name="DEBUG_SERVER_PORT"><number>8501</number></var></struct></data></wddxPacket>"#;
        let w = Wddx::from_xml(xml).expect("should parse");
        assert_eq!(w.get_string("STATUS"), Some("RDS_OK"));
        assert_eq!(w.get_number("DEBUG_SERVER_PORT"), Some(8501.0));
    }

    #[test]
    fn parse_nested_path() {
        let xml = r#"<wddxPacket version="1.0"><header/><data><array length="1"><struct type="java.util.HashMap"><var name="EVENT"><string>BREAKPOINT</string></var></struct></array></data></wddxPacket>"#;
        let w = Wddx::from_xml(xml).expect("should parse");
        assert_eq!(w.get_string("0,EVENT"), Some("BREAKPOINT"));
        let root = w.data().unwrap();
        assert_eq!(root.node_type(), WddxType::Array);
        assert_eq!(root.node_array_size(), 1);
    }

    #[test]
    fn parse_boolean_false() {
        let xml = r#"<wddxPacket version="1.0"><header/><data><boolean value="false"/></data></wddxPacket>"#;
        let w = Wddx::from_xml(xml).expect("should parse");
        let node = w.data().unwrap();
        assert_eq!(node.node_type(), WddxType::Boolean);
        assert_eq!(node, &WddxNode::Boolean(false));
    }

    #[test]
    fn put_number_formats_integers() {
        let mut w = Wddx::new();
        assert!(w.put_number("0,Y", 5.0));
        let xml = w.to_xml();
        assert!(xml.contains("<number>5</number>"));
    }

    #[test]
    fn format_g_values() {
        assert_eq!(format_g(5.0, 16), "5");
        assert_eq!(format_g(0.0, 16), "0");
        assert_eq!(format_g(1.5, 16), "1.5");
        assert_eq!(format_g(1e20, 16), "1e+20");
        assert_eq!(format_g(0.1, 16), "0.1");
    }
}
