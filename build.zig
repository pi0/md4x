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
    md4x_html.addCSourceFiles(.{ .files = &.{"src/md4x-html.c"}, .flags = c_flags });
    md4x_html.addCSourceFile(entity_src);
    md4x_html.addIncludePath(b.path("src"));
    md4x_html.linkLibrary(md4x);
    md4x_html.installHeader(b.path("src/md4x-html.h"), "md4x-html.h");

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
    md4x_json.addCSourceFiles(.{ .files = &.{"src/md4x-json.c"}, .flags = c_flags });
    md4x_json.addIncludePath(b.path("src"));
    md4x_json.linkLibrary(md4x);
    md4x_json.installHeader(b.path("src/md4x-json.h"), "md4x-json.h");

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
    md4x_ansi.addCSourceFiles(.{ .files = &.{"src/md4x-ansi.c"}, .flags = c_flags });
    md4x_ansi.addCSourceFile(entity_src);
    md4x_ansi.addIncludePath(b.path("src"));
    md4x_ansi.linkLibrary(md4x);
    md4x_ansi.installHeader(b.path("src/md4x-ansi.h"), "md4x-ansi.h");

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
    exe.addCSourceFiles(.{ .files = &.{ "cli/md4x-cli.c", "cli/cmdline.c" }, .flags = c_flags });
    exe.addIncludePath(b.path("src"));
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
}
