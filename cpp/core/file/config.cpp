/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include <optional>
#include <vector>

#include "app.h"
#include "debug.h"
#include "env.h"
#include "exception.h"
#include "header.h"
#include "mrtrix.h"
#include "types.h"

#include "file/config.h"
#include "file/path.h"

namespace MR::File {

KeyValues Config::config;

const std::string Config::file_basename("mrtrix.conf");
const std::string Config::default_sys_config_file("/etc/" + file_basename);

// ENVVAR name: MRTRIX_CONFIGFILE
// ENVVAR This can be used to set the location of the system-wide
// ENVVAR configuration file. By default, this is ``/etc/mrtrix.conf``.
// ENVVAR This can be useful for deployments where access to the system's
// ENVVAR ``/etc`` folder is problematic, or to allow different versions of
// ENVVAR the software to have different configurations, etc.

void Config::init() {
  const std::string sysconf_location = MR::get_env("MRTRIX_CONFIGFILE", default_sys_config_file);

  std::filesystem::path sysconf_path(sysconf_location);
  if (std::filesystem::is_regular_file(sysconf_path)) {
    INFO("reading config file \"" + sysconf_path.string() + "\"...");
    try {
      KeyValue::Reader kv(sysconf_path);
      while (kv.next()) {
        config[std::string(kv.key())] = std::string(kv.value());
      }
    } catch (Exception &e) {
      WARN("Error reading key-values from system config file \"" + sysconf_location + "\": " + e[0]);
    }
  } else {
    DEBUG(std::string("No config file found at \"") + sysconf_path.string() + "\"");
  }
  std::filesystem::path home_path = Path::home() / ("." + file_basename);
  if (std::filesystem::is_regular_file(home_path)) {
    INFO("reading config file \"" + home_path.string() + "\"...");
    try {
      KeyValue::Reader kv(home_path);
      while (kv.next()) {
        config[std::string(kv.key())] = std::string(kv.value());
      }
    } catch (Exception &e) {
      WARN("Error reading key-values from user config file \"" + home_path.string() + "\": " + e[0]);
    }
  } else {
    DEBUG("No config file found at \"" + home_path.string() + "\"");
  }

  auto opt = App::get_options("config");
  for (const auto &keyval : opt)
    config[std::string(keyval[0])] = std::string(keyval[1]);

  // CONF option: RealignTransform
  // CONF default: 1 (true)
  // CONF A boolean value to indicate whether all images should be realigned
  // CONF to an approximately axial orientation at load.
  Header::do_realign_transform = get_bool("RealignTransform", true);
}

std::optional<std::string> Config::get(std::string_view key) {
  const KeyValues::const_iterator i = config.find(std::string(key));
  return (i != config.end() ? std::optional<std::string>(i->second) : std::nullopt);
}

std::string Config::get(std::string_view key, std::string_view default_value) {
  const KeyValues::const_iterator i = config.find(std::string(key));
  return (i != config.end() ? i->second : std::string(default_value));
}

bool Config::get_bool(std::string_view key, bool default_value) {
  const auto from_config = get(std::string(key));
  if (!from_config.has_value())
    return default_value;
  try {
    return to<bool>(from_config.value());
  } catch (...) {
    WARN("malformed boolean entry \"" + from_config.value() + "\" for key \"" + key + "\"" + //
         " in configuration file - ignored");
    return default_value;
  }
}

int Config::get_int(std::string_view key, int default_value) {
  const auto from_config = get(std::string(key));
  if (!from_config.has_value())
    return default_value;
  try {
    return to<int>(from_config.value());
  } catch (...) {
    WARN("malformed integer entry \"" + from_config.value() + "\" for key \"" + key + "\"" + //
         " in configuration file - ignored");
    return default_value;
  }
}

float Config::get_float(std::string_view key, float default_value) {
  const auto from_config = get(std::string(key));
  if (!from_config.has_value())
    return default_value;
  try {
    return to<float>(from_config.value());
  } catch (...) {
    WARN("malformed floating-point entry \"" + from_config.value() + "\" for key \"" + key + "\"" + //
         " in configuration file - ignored");
    return default_value;
  }
}

Eigen::Array3f Config::get_RGB(std::string_view key, const Eigen::Array3f &default_value) {
  const auto from_config = get(std::string(key));
  if (!from_config.has_value())
    return default_value;
  try {
    std::vector<default_type> V(parse_floats(from_config.value()));
    if (V.size() < 3)
      throw Exception("malformed RGB entry \"" + from_config.value() + "\" for key \"" + key + "\"" + //
                      "in configuration file - ignored");
    return {static_cast<float>(V[0]), static_cast<float>(V[1]), static_cast<float>(V[2])};
  } catch (Exception &) {
    return default_value;
  }
}

} // namespace MR::File
