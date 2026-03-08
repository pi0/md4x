use std::env;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    // Prefer vendored sources (used when publishing to crates.io).
    // Fall back to the monorepo layout (rust/md4x-sys -> rust -> repo root -> src/).
    let src = {
        let vendored = manifest_dir.join("vendor/md4x");
        if vendored.exists() {
            vendored
        } else {
            let root = manifest_dir.parent().unwrap().parent().unwrap();
            root.join("src")
        }
    };
    let renderers = src.join("renderers");
    let libyaml = manifest_dir.join("vendor/libyaml");

    // --- Compile bundled libyaml 0.2.5 ---
    cc::Build::new()
        .include(libyaml.join("include"))
        .include(libyaml.join("src"))
        .define("YAML_DECLARE_STATIC", None)
        .define("YAML_VERSION_MAJOR", "0")
        .define("YAML_VERSION_MINOR", "2")
        .define("YAML_VERSION_PATCH", "5")
        .define("YAML_VERSION_STRING", "\"0.2.5\"")
        .file(libyaml.join("src/api.c"))
        .file(libyaml.join("src/reader.c"))
        .file(libyaml.join("src/scanner.c"))
        .file(libyaml.join("src/parser.c"))
        .compile("yaml");

    // --- Compile md4x C sources ---
    cc::Build::new()
        .include(&src)
        .include(&renderers)
        .include(libyaml.join("include"))
        .define("MD4X_USE_UTF8", None)
        .define("MD_VERSION_MAJOR", "0")
        .define("MD_VERSION_MINOR", "0")
        .define("MD_VERSION_RELEASE", "18")
        .define("YAML_DECLARE_STATIC", None)
        .flag_if_supported("-Wall")
        .flag_if_supported("-Wextra")
        .flag_if_supported("-Wshadow")
        .flag_if_supported("-Wdeclaration-after-statement")
        .flag_if_supported("-O2")
        .file(src.join("md4x.c"))
        .file(src.join("entity.c"))
        .file(renderers.join("md4x-html.c"))
        .file(renderers.join("md4x-ast.c"))
        .file(renderers.join("md4x-ansi.c"))
        .file(renderers.join("md4x-text.c"))
        .file(renderers.join("md4x-meta.c"))
        .file(renderers.join("md4x-heal.c"))
        .compile("md4x");

    println!("cargo:rerun-if-changed={}", src.display());
    println!("cargo:rerun-if-changed={}", libyaml.display());
}
