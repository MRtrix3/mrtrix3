/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "app.h"
#include "debug.h"
#include "header.h"

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
  const char *sysconf_location_env = getenv("MRTRIX_CONFIGFILE"); // check_syntax off
  const std::string sysconf_location(sysconf_location_env == nullptr ? default_sys_config_file
                                                                     : std::string(sysconf_location_env));

  if (Path::is_file(sysconf_location)) {
    INFO("reading config file \"" + sysconf_location + "\"...");
    try {
      KeyValue::Reader kv(sysconf_location);
      while (kv.next()) {
        config[std::string(kv.key())] = std::string(kv.value());
      }
    } catch (...) {
    }
  } else {
    DEBUG("No config file found at \"" + sysconf_location + "\"");
  }

  const std::string path = Path::join(Path::home(), "." + file_basename);
  if (Path::is_file(path)) {
    INFO("reading config file \"" + path + "\"...");
    try {
      KeyValue::Reader kv(path);
      while (kv.next()) {
        config[std::string(kv.key())] = std::string(kv.value());
      }
    } catch (...) {
    }
  } else {
    DEBUG("No config file found at \"" + path + "\"");
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

std::string Config::get(std::string_view key) {
  const KeyValues::const_iterator i = config.find(std::string(key));
  return (i != config.end() ? i->second : "");
}

std::string Config::get(std::string_view key, std::string_view default_value) {
  const KeyValues::const_iterator i = config.find(std::string(key));
  return (i != config.end() ? i->second : std::string(default_value));
}

bool Config::get_bool(std::string_view key, bool default_value) {
  const std::string value = get(std::string(key));
  if (value.empty())
    return default_value;
  try {
    return to<bool>(value);
  } catch (...) {
    WARN("malformed boolean entry \"" + value + "\" for key \"" + key + "\" in configuration file - ignored");
    return default_value;
  }
}

int Config::get_int(std::string_view key, int default_value) {
  const std::string value = get(std::string(key));
  if (value.empty())
    return default_value;
  try {
    return to<int>(value);
  } catch (...) {
    WARN("malformed integer entry \"" + value + "\" for key \"" + key + "\" in configuration file - ignored");
    return default_value;
  }
}

float Config::get_float(std::string_view key, float default_value) {
  const std::string value = get(std::string(key));
  if (value.empty())
    return default_value;
  try {
    return to<float>(value);
  } catch (...) {
    WARN("malformed floating-point entry \"" + value + "\" for key \"" + key + "\" in configuration file - ignored");
    return default_value;
  }
}

void Config::get_RGB(std::string_view key, float *ret, float default_R, float default_G, float default_B) {
  const std::string value = get(std::string(key));
  if (!value.empty()) {
    try {
      std::vector<default_type> V(parse_floats(value));
      if (V.size() < 3)
        throw Exception("malformed RGB entry \"" + value + "\" for key \"" + key +
                        "\" in configuration file - ignored");
      ret[0] = V[0];
      ret[1] = V[1];
      ret[2] = V[2];
    } catch (Exception) {
    }
  } else {
    ret[0] = default_R;
    ret[1] = default_G;
    ret[2] = default_B;
  }
}

} // namespace MR::File
