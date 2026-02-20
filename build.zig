const std = @import("std");
const zon = @import("build.zig.zon");

const version = std.SemanticVersion.parse(zon.version) catch unreachable;

// --- Source files ---

const parser_source = "src/md4x.c";
const renderer_sources = [_][]const u8{ "src/renderers/md4x-html.c", "src/renderers/md4x-ast.c", "src/renderers/md4x-ansi.c", "src/entity.c" };
const cli_sources = renderer_sources ++ .{ "src/cli/md4x-cli.c", "src/cli/cmdline.c" };
const wasm_sources = renderer_sources ++ .{"src/md4x-wasm.c"};
const napi_sources = renderer_sources ++ .{"src/md4x-napi.c"};

// --- Compiler flags ---

const c_flags: []const []const u8 = &.{
    std.fmt.comptimePrint("-DMD_VERSION_MAJOR={d}", .{version.major}),
    std.fmt.comptimePrint("-DMD_VERSION_MINOR={d}", .{version.minor}),
    std.fmt.comptimePrint("-DMD_VERSION_RELEASE={d}", .{version.patch}),
    "-Wall",
    "-Wextra",
    "-Wshadow",
    "-Wdeclaration-after-statement",
    "-O2",
};
const c_flags_utf8 = c_flags ++ &[_][]const u8{"-DMD4X_USE_UTF8"};
const napi_c_flags = c_flags ++ &[_][]const u8{"-DNODE_GYP_MODULE_NAME=md4x"};

const libyaml_c_flags: []const []const u8 = &.{
    "-DYAML_DECLARE_STATIC",
    "-DYAML_VERSION_MAJOR=0",
    "-DYAML_VERSION_MINOR=2",
    "-DYAML_VERSION_PATCH=5",
    "-DYAML_VERSION_STRING=\"0.2.5\"",
};

// --- Build options passed to WASM/NAPI targets ---

const PkgBuildOptions = struct {
    optimize: std.builtin.OptimizeMode,
    strip: bool,
    libyaml_src: std.Build.Module.AddCSourceFilesOptions,
    include_paths: []const std.Build.LazyPath,
    clean_step: *std.Build.Step,
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.option(std.builtin.OptimizeMode, "optimize", "Prioritize performance, safety, or binary size") orelse .ReleaseFast;

    const strip = optimize != .Debug;

    const mod_opts: std.Build.Module.CreateOptions = .{
        .target = target,
        .optimize = optimize,
        .strip = strip,
        .link_libc = true,
    };

    // --- libyaml (YAML parser for frontmatter) ---

    const libyaml = b.dependency("libyaml", .{});
    const libyaml_src: std.Build.Module.AddCSourceFilesOptions = .{
        .root = libyaml.path(""),
        .files = &.{ "src/api.c", "src/reader.c", "src/scanner.c", "src/parser.c" },
        .flags = libyaml_c_flags,
    };

    const include_paths: []const std.Build.LazyPath = &.{ b.path("src"), b.path("src/renderers"), libyaml.path("include") };

    // --- CLI executable ---

    const exe = b.addExecutable(.{
        .name = "md4x",
        .root_module = b.createModule(mod_opts),
    });
    exe.addCSourceFile(.{ .file = b.path(parser_source), .flags = c_flags_utf8 });
    exe.addCSourceFiles(.{ .files = &cli_sources, .flags = c_flags });
    exe.addCSourceFiles(libyaml_src);
    for (include_paths) |p| exe.addIncludePath(p);
    b.installArtifact(exe);

    // --- Clean packages/md4x/build before rebuilding ---

    const clean_build = b.addRemoveDirTree(b.path("packages/md4x/build"));

    // --- WASM & NAPI targets ---

    const pkg_opts: PkgBuildOptions = .{
        .optimize = optimize,
        .strip = strip,
        .libyaml_src = libyaml_src,
        .include_paths = include_paths,
        .clean_step = &clean_build.step,
    };
    const wasm_step = addWasm(b, pkg_opts);
    const napi_all_step = addNapi(b, pkg_opts);

    b.getInstallStep().dependOn(wasm_step);
    b.getInstallStep().dependOn(napi_all_step);
}

fn addWasm(b: *std.Build, opts: PkgBuildOptions) *std.Build.Step {
    const wasm_target = b.resolveTargetQuery(.{
        .cpu_arch = .wasm32,
        .os_tag = .wasi,
    });

    const md4x_wasm = b.addExecutable(.{
        .name = "md4x",
        .root_module = b.createModule(.{
            .target = wasm_target,
            .optimize = .ReleaseSmall,
            .link_libc = true,
            .strip = true,
        }),
    });
    md4x_wasm.rdynamic = true;
    md4x_wasm.addCSourceFile(.{ .file = b.path(parser_source), .flags = c_flags_utf8 });
    md4x_wasm.addCSourceFiles(.{ .files = &wasm_sources, .flags = c_flags });
    md4x_wasm.addCSourceFiles(opts.libyaml_src);
    for (opts.include_paths) |p| md4x_wasm.addIncludePath(p);
    md4x_wasm.root_module.export_symbol_names = &.{
        "md4x_alloc",
        "md4x_free",
        "md4x_to_html",
        "md4x_to_ast",
        "md4x_to_ansi",
        "md4x_result_ptr",
        "md4x_result_size",
    };

    const wasm_install = b.addInstallArtifact(md4x_wasm, .{
        .dest_dir = .{ .override = .{ .custom = "../packages/md4x/build" } },
    });
    wasm_install.step.dependOn(opts.clean_step);
    const wasm_step = b.step("wasm", "Build WASM library");
    wasm_step.dependOn(&wasm_install.step);
    return wasm_step;
}

fn addNapi(b: *std.Build, opts: PkgBuildOptions) *std.Build.Step {
    const napi_include = b.option([]const u8, "napi-include", "Path to node-api-headers include directory") orelse "node_modules/node-api-headers/include";
    const napi_def = b.option([]const u8, "napi-def", "Path to node_api.def file (for Windows targets)") orelse "node_modules/node-api-headers/def/node_api.def";

    // Ensure node_modules are installed (provides node-api-headers)
    const bun_install = b.addSystemCommand(&.{ "bun", "install", "--frozen-lockfile" });

    const NapiTarget = struct {
        name: []const u8,
        cpu_arch: std.Target.Cpu.Arch,
        os_tag: std.Target.Os.Tag,
        abi: std.Target.Abi,
        output_name: []const u8,
        dlltool_machine: ?[]const u8,
    };

    const napi_targets = [_]NapiTarget{
        .{ .name = "linux-x64", .cpu_arch = .x86_64, .os_tag = .linux, .abi = .gnu, .output_name = "md4x.linux-x64.node", .dlltool_machine = null },
        .{ .name = "linux-arm64", .cpu_arch = .aarch64, .os_tag = .linux, .abi = .gnu, .output_name = "md4x.linux-arm64.node", .dlltool_machine = null },
        .{ .name = "darwin-x64", .cpu_arch = .x86_64, .os_tag = .macos, .abi = .none, .output_name = "md4x.darwin-x64.node", .dlltool_machine = null },
        .{ .name = "darwin-arm64", .cpu_arch = .aarch64, .os_tag = .macos, .abi = .none, .output_name = "md4x.darwin-arm64.node", .dlltool_machine = null },
        .{ .name = "win32-x64", .cpu_arch = .x86_64, .os_tag = .windows, .abi = .gnu, .output_name = "md4x.win32-x64.node", .dlltool_machine = "i386:x86-64" },
        .{ .name = "win32-arm64", .cpu_arch = .aarch64, .os_tag = .windows, .abi = .gnu, .output_name = "md4x.win32-arm64.node", .dlltool_machine = "arm64" },
    };

    const napi_all_step = b.step("napi-all", "Build NAPI addon for all platforms");

    inline for (napi_targets) |nt| {
        const cross_target = b.resolveTargetQuery(.{
            .cpu_arch = nt.cpu_arch,
            .os_tag = nt.os_tag,
            .abi = nt.abi,
        });

        const napi_lib = b.addLibrary(.{
            .linkage = .dynamic,
            .name = "md4x",
            .root_module = b.createModule(.{
                .target = cross_target,
                .optimize = opts.optimize,
                .link_libc = true,
                .strip = opts.strip,
            }),
        });
        napi_lib.addCSourceFile(.{ .file = b.path(parser_source), .flags = c_flags_utf8 });
        napi_lib.addCSourceFiles(.{ .files = &napi_sources, .flags = napi_c_flags });
        napi_lib.addCSourceFiles(opts.libyaml_src);
        for (opts.include_paths) |p| napi_lib.addIncludePath(p);
        napi_lib.addIncludePath(.{ .cwd_relative = napi_include });

        if (nt.dlltool_machine) |machine| {
            const dlltool = b.addSystemCommand(&.{
                "zig", "dlltool",
                "-d",  napi_def,
                "-m",  machine,
                "-D",  "node.exe",
                "-l",
            });
            const implib = dlltool.addOutputFileArg("node_api.lib");
            napi_lib.addObjectFile(implib);
        } else {
            napi_lib.linker_allow_shlib_undefined = true;
        }

        const cross_install = b.addInstallArtifact(napi_lib, .{
            .dest_dir = .{ .override = .{ .custom = "../packages/md4x/build" } },
            .dest_sub_path = nt.output_name,
        });
        cross_install.step.dependOn(opts.clean_step);
        cross_install.step.dependOn(&bun_install.step);

        const cross_step = b.step("napi-" ++ nt.name, "Build NAPI addon for " ++ nt.name);
        cross_step.dependOn(&cross_install.step);
        napi_all_step.dependOn(&cross_install.step);
    }

    return napi_all_step;
}
