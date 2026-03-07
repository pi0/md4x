//! Raw FFI bindings to md4x.
//!
//! Prefer the safe [`md4x`] crate over using this directly.

#![allow(non_camel_case_types, non_snake_case, dead_code, clippy::upper_case_acronyms)]

use std::os::raw::{c_char, c_int, c_uint, c_void};

// ---------------------------------------------------------------------------
// Basic types
// ---------------------------------------------------------------------------

pub type MD_CHAR = c_char;
pub type MD_SIZE = c_uint;
pub type MD_OFFSET = c_uint;

// ---------------------------------------------------------------------------
// Block types
// ---------------------------------------------------------------------------

pub type MD_BLOCKTYPE = c_int;

pub const MD_BLOCK_DOC: MD_BLOCKTYPE = 0;
pub const MD_BLOCK_QUOTE: MD_BLOCKTYPE = 1;
pub const MD_BLOCK_UL: MD_BLOCKTYPE = 2;
pub const MD_BLOCK_OL: MD_BLOCKTYPE = 3;
pub const MD_BLOCK_LI: MD_BLOCKTYPE = 4;
pub const MD_BLOCK_HR: MD_BLOCKTYPE = 5;
pub const MD_BLOCK_H: MD_BLOCKTYPE = 6;
pub const MD_BLOCK_CODE: MD_BLOCKTYPE = 7;
pub const MD_BLOCK_HTML: MD_BLOCKTYPE = 8;
pub const MD_BLOCK_P: MD_BLOCKTYPE = 9;
pub const MD_BLOCK_TABLE: MD_BLOCKTYPE = 10;
pub const MD_BLOCK_THEAD: MD_BLOCKTYPE = 11;
pub const MD_BLOCK_TBODY: MD_BLOCKTYPE = 12;
pub const MD_BLOCK_TR: MD_BLOCKTYPE = 13;
pub const MD_BLOCK_TH: MD_BLOCKTYPE = 14;
pub const MD_BLOCK_TD: MD_BLOCKTYPE = 15;
pub const MD_BLOCK_FRONTMATTER: MD_BLOCKTYPE = 16;
pub const MD_BLOCK_COMPONENT: MD_BLOCKTYPE = 17;
pub const MD_BLOCK_TEMPLATE: MD_BLOCKTYPE = 18;
pub const MD_BLOCK_ALERT: MD_BLOCKTYPE = 19;

// ---------------------------------------------------------------------------
// Span types
// ---------------------------------------------------------------------------

pub type MD_SPANTYPE = c_int;

pub const MD_SPAN_EM: MD_SPANTYPE = 0;
pub const MD_SPAN_STRONG: MD_SPANTYPE = 1;
pub const MD_SPAN_A: MD_SPANTYPE = 2;
pub const MD_SPAN_IMG: MD_SPANTYPE = 3;
pub const MD_SPAN_CODE: MD_SPANTYPE = 4;
pub const MD_SPAN_DEL: MD_SPANTYPE = 5;
pub const MD_SPAN_LATEXMATH: MD_SPANTYPE = 6;
pub const MD_SPAN_LATEXMATH_DISPLAY: MD_SPANTYPE = 7;
pub const MD_SPAN_WIKILINK: MD_SPANTYPE = 8;
pub const MD_SPAN_U: MD_SPANTYPE = 9;
pub const MD_SPAN_COMPONENT: MD_SPANTYPE = 10;
pub const MD_SPAN_SPAN: MD_SPANTYPE = 11;

// ---------------------------------------------------------------------------
// Text types
// ---------------------------------------------------------------------------

pub type MD_TEXTTYPE = c_int;

pub const MD_TEXT_NORMAL: MD_TEXTTYPE = 0;
pub const MD_TEXT_NULLCHAR: MD_TEXTTYPE = 1;
pub const MD_TEXT_BR: MD_TEXTTYPE = 2;
pub const MD_TEXT_SOFTBR: MD_TEXTTYPE = 3;
pub const MD_TEXT_ENTITY: MD_TEXTTYPE = 4;
pub const MD_TEXT_CODE: MD_TEXTTYPE = 5;
pub const MD_TEXT_HTML: MD_TEXTTYPE = 6;
pub const MD_TEXT_LATEXMATH: MD_TEXTTYPE = 7;

// ---------------------------------------------------------------------------
// Alignment
// ---------------------------------------------------------------------------

pub type MD_ALIGN = c_int;

pub const MD_ALIGN_DEFAULT: MD_ALIGN = 0;
pub const MD_ALIGN_LEFT: MD_ALIGN = 1;
pub const MD_ALIGN_CENTER: MD_ALIGN = 2;
pub const MD_ALIGN_RIGHT: MD_ALIGN = 3;

// ---------------------------------------------------------------------------
// Parser flags
// ---------------------------------------------------------------------------

pub const MD_FLAG_COLLAPSEWHITESPACE: u32 = 0x0001;
pub const MD_FLAG_PERMISSIVEATXHEADERS: u32 = 0x0002;
pub const MD_FLAG_PERMISSIVEURLAUTOLINKS: u32 = 0x0004;
pub const MD_FLAG_PERMISSIVEEMAILAUTOLINKS: u32 = 0x0008;
pub const MD_FLAG_NOINDENTEDCODEBLOCKS: u32 = 0x0010;
pub const MD_FLAG_NOHTMLBLOCKS: u32 = 0x0020;
pub const MD_FLAG_NOHTMLSPANS: u32 = 0x0040;
pub const MD_FLAG_TABLES: u32 = 0x0100;
pub const MD_FLAG_STRIKETHROUGH: u32 = 0x0200;
pub const MD_FLAG_PERMISSIVEWWWAUTOLINKS: u32 = 0x0400;
pub const MD_FLAG_TASKLISTS: u32 = 0x0800;
pub const MD_FLAG_LATEXMATHSPANS: u32 = 0x1000;
pub const MD_FLAG_WIKILINKS: u32 = 0x2000;
pub const MD_FLAG_UNDERLINE: u32 = 0x4000;
pub const MD_FLAG_HARD_SOFT_BREAKS: u32 = 0x8000;
pub const MD_FLAG_FRONTMATTER: u32 = 0x10000;
pub const MD_FLAG_COMPONENTS: u32 = 0x20000;
pub const MD_FLAG_ATTRIBUTES: u32 = 0x40000;
pub const MD_FLAG_ALERTS: u32 = 0x80000;

pub const MD_FLAG_PERMISSIVEAUTOLINKS: u32 =
    MD_FLAG_PERMISSIVEEMAILAUTOLINKS | MD_FLAG_PERMISSIVEURLAUTOLINKS | MD_FLAG_PERMISSIVEWWWAUTOLINKS;
pub const MD_FLAG_NOHTML: u32 = MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS;

pub const MD_DIALECT_COMMONMARK: u32 = 0;
pub const MD_DIALECT_GITHUB: u32 =
    MD_FLAG_PERMISSIVEAUTOLINKS | MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS | MD_FLAG_ALERTS;
pub const MD_DIALECT_ALL: u32 = MD_FLAG_PERMISSIVEAUTOLINKS
    | MD_FLAG_TABLES
    | MD_FLAG_STRIKETHROUGH
    | MD_FLAG_TASKLISTS
    | MD_FLAG_LATEXMATHSPANS
    | MD_FLAG_WIKILINKS
    | MD_FLAG_UNDERLINE
    | MD_FLAG_FRONTMATTER
    | MD_FLAG_COMPONENTS
    | MD_FLAG_ATTRIBUTES
    | MD_FLAG_ALERTS;

// ---------------------------------------------------------------------------
// Renderer flags
// ---------------------------------------------------------------------------

pub const MD_HTML_FLAG_DEBUG: u32 = 0x0001;
pub const MD_HTML_FLAG_VERBATIM_ENTITIES: u32 = 0x0002;
pub const MD_HTML_FLAG_SKIP_UTF8_BOM: u32 = 0x0004;
pub const MD_HTML_FLAG_FULL_HTML: u32 = 0x0008;
pub const MD_HTML_FLAG_HEAL: u32 = 0x0100;

pub const MD_AST_FLAG_DEBUG: u32 = 0x0001;
pub const MD_AST_FLAG_SKIP_UTF8_BOM: u32 = 0x0002;
pub const MD_AST_FLAG_HEAL: u32 = 0x0100;

pub const MD_ANSI_FLAG_DEBUG: u32 = 0x0001;
pub const MD_ANSI_FLAG_SKIP_UTF8_BOM: u32 = 0x0002;
pub const MD_ANSI_FLAG_NO_COLOR: u32 = 0x0004;
pub const MD_ANSI_FLAG_HEAL: u32 = 0x0100;

pub const MD_TEXT_FLAG_DEBUG: u32 = 0x0001;
pub const MD_TEXT_FLAG_SKIP_UTF8_BOM: u32 = 0x0002;
pub const MD_TEXT_FLAG_HEAL: u32 = 0x0100;

pub const MD_META_FLAG_DEBUG: u32 = 0x0001;
pub const MD_META_FLAG_SKIP_UTF8_BOM: u32 = 0x0002;
pub const MD_META_FLAG_HEAL: u32 = 0x0100;

// ---------------------------------------------------------------------------
// Structs
// ---------------------------------------------------------------------------

/// String attribute — a slice of the input with substring type information.
#[repr(C)]
pub struct MD_ATTRIBUTE {
    pub text: *const MD_CHAR,
    pub size: MD_SIZE,
    pub substr_types: *const MD_TEXTTYPE,
    pub substr_offsets: *const MD_OFFSET,
}

#[repr(C)]
pub struct MD_BLOCK_UL_DETAIL {
    pub is_tight: c_int,
    pub mark: MD_CHAR,
}

#[repr(C)]
pub struct MD_BLOCK_OL_DETAIL {
    pub start: c_uint,
    pub is_tight: c_int,
    pub mark_delimiter: MD_CHAR,
}

#[repr(C)]
pub struct MD_BLOCK_LI_DETAIL {
    pub is_task: c_int,
    pub task_mark: MD_CHAR,
    pub task_mark_offset: MD_OFFSET,
}

#[repr(C)]
pub struct MD_BLOCK_H_DETAIL {
    pub level: c_uint,
}

#[repr(C)]
pub struct MD_BLOCK_CODE_DETAIL {
    pub info: MD_ATTRIBUTE,
    pub lang: MD_ATTRIBUTE,
    pub fence_char: MD_CHAR,
    pub filename: MD_ATTRIBUTE,
    pub meta: *const MD_CHAR,
    pub meta_size: MD_SIZE,
    pub highlights: *const c_uint,
    pub highlight_count: c_uint,
}

#[repr(C)]
pub struct MD_BLOCK_TABLE_DETAIL {
    pub col_count: c_uint,
    pub head_row_count: c_uint,
    pub body_row_count: c_uint,
}

#[repr(C)]
pub struct MD_BLOCK_TD_DETAIL {
    pub align: MD_ALIGN,
}

#[repr(C)]
pub struct MD_SPAN_A_DETAIL {
    pub href: MD_ATTRIBUTE,
    pub title: MD_ATTRIBUTE,
    pub raw_attrs: *const MD_CHAR,
    pub raw_attrs_size: MD_SIZE,
    pub is_autolink: c_int,
}

#[repr(C)]
pub struct MD_SPAN_IMG_DETAIL {
    pub src: MD_ATTRIBUTE,
    pub title: MD_ATTRIBUTE,
    pub raw_attrs: *const MD_CHAR,
    pub raw_attrs_size: MD_SIZE,
}

#[repr(C)]
pub struct MD_SPAN_WIKILINK_DETAIL {
    pub target: MD_ATTRIBUTE,
}

#[repr(C)]
pub struct MD_SPAN_COMPONENT_DETAIL {
    pub tag_name: MD_ATTRIBUTE,
    pub raw_props: *const MD_CHAR,
    pub raw_props_size: MD_SIZE,
}

#[repr(C)]
pub struct MD_BLOCK_COMPONENT_DETAIL {
    pub tag_name: MD_ATTRIBUTE,
    pub raw_props: *const MD_CHAR,
    pub raw_props_size: MD_SIZE,
}

#[repr(C)]
pub struct MD_BLOCK_TEMPLATE_DETAIL {
    pub name: MD_ATTRIBUTE,
}

#[repr(C)]
pub struct MD_BLOCK_ALERT_DETAIL {
    pub type_name: MD_ATTRIBUTE,
}

#[repr(C)]
pub struct MD_SPAN_ATTRS_DETAIL {
    pub raw_attrs: *const MD_CHAR,
    pub raw_attrs_size: MD_SIZE,
}

#[repr(C)]
pub struct MD_SPAN_SPAN_DETAIL {
    pub raw_attrs: *const MD_CHAR,
    pub raw_attrs_size: MD_SIZE,
}

/// SAX-like parser callbacks. Set unused callbacks to `None`.
#[repr(C)]
pub struct MD_PARSER {
    /// Reserved — set to 0.
    pub abi_version: c_uint,
    /// Bitmask of `MD_FLAG_*` constants.
    pub flags: c_uint,
    pub enter_block: Option<unsafe extern "C" fn(MD_BLOCKTYPE, *mut c_void, *mut c_void) -> c_int>,
    pub leave_block: Option<unsafe extern "C" fn(MD_BLOCKTYPE, *mut c_void, *mut c_void) -> c_int>,
    pub enter_span: Option<unsafe extern "C" fn(MD_SPANTYPE, *mut c_void, *mut c_void) -> c_int>,
    pub leave_span: Option<unsafe extern "C" fn(MD_SPANTYPE, *mut c_void, *mut c_void) -> c_int>,
    pub text: Option<unsafe extern "C" fn(MD_TEXTTYPE, *const MD_CHAR, MD_SIZE, *mut c_void) -> c_int>,
    /// Optional debug callback (may be `None`).
    pub debug_log: Option<unsafe extern "C" fn(*const c_char, *mut c_void)>,
    /// Reserved — set to `None`.
    pub syntax: Option<unsafe extern "C" fn()>,
}

/// Options for [`md_html_ex`].
#[repr(C)]
pub struct MD_HTML_OPTS {
    /// Document title override (`NULL` = use frontmatter title).
    pub title: *const c_char,
    /// CSS stylesheet URL (`NULL` = omit).
    pub css_url: *const c_char,
}

// ---------------------------------------------------------------------------
// C functions
// ---------------------------------------------------------------------------

extern "C" {
    /// Core SAX-like Markdown parser. Returns 0 on success, -1 on error, or
    /// the non-zero return value of a callback that aborted parsing.
    pub fn md_parse(
        text: *const MD_CHAR,
        size: MD_SIZE,
        parser: *const MD_PARSER,
        userdata: *mut c_void,
    ) -> c_int;

    /// Render Markdown to HTML body content.
    pub fn md_html(
        input: *const MD_CHAR,
        input_size: MD_SIZE,
        process_output: unsafe extern "C" fn(*const MD_CHAR, MD_SIZE, *mut c_void),
        userdata: *mut c_void,
        parser_flags: c_uint,
        renderer_flags: c_uint,
    ) -> c_int;

    /// Extended HTML renderer (supports `MD_HTML_FLAG_FULL_HTML`).
    pub fn md_html_ex(
        input: *const MD_CHAR,
        input_size: MD_SIZE,
        process_output: unsafe extern "C" fn(*const MD_CHAR, MD_SIZE, *mut c_void),
        userdata: *mut c_void,
        parser_flags: c_uint,
        renderer_flags: c_uint,
        opts: *const MD_HTML_OPTS,
    ) -> c_int;

    /// Render Markdown to Comark JSON AST.
    pub fn md_ast(
        input: *const MD_CHAR,
        input_size: MD_SIZE,
        process_output: unsafe extern "C" fn(*const MD_CHAR, MD_SIZE, *mut c_void),
        userdata: *mut c_void,
        parser_flags: c_uint,
        renderer_flags: c_uint,
    ) -> c_int;

    /// Render Markdown to ANSI terminal output.
    pub fn md_ansi(
        input: *const MD_CHAR,
        input_size: MD_SIZE,
        process_output: unsafe extern "C" fn(*const MD_CHAR, MD_SIZE, *mut c_void),
        userdata: *mut c_void,
        parser_flags: c_uint,
        renderer_flags: c_uint,
    ) -> c_int;

    /// Render Markdown to plain text.
    pub fn md_text(
        input: *const MD_CHAR,
        input_size: MD_SIZE,
        process_output: unsafe extern "C" fn(*const MD_CHAR, MD_SIZE, *mut c_void),
        userdata: *mut c_void,
        parser_flags: c_uint,
        renderer_flags: c_uint,
    ) -> c_int;

    /// Render Markdown metadata (frontmatter + headings) to a flat JSON object.
    pub fn md_meta(
        input: *const MD_CHAR,
        input_size: MD_SIZE,
        process_output: unsafe extern "C" fn(*const MD_CHAR, MD_SIZE, *mut c_void),
        userdata: *mut c_void,
        parser_flags: c_uint,
        renderer_flags: c_uint,
    ) -> c_int;

    /// Heal (complete) incomplete streaming Markdown text.
    pub fn md_heal(
        input: *const c_char,
        input_size: c_uint,
        process_output: unsafe extern "C" fn(*const c_char, c_uint, *mut c_void),
        userdata: *mut c_void,
    ) -> c_int;
}
