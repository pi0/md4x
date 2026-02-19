const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.option(std.builtin.OptimizeMode, "optimize", "Prioritize performance, safety, or binary size") orelse .ReleaseFast;

    const version = std.SemanticVersion{ .major = 0, .minor = 5, .patch = 2 };

    const c_flags: []const []const u8 = &.{
        "-DMD_VERSION_MAJOR=0",
        "-DMD_VERSION_MINOR=5",
        "-DMD_VERSION_RELEASE=2",
        "-Wall",
        "-Wextra",
        "-Wshadow",
        "-Wdeclaration-after-statement",
        "-O2",
    };

    const strip = optimize != .Debug;

    // --- libyaml (YAML parser for JSON renderer frontmatter) ---

    const libyaml = b.dependency("libyaml", .{});
    const libyaml_c_flags: []const []const u8 = &.{
        "-DYAML_DECLARE_STATIC",
        "-DYAML_VERSION_MAJOR=0",
        "-DYAML_VERSION_MINOR=2",
        "-DYAML_VERSION_PATCH=5",
        "-DYAML_VERSION_STRING=\"0.2.5\"",
    };
    const libyaml_src: std.Build.Module.AddCSourceFilesOptions = .{
        .root = libyaml.path(""),
        .files = &.{ "src/api.c", "src/reader.c", "src/scanner.c", "src/parser.c" },
        .flags = libyaml_c_flags,
    };
    const libyaml_include = libyaml.path("include");

    // --- Libraries ---

    const md4x = b.addLibrary(.{
        .name = "md4x",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .strip = strip,
        }),
        .version = version,
    });
    md4x.addCSourceFile(.{ .file = b.path("src/md4x.c"), .flags = c_flags ++ &[_][]const u8{"-DMD4X_USE_UTF8"} });
    md4x.addIncludePath(b.path("src"));
    md4x.installHeader(b.path("src/md4x.h"), "md4x.h");

    const entity_src: std.Build.Module.CSourceFile = .{ .file = b.path("src/entity.c"), .flags = c_flags };

    const md4x_html = b.addLibrary(.{
        .name = "md4x-html",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .strip = strip,
        }),
        .version = version,
    });
    md4x_html.addCSourceFiles(.{ .files = &.{"src/renderers/md4x-html.c"}, .flags = c_flags });
    md4x_html.addCSourceFile(entity_src);
    md4x_html.addIncludePath(b.path("src"));
    md4x_html.addIncludePath(b.path("src/renderers"));
    md4x_html.linkLibrary(md4x);
    md4x_html.installHeader(b.path("src/renderers/md4x-html.h"), "md4x-html.h");

    const md4x_json = b.addLibrary(.{
        .name = "md4x-json",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .strip = strip,
        }),
        .version = version,
    });
    md4x_json.addCSourceFiles(.{ .files = &.{"src/renderers/md4x-json.c"}, .flags = c_flags });
    md4x_json.addCSourceFiles(libyaml_src);
    md4x_json.addIncludePath(b.path("src"));
    md4x_json.addIncludePath(b.path("src/renderers"));
    md4x_json.addIncludePath(libyaml_include);
    md4x_json.linkLibrary(md4x);
    md4x_json.installHeader(b.path("src/renderers/md4x-json.h"), "md4x-json.h");

    const md4x_ansi = b.addLibrary(.{
        .name = "md4x-ansi",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .strip = strip,
        }),
        .version = version,
    });
    md4x_ansi.addCSourceFiles(.{ .files = &.{"src/renderers/md4x-ansi.c"}, .flags = c_flags });
    md4x_ansi.addCSourceFile(entity_src);
    md4x_ansi.addIncludePath(b.path("src"));
    md4x_ansi.addIncludePath(b.path("src/renderers"));
    md4x_ansi.linkLibrary(md4x);
    md4x_ansi.installHeader(b.path("src/renderers/md4x-ansi.h"), "md4x-ansi.h");

    b.installArtifact(md4x);
    b.installArtifact(md4x_html);
    b.installArtifact(md4x_json);
    b.installArtifact(md4x_ansi);

    // --- CLI executable ---

    const exe = b.addExecutable(.{
        .name = "md4x",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .strip = strip,
        }),
    });
    exe.addCSourceFiles(.{ .files = &.{ "src/cli/md4x-cli.c", "src/cli/cmdline.c" }, .flags = c_flags });
    exe.addIncludePath(b.path("src"));
    exe.addIncludePath(b.path("src/renderers"));
    exe.linkLibrary(md4x_html);
    exe.linkLibrary(md4x_json);
    exe.linkLibrary(md4x_ansi);
    b.installArtifact(exe);

    // --- Run step ---

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
    const run_step = b.step("run", "Run md4x CLI");
    run_step.dependOn(&run_cmd.step);

    // --- WASM library ---

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
    md4x_wasm.addCSourceFile(.{ .file = b.path("src/md4x.c"), .flags = c_flags ++ &[_][]const u8{"-DMD4X_USE_UTF8"} });
    md4x_wasm.addCSourceFiles(.{
        .files = &.{ "src/renderers/md4x-html.c", "src/renderers/md4x-json.c", "src/renderers/md4x-ansi.c", "src/entity.c", "src/md4x-wasm.c" },
        .flags = c_flags,
    });
    md4x_wasm.addCSourceFiles(libyaml_src);
    md4x_wasm.addIncludePath(b.path("src"));
    md4x_wasm.addIncludePath(b.path("src/renderers"));
    md4x_wasm.addIncludePath(libyaml_include);
    md4x_wasm.root_module.export_symbol_names = &.{
        "md4x_alloc",
        "md4x_free",
        "md4x_to_html",
        "md4x_to_json",
        "md4x_to_ansi",
        "md4x_result_ptr",
        "md4x_result_size",
    };

    const wasm_install = b.addInstallArtifact(md4x_wasm, .{
        .dest_dir = .{ .override = .{ .custom = "../packages/md4x/build" } },
    });
    const wasm_log = b.addSystemCommand(&.{ "echo", "Built WASM -> packages/md4x/build/md4x.wasm" });
    wasm_log.step.dependOn(&wasm_install.step);
    const wasm_step = b.step("wasm", "Build WASM library");
    wasm_step.dependOn(&wasm_log.step);

    // --- NAPI shared libraries ---

    const napi_include = b.option([]const u8, "napi-include", "Path to node-api-headers include directory") orelse "node_modules/node-api-headers/include";
    const napi_def = b.option([]const u8, "napi-def", "Path to node_api.def file (for Windows targets)") orelse "node_modules/node-api-headers/def/node_api.def";

    const napi_c_flags = c_flags ++ &[_][]const u8{"-DNODE_GYP_MODULE_NAME=md4x"};
    const napi_parser_flags = c_flags ++ &[_][]const u8{"-DMD4X_USE_UTF8"};

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

    // Cross-platform NAPI targets
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
                .optimize = optimize,
                .link_libc = true,
                .strip = strip,
            }),
        });
        napi_lib.addCSourceFile(.{ .file = b.path("src/md4x.c"), .flags = napi_parser_flags });
        napi_lib.addCSourceFiles(.{
            .files = &.{ "src/renderers/md4x-html.c", "src/renderers/md4x-json.c", "src/renderers/md4x-ansi.c", "src/entity.c", "src/md4x-napi.c" },
            .flags = napi_c_flags,
        });
        napi_lib.addCSourceFiles(libyaml_src);
        napi_lib.addIncludePath(b.path("src"));
        napi_lib.addIncludePath(b.path("src/renderers"));
        napi_lib.addIncludePath(libyaml_include);
        napi_lib.addIncludePath(.{ .cwd_relative = napi_include });

        if (nt.dlltool_machine) |machine| {
            // Windows: generate import library from .def file using zig dlltool
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

        const cross_step = b.step("napi-" ++ nt.name, "Build NAPI addon for " ++ nt.name);
        cross_step.dependOn(&cross_install.step);
        napi_all_step.dependOn(&cross_install.step);
    }

    b.getInstallStep().dependOn(wasm_step);
    b.getInstallStep().dependOn(napi_all_step);
}
