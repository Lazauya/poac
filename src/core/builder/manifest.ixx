module;

// std
#include <fstream>
#include <string>

// external
#include <boost/algorithm/string.hpp> // boost::algorithm::join
#include <ninja/build.h> // Builder, Status // NOLINT(build/include_order)
#include <ninja/graph.h> // Node // NOLINT(build/include_order)
#include <toml.hpp>

// internal
#include "../../util/result-macros.hpp"

export module poac.core.builder.manifest;

import poac.config;
import poac.core.builder.compiler.cxx;
import poac.core.builder.data;
import poac.core.builder.syntax;
import poac.core.resolver.types; // ResolvedDeps
import poac.core.resolver; // get_extracted_path
import poac.data.manifest;
import poac.util.cfg;
import poac.util.format;
import poac.util.log;
import poac.util.result;
import poac.util.rustify;
import poac.util.registry.conan.v1.manifest;

namespace poac::core::builder::manifest {

export inline constexpr StringRef MANIFEST_FILE_NAME = "build.ninja";
inline constexpr Arr<StringRef, 2> MANIFEST_HEADERS{
    "This file is automatically generated by Poac.",
    "It is not intended for manual editing."};

inline auto ninja_manifest_last_modified(const Path& build_dir)
    -> fs::file_time_type {
  return fs::last_write_time(build_dir / MANIFEST_FILE_NAME);
}

inline auto is_outdated(const Path& build_dir) -> bool {
  if (!fs::exists(build_dir / MANIFEST_FILE_NAME)) {
    return true;
  }
  using poac::data::manifest::poac_toml_last_modified;
  return ninja_manifest_last_modified(build_dir)
         < poac_toml_last_modified(config::cwd);
}

export auto rebuild(data::NinjaMain& ninja_main, Status& status, String& err)
    -> bool {
  Node* node = ninja_main.state.LookupNode(
      (ninja_main.build_dir / MANIFEST_FILE_NAME).string()
  );
  if (!node) {
    return false;
  }

  Builder builder(
      &ninja_main.state, ninja_main.config, &ninja_main.build_log,
      &ninja_main.deps_log, &ninja_main.disk_interface, &status,
      ninja_main.start_time_millis
  );
  if (!builder.AddTarget(node, &err)) {
    return false;
  }
  if (builder.AlreadyUpToDate()) {
    return false; // Not an error, but we didn't rebuild.
  }
  if (!builder.Build(&err)) {
    return false;
  }

  // The manifest was only rebuilt if it is now dirty (it may have been cleaned
  // by a restat).
  if (!node->dirty()) {
    // Reset the state to prevent problems like
    // https://github.com/ninja-build/ninja/issues/874
    ninja_main.state.Reset();
    return false;
  }
  return true;
}

auto construct_includes(Vec<Path>& includes) -> Vec<String> {
  Vec<String> include_flags;
  for (const Path& inc : includes) {
    include_flags.emplace_back(format("-I{}", inc.string()));
  }
  return include_flags;
}

auto gather_includes(const resolver::ResolvedDeps& resolved_deps)
    -> Vec<String> {
  Vec<Path> includes;
  if (fs::exists(config::include_dir)) {
    includes.emplace_back(config::include_dir);
  }
  for (const auto& [package, inner_deps] : resolved_deps) {
    static_cast<void>(inner_deps);

    const Path include_path = resolver::get_extracted_path(package) / "include";
    if (fs::exists(include_path) && fs::is_directory(include_path)) {
      includes.emplace_back(include_path);
    }
  }
  return construct_includes(includes);
}

auto get_cfg_profile(const toml::value& poac_manifest) -> Vec<toml::table> {
  const auto target =
      toml::find_or<toml::table>(poac_manifest, "target", toml::table{});
  Vec<toml::table> profiles;
  for (const auto& [key, val] : target) {
    if (key.find("cfg(") != None) {
      if (util::cfg::parse(key).match()) {
        const auto profile =
            toml::find_or<toml::table>(val, "profile", toml::table{});
        profiles.emplace_back(profile);
      }
    }
  }
  return profiles;
}

auto gather_flags(
    const toml::value& poac_manifest, const String& name,
    const Option<String>& prefix = None
) -> Vec<String> {
  auto f = toml::find_or<Vec<String>>(
      poac_manifest, "target", "profile", name, Vec<String>{}
  );
  if (prefix.has_value()) {
    std::transform(
        f.begin(), f.end(), f.begin(),
        [p = prefix.value()](const auto& s) { return p + s; }
    );
  }
  return f;
}

[[nodiscard]] auto construct(
    const Path& build_dir, const toml::value& poac_manifest,
    const resolver::ResolvedDeps& resolved_deps
) -> Result<String> {
  syntax::Writer writer{std::ostringstream()};
  for (const StringRef header : MANIFEST_HEADERS) {
    writer.comment(String(header));
  }
  writer.newline();

  const String name = toml::find<String>(poac_manifest, "package", "name");
  const String version =
      toml::find<String>(poac_manifest, "package", "version");
  const i64 edition = toml::find<i64>(poac_manifest, "package", "edition");
  const String command = Try(compiler::cxx::get_command(edition, false));

  writer.rule(
      "compile",
      format(
          "{} $in -o $out $OPTIONS $DEFINES $INCLUDES $LIBDIRS $LIBRARIES",
          command
      ),
      syntax::RuleSet{
          .description = "$PACKAGE_NAME v$PACKAGE_VERSION $PACKAGE_PATH",
      }
  );
  writer.newline();

  const Path source_file = "src"_path / "main.cpp";
  Path output_file;
  if (source_file == "src"_path / "main.cpp") {
    // When building src/main.cpp, the output executable should be stored at
    // poac-out/debug/name
    output_file = build_dir / name;
  } else {
    output_file = (build_dir / source_file).string() + ".o";
    fs::create_directories(output_file.parent_path());
  }

  const Vec<String> options = gather_flags(poac_manifest, "options");
  Vec<String> includes = gather_includes(resolved_deps);
  Vec<String> defines = gather_flags(poac_manifest, "definitions", "-D");
  Vec<String> libraries = gather_flags(poac_manifest, "libraries", "-l");
  Vec<String> libdirs;

  auto conan_manifest =
      Try(poac::util::registry::conan::v1::manifest::gather_conan_deps());
  append(includes, conan_manifest.includes);
  append(defines, conan_manifest.defines);
  append(libraries, conan_manifest.libraries);
  append(libdirs, conan_manifest.libdirs);

  writer.build(
      {output_file.string()}, "compile",
      syntax::BuildSet{
          .inputs = std::vector{source_file.string()},
          .variables =
              syntax::Variables{
                  {"PACKAGE_NAME", name},
                  {"PACKAGE_VERSION", version},
                  {"PACKAGE_PATH", format("({})", config::cwd)},
                  {"OPTIONS", boost::algorithm::join(options, " ")},
                  {"DEFINES", boost::algorithm::join(defines, " ")},
                  {"INCLUDES", boost::algorithm::join(includes, " ")},
                  {"LIBDIRS", boost::algorithm::join(libdirs, " ")},
                  {"LIBRARIES", boost::algorithm::join(libraries, " ")},
              },
      }
  );
  writer.newline();

  writer.defalt({output_file.string()});
  return Ok(writer.get_value());
}

export [[nodiscard]] auto create(
    const Path& build_dir, const toml::value& poac_manifest,
    const resolver::ResolvedDeps& resolved_deps
) -> Result<void> {
  // TODO(ken-matsui): `build.ninja` will be constructed from `poac.toml`,
  //   so if `poac.toml` has no change,
  //   then `build.ninja` is not needed to be updated.
  //        if (is_outdated(build_dir)) {
  std::ofstream ofs(build_dir / MANIFEST_FILE_NAME, std::ios::out);
  ofs << Try(construct(build_dir, poac_manifest, resolved_deps));
  //        }
  return Ok();
}

} // namespace poac::core::builder::manifest
