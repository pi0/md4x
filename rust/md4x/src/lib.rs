//! Safe Rust bindings to [md4x](https://github.com/unjs/md4x), a fast
//! CommonMark 0.31.2 markdown parser written in C.
//!
//! # Quick start
//!
//! ```rust
//! use md4x::{render_html, render_text, heal, ParserFlags, HtmlFlags};
//!
//! let html = render_html("# Hello **world**", ParserFlags::DIALECT_ALL, HtmlFlags::empty()).unwrap();
//! assert!(html.contains("<h1>"));
//!
//! let plain = render_text("# Hello **world**", ParserFlags::DIALECT_ALL, Default::default()).unwrap();
//! assert!(plain.contains("Hello world"));
//!
//! let fixed = heal("**unclosed bold").unwrap();
//! assert_eq!(fixed, "**unclosed bold**");
//! ```
//!
//! # System requirements
//!
//! No external C dependencies are required. `md4x-sys` bundles and compiles
//! both md4x C sources and libyaml 0.2.5 via the `cc` crate.

use std::os::raw::{c_char, c_void};
use std::ptr;

pub use md4x_sys as sys;

// ---------------------------------------------------------------------------
// Error type
// ---------------------------------------------------------------------------

/// Error returned when rendering fails.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error;

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "md4x rendering failed")
    }
}

impl std::error::Error for Error {}

// ---------------------------------------------------------------------------
// Flags
// ---------------------------------------------------------------------------

bitflags::bitflags! {
    /// Parser feature flags. Combine to enable extensions.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
    pub struct ParserFlags: u32 {
        /// Collapse non-trivial whitespace to a single space in normal text.
        const COLLAPSE_WHITESPACE      = sys::MD_FLAG_COLLAPSEWHITESPACE;
        /// Allow ATX headers without a space (`###header`).
        const PERMISSIVE_ATX_HEADERS   = sys::MD_FLAG_PERMISSIVEATXHEADERS;
        /// Recognize bare URLs as autolinks.
        const PERMISSIVE_URL_AUTOLINKS = sys::MD_FLAG_PERMISSIVEURLAUTOLINKS;
        /// Recognize bare email addresses as autolinks.
        const PERMISSIVE_EMAIL_AUTOLINKS = sys::MD_FLAG_PERMISSIVEEMAILAUTOLINKS;
        /// Disable indented code blocks (fenced only).
        const NO_INDENTED_CODE_BLOCKS  = sys::MD_FLAG_NOINDENTEDCODEBLOCKS;
        /// Disable raw HTML blocks.
        const NO_HTML_BLOCKS           = sys::MD_FLAG_NOHTMLBLOCKS;
        /// Disable inline raw HTML.
        const NO_HTML_SPANS            = sys::MD_FLAG_NOHTMLSPANS;
        /// Enable GFM-style tables.
        const TABLES                   = sys::MD_FLAG_TABLES;
        /// Enable `~~strikethrough~~`.
        const STRIKETHROUGH            = sys::MD_FLAG_STRIKETHROUGH;
        /// Enable `www.` autolinks.
        const PERMISSIVE_WWW_AUTOLINKS = sys::MD_FLAG_PERMISSIVEWWWAUTOLINKS;
        /// Enable `- [x] task` lists.
        const TASKLISTS                = sys::MD_FLAG_TASKLISTS;
        /// Enable `$...$` and `$$...$$` LaTeX math.
        const LATEX_MATH_SPANS         = sys::MD_FLAG_LATEXMATHSPANS;
        /// Enable `[[wiki links]]`.
        const WIKI_LINKS               = sys::MD_FLAG_WIKILINKS;
        /// Enable `_underline_` (disables `_` for emphasis).
        const UNDERLINE                = sys::MD_FLAG_UNDERLINE;
        /// Treat all soft breaks as hard breaks.
        const HARD_SOFT_BREAKS         = sys::MD_FLAG_HARD_SOFT_BREAKS;
        /// Enable YAML `---` frontmatter.
        const FRONTMATTER              = sys::MD_FLAG_FRONTMATTER;
        /// Enable `:component` and `::block-component` syntax.
        const COMPONENTS               = sys::MD_FLAG_COMPONENTS;
        /// Enable `{.class #id key="value"}` trailing attributes.
        const ATTRIBUTES               = sys::MD_FLAG_ATTRIBUTES;
        /// Enable `> [!NOTE]` alert/admonition syntax.
        const ALERTS                   = sys::MD_FLAG_ALERTS;

        /// All permissive autolink variants combined.
        const PERMISSIVE_AUTOLINKS =
            Self::PERMISSIVE_EMAIL_AUTOLINKS.bits()
            | Self::PERMISSIVE_URL_AUTOLINKS.bits()
            | Self::PERMISSIVE_WWW_AUTOLINKS.bits();
        /// Disable both HTML blocks and inline HTML.
        const NO_HTML = Self::NO_HTML_BLOCKS.bits() | Self::NO_HTML_SPANS.bits();

        /// Strict CommonMark (no extensions).
        const DIALECT_COMMONMARK = 0;
        /// GitHub Flavored Markdown subset.
        const DIALECT_GITHUB =
            Self::PERMISSIVE_AUTOLINKS.bits()
            | Self::TABLES.bits()
            | Self::STRIKETHROUGH.bits()
            | Self::TASKLISTS.bits()
            | Self::ALERTS.bits();
        /// All additive extensions enabled.
        const DIALECT_ALL =
            Self::PERMISSIVE_AUTOLINKS.bits()
            | Self::TABLES.bits()
            | Self::STRIKETHROUGH.bits()
            | Self::TASKLISTS.bits()
            | Self::LATEX_MATH_SPANS.bits()
            | Self::WIKI_LINKS.bits()
            | Self::UNDERLINE.bits()
            | Self::FRONTMATTER.bits()
            | Self::COMPONENTS.bits()
            | Self::ATTRIBUTES.bits()
            | Self::ALERTS.bits();
    }
}

bitflags::bitflags! {
    /// HTML renderer flags.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
    pub struct HtmlFlags: u32 {
        /// Send debug output to stderr.
        const DEBUG            = sys::MD_HTML_FLAG_DEBUG;
        /// Do not translate HTML entities.
        const VERBATIM_ENTITIES = sys::MD_HTML_FLAG_VERBATIM_ENTITIES;
        /// Skip a UTF-8 BOM at the start of input.
        const SKIP_UTF8_BOM    = sys::MD_HTML_FLAG_SKIP_UTF8_BOM;
        /// Generate a complete HTML document (requires [`render_html_full`]).
        const FULL_HTML        = sys::MD_HTML_FLAG_FULL_HTML;
        /// Heal incomplete streaming markdown before rendering.
        const HEAL             = sys::MD_HTML_FLAG_HEAL;
    }
}

bitflags::bitflags! {
    /// AST renderer flags.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
    pub struct AstFlags: u32 {
        const DEBUG         = sys::MD_AST_FLAG_DEBUG;
        const SKIP_UTF8_BOM = sys::MD_AST_FLAG_SKIP_UTF8_BOM;
        const HEAL          = sys::MD_AST_FLAG_HEAL;
    }
}

bitflags::bitflags! {
    /// ANSI terminal renderer flags.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
    pub struct AnsiFlags: u32 {
        const DEBUG         = sys::MD_ANSI_FLAG_DEBUG;
        const SKIP_UTF8_BOM = sys::MD_ANSI_FLAG_SKIP_UTF8_BOM;
        /// Suppress ANSI escape codes (plain text output).
        const NO_COLOR      = sys::MD_ANSI_FLAG_NO_COLOR;
        const HEAL          = sys::MD_ANSI_FLAG_HEAL;
    }
}

bitflags::bitflags! {
    /// Plain-text renderer flags.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
    pub struct TextFlags: u32 {
        const DEBUG         = sys::MD_TEXT_FLAG_DEBUG;
        const SKIP_UTF8_BOM = sys::MD_TEXT_FLAG_SKIP_UTF8_BOM;
        const HEAL          = sys::MD_TEXT_FLAG_HEAL;
    }
}

bitflags::bitflags! {
    /// Meta renderer flags.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
    pub struct MetaFlags: u32 {
        const DEBUG         = sys::MD_META_FLAG_DEBUG;
        const SKIP_UTF8_BOM = sys::MD_META_FLAG_SKIP_UTF8_BOM;
        const HEAL          = sys::MD_META_FLAG_HEAL;
    }
}

// ---------------------------------------------------------------------------
// Full-HTML document options
// ---------------------------------------------------------------------------

/// Options for [`render_html_full`].
#[derive(Debug, Clone, Default)]
pub struct HtmlOpts<'a> {
    /// Override the document title (otherwise uses frontmatter `title`).
    pub title: Option<&'a str>,
    /// URL of a CSS stylesheet to include in `<head>`.
    pub css_url: Option<&'a str>,
}

// ---------------------------------------------------------------------------
// Internal output-collection helper
// ---------------------------------------------------------------------------

unsafe extern "C" fn collect_output(data: *const c_char, size: u32, userdata: *mut c_void) {
    let buf = &mut *(userdata as *mut Vec<u8>);
    let slice = std::slice::from_raw_parts(data as *const u8, size as usize);
    buf.extend_from_slice(slice);
}

fn buf_to_string(buf: Vec<u8>) -> String {
    // The C renderers emit valid UTF-8; use lossy conversion as a safety net.
    String::from_utf8(buf).unwrap_or_else(|e| String::from_utf8_lossy(e.as_bytes()).into_owned())
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/// Render Markdown to HTML body content.
///
/// All extensions are disabled by default — pass [`ParserFlags::DIALECT_ALL`]
/// to enable everything, or compose individual flags.
///
/// # Errors
/// Returns [`Error`] if the underlying C renderer fails.
pub fn render_html(
    input: &str,
    parser_flags: ParserFlags,
    renderer_flags: HtmlFlags,
) -> Result<String, Error> {
    let mut buf: Vec<u8> = Vec::new();
    let rc = unsafe {
        sys::md_html(
            input.as_ptr() as *const sys::MD_CHAR,
            input.len() as u32,
            collect_output,
            &mut buf as *mut Vec<u8> as *mut c_void,
            parser_flags.bits(),
            renderer_flags.bits(),
        )
    };
    if rc == 0 {
        Ok(buf_to_string(buf))
    } else {
        Err(Error)
    }
}

/// Render Markdown to a complete HTML document.
///
/// Requires [`HtmlFlags::FULL_HTML`] to be set in `renderer_flags`.
/// Frontmatter `title` and `description` fields are reflected in `<head>`;
/// `opts.title` overrides the frontmatter title.
///
/// # Errors
/// Returns [`Error`] if the underlying C renderer fails.
pub fn render_html_full(
    input: &str,
    parser_flags: ParserFlags,
    renderer_flags: HtmlFlags,
    opts: &HtmlOpts<'_>,
) -> Result<String, Error> {
    // We need null-terminated strings for the C API.
    let title_cstr;
    let css_cstr;

    let c_title = match opts.title {
        Some(t) => {
            title_cstr = std::ffi::CString::new(t).unwrap_or_default();
            title_cstr.as_ptr()
        }
        None => ptr::null(),
    };
    let c_css = match opts.css_url {
        Some(u) => {
            css_cstr = std::ffi::CString::new(u).unwrap_or_default();
            css_cstr.as_ptr()
        }
        None => ptr::null(),
    };

    let c_opts = sys::MD_HTML_OPTS {
        title: c_title,
        css_url: c_css,
    };

    let mut buf: Vec<u8> = Vec::new();
    let rc = unsafe {
        sys::md_html_ex(
            input.as_ptr() as *const sys::MD_CHAR,
            input.len() as u32,
            collect_output,
            &mut buf as *mut Vec<u8> as *mut c_void,
            parser_flags.bits(),
            renderer_flags.bits(),
            &c_opts,
        )
    };
    if rc == 0 {
        Ok(buf_to_string(buf))
    } else {
        Err(Error)
    }
}

/// Render Markdown to a [Comark AST](https://github.com/unjs/md4x#ast) JSON string.
///
/// The output is `{"nodes":[...],"frontmatter":{...},"meta":{}}` where each
/// node is either a plain string or an element tuple `["tag", {props}, ...children]`.
///
/// # Errors
/// Returns [`Error`] if the underlying C renderer fails.
pub fn render_ast(
    input: &str,
    parser_flags: ParserFlags,
    renderer_flags: AstFlags,
) -> Result<String, Error> {
    let mut buf: Vec<u8> = Vec::new();
    let rc = unsafe {
        sys::md_ast(
            input.as_ptr() as *const sys::MD_CHAR,
            input.len() as u32,
            collect_output,
            &mut buf as *mut Vec<u8> as *mut c_void,
            parser_flags.bits(),
            renderer_flags.bits(),
        )
    };
    if rc == 0 {
        Ok(buf_to_string(buf))
    } else {
        Err(Error)
    }
}

/// Render Markdown to ANSI terminal output with escape codes for styling.
///
/// # Errors
/// Returns [`Error`] if the underlying C renderer fails.
pub fn render_ansi(
    input: &str,
    parser_flags: ParserFlags,
    renderer_flags: AnsiFlags,
) -> Result<String, Error> {
    let mut buf: Vec<u8> = Vec::new();
    let rc = unsafe {
        sys::md_ansi(
            input.as_ptr() as *const sys::MD_CHAR,
            input.len() as u32,
            collect_output,
            &mut buf as *mut Vec<u8> as *mut c_void,
            parser_flags.bits(),
            renderer_flags.bits(),
        )
    };
    if rc == 0 {
        Ok(buf_to_string(buf))
    } else {
        Err(Error)
    }
}

/// Render Markdown to plain text, stripping all formatting.
///
/// # Errors
/// Returns [`Error`] if the underlying C renderer fails.
pub fn render_text(
    input: &str,
    parser_flags: ParserFlags,
    renderer_flags: TextFlags,
) -> Result<String, Error> {
    let mut buf: Vec<u8> = Vec::new();
    let rc = unsafe {
        sys::md_text(
            input.as_ptr() as *const sys::MD_CHAR,
            input.len() as u32,
            collect_output,
            &mut buf as *mut Vec<u8> as *mut c_void,
            parser_flags.bits(),
            renderer_flags.bits(),
        )
    };
    if rc == 0 {
        Ok(buf_to_string(buf))
    } else {
        Err(Error)
    }
}

/// Render Markdown metadata (frontmatter + headings) to a flat JSON object.
///
/// Output: `{"key":"value",...,"headings":[{"level":N,"text":"..."},...]}`
///
/// # Errors
/// Returns [`Error`] if the underlying C renderer fails.
pub fn render_meta(
    input: &str,
    parser_flags: ParserFlags,
    renderer_flags: MetaFlags,
) -> Result<String, Error> {
    let mut buf: Vec<u8> = Vec::new();
    let rc = unsafe {
        sys::md_meta(
            input.as_ptr() as *const sys::MD_CHAR,
            input.len() as u32,
            collect_output,
            &mut buf as *mut Vec<u8> as *mut c_void,
            parser_flags.bits(),
            renderer_flags.bits(),
        )
    };
    if rc == 0 {
        Ok(buf_to_string(buf))
    } else {
        Err(Error)
    }
}

/// Heal (complete) incomplete streaming Markdown text.
///
/// Closes unclosed formatting markers, completes partial links, closes open
/// code fences, and other fixes — useful for rendering mid-stream LLM output.
///
/// # Errors
/// Returns [`Error`] if the underlying C function fails (memory allocation).
pub fn heal(input: &str) -> Result<String, Error> {
    let mut buf: Vec<u8> = Vec::new();

    unsafe extern "C" fn collect_heal(data: *const c_char, size: u32, userdata: *mut c_void) {
        let buf = &mut *(userdata as *mut Vec<u8>);
        let slice = std::slice::from_raw_parts(data as *const u8, size as usize);
        buf.extend_from_slice(slice);
    }

    let rc = unsafe {
        sys::md_heal(
            input.as_ptr() as *const c_char,
            input.len() as u32,
            collect_heal,
            &mut buf as *mut Vec<u8> as *mut c_void,
        )
    };
    if rc == 0 {
        Ok(buf_to_string(buf))
    } else {
        Err(Error)
    }
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn html_heading() {
        let out = render_html("# Hello", ParserFlags::DIALECT_ALL, HtmlFlags::empty()).unwrap();
        assert!(out.contains("<h1>Hello</h1>"), "got: {out}");
    }

    #[test]
    fn html_bold() {
        let out = render_html("**bold**", ParserFlags::DIALECT_ALL, HtmlFlags::empty()).unwrap();
        assert!(out.contains("<strong>bold</strong>"), "got: {out}");
    }

    #[test]
    fn text_strips_formatting() {
        let out = render_text(
            "# Title\n\n**bold** and _italic_",
            ParserFlags::DIALECT_ALL,
            TextFlags::empty(),
        )
        .unwrap();
        assert!(!out.contains('#'));
        assert!(!out.contains('*'));
        assert!(out.contains("Title"));
        assert!(out.contains("bold"));
    }

    #[test]
    fn ast_is_json() {
        let out = render_ast("# Hello", ParserFlags::DIALECT_ALL, AstFlags::empty()).unwrap();
        assert!(out.starts_with('{'), "expected JSON object, got: {out}");
        assert!(out.contains("nodes"), "got: {out}");
    }

    #[test]
    fn meta_headings() {
        let out = render_meta(
            "# Title\n\n## Section",
            ParserFlags::DIALECT_ALL,
            MetaFlags::empty(),
        )
        .unwrap();
        assert!(out.contains("headings"), "got: {out}");
        assert!(out.contains("Title"), "got: {out}");
    }

    #[test]
    fn heal_closes_bold() {
        let out = heal("**unclosed").unwrap();
        assert!(out.ends_with("**"), "got: {out}");
    }

    #[test]
    fn ansi_renders() {
        let out = render_ansi("# Hello", ParserFlags::DIALECT_ALL, AnsiFlags::empty()).unwrap();
        assert!(!out.is_empty());
        assert!(out.contains("Hello"));
    }

    #[test]
    fn html_full_document() {
        let out = render_html_full(
            "# Hello",
            ParserFlags::DIALECT_ALL,
            HtmlFlags::FULL_HTML,
            &HtmlOpts {
                title: Some("My Doc"),
                css_url: None,
            },
        )
        .unwrap();
        assert!(out.contains("<!DOCTYPE html>"), "got: {out}");
        assert!(out.contains("My Doc"), "got: {out}");
    }

    #[test]
    fn frontmatter_in_meta() {
        let md = "---\ntitle: My Page\n---\n\n# Hello";
        let out = render_meta(md, ParserFlags::DIALECT_ALL, MetaFlags::empty()).unwrap();
        assert!(out.contains("My Page"), "got: {out}");
    }
}
