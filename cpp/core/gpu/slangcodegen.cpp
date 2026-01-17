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

#include "slangcodegen.h"

#include "exception.h"
#include "gpu/gpu.h"
#include "match_variant.h"
#include "shadercache.h"

#include <functional>
#include <slang-com-ptr.h>
#include <slang.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <ios>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace MR::GPU::SlangCodegen {

namespace {

enum ReadFileMode : uint8_t { Text, Binary };
std::string read_file(const std::filesystem::path &filePath, ReadFileMode mode = ReadFileMode::Text) {
  using namespace std::string_literals;
  if (!std::filesystem::exists(filePath)) {
    throw std::runtime_error("File not found: "s + filePath.string());
  }

  const auto openMode = (mode == ReadFileMode::Binary) ? std::ios::in | std::ios::binary : std::ios::in;
  std::ifstream f(filePath, std::ios::in | openMode);
  const auto fileSize64 = std::filesystem::file_size(filePath);
  if (fileSize64 > static_cast<uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
    throw std::runtime_error("File too large to read into memory: "s + filePath.string());
  }
  const std::streamsize fileSize = static_cast<std::streamsize>(fileSize64);
  std::string result(static_cast<std::string::size_type>(fileSize), '\0');
  f.read(result.data(), fileSize);

  return result;
}

std::string hash_string(std::string_view input) {
  const std::hash<std::string> hasher;
  const size_t hashValue = hasher(std::string(input));
  return std::to_string(hashValue);
}

void check_slang_result(SlangResult res,
                        std::string_view errorMessage = "",
                        const Slang::ComPtr<slang::IBlob> &diagnostics = nullptr) {
  if (SLANG_FAILED(res)) {
    std::string full_error = "Slang Error: " + errorMessage;
    if (diagnostics != nullptr) {
      const std::string diag_string =
          std::string(static_cast<const char *>(diagnostics->getBufferPointer()), diagnostics->getBufferSize());
      if (!diag_string.empty()) {
        full_error += "\nDiagnostics:\n" + diag_string;
      }
    }
    throw SlangCodeGenException(full_error);
  }
}

void find_bindings_in_type_layout(slang::TypeLayoutReflection *typeLayout,
                                  std::unordered_map<std::string, ReflectedBindingInfo> &bindings);

struct EntryPointSelection {
  SlangUInt index = 0;
  slang::EntryPointLayout *layout = nullptr;
  std::string name;
};

EntryPointSelection select_entry_point(slang::ProgramLayout *programLayout, std::string_view requested_entry_point) {
  if (programLayout == nullptr) {
    throw SlangCodeGenException("Slang program layout is null!");
  }

  const SlangUInt entry_point_count = programLayout->getEntryPointCount();
  if (entry_point_count == 0) {
    throw SlangCodeGenException("Slang program layout has no entry points!");
  }

  for (SlangUInt i = 0; i < entry_point_count; ++i) {
    auto *entry_point_layout = programLayout->getEntryPointByIndex(i);
    if (entry_point_layout == nullptr) {
      continue;
    }

    const char *const name_override = entry_point_layout->getNameOverride(); // check_syntax off
    const char *const name = entry_point_layout->getName();                  // check_syntax off

    const bool override_matches = (name_override != nullptr) && (requested_entry_point == name_override);
    const bool name_matches = (name != nullptr) && (requested_entry_point == name);

    if (!override_matches && !name_matches) {
      continue;
    }

    const std::string resolved_name = (name_override != nullptr) ? name_override : (name != nullptr) ? name : "";
    return EntryPointSelection{.index = i, .layout = entry_point_layout, .name = resolved_name};
  }

  // Produce a human-readable list of available entry points
  std::string available;
  for (SlangUInt i = 0; i < entry_point_count; ++i) {
    auto *entry_point_layout = programLayout->getEntryPointByIndex(i);
    if (entry_point_layout == nullptr) {
      continue;
    }
    const char *const name_override = entry_point_layout->getNameOverride(); // check_syntax off
    const char *const name = entry_point_layout->getName();                  // check_syntax off
    const char *const resolved = (name_override != nullptr) ? name_override : name;
    if (resolved == nullptr) {
      continue;
    }

    if (!available.empty()) {
      available += ", ";
    }
    available += resolved;
  }

  throw SlangCodeGenException("Failed to find entry point '" + std::string(requested_entry_point) +
                              "' in linked Slang program layout. Available entry points: [" + available + "]");
}

void find_bindings_in_variable_layout(slang::VariableLayoutReflection *varLayout,
                                      std::unordered_map<std::string, ReflectedBindingInfo> &bindings) {
  if (varLayout == nullptr) {
    return;
  }

  const char *var_name = varLayout->getName(); // check_syntax off

  if (var_name != nullptr) {
    for (uint32_t i = 0; i < varLayout->getCategoryCount(); ++i) {
      auto category = varLayout->getCategoryByIndex(i);
      if (category == slang::ParameterCategory::DescriptorTableSlot) {
        // This is a Texture, Buffer, or Sampler.
        const ReflectedBindingInfo binding_info{.binding_index = static_cast<uint32_t>(varLayout->getOffset(category)),
                                                .layout = varLayout,
                                                .category = category};

        bindings[var_name] = binding_info;
        break; // A variable can only have one slot binding.
      }
    }
  } else {
    // This is an anonymous variable (e.g., the element inside a ConstantBuffer).
    // It doesn't have a name or binding itself, but we must traverse its type.
    // We pass the parent's path along without modification.
    find_bindings_in_type_layout(varLayout->getTypeLayout(), bindings);
  }
}

// Traverses the members of a type layout (like a struct or container).
void find_bindings_in_type_layout(slang::TypeLayoutReflection *typeLayout,
                                  std::unordered_map<std::string, ReflectedBindingInfo> &bindings) {
  if (typeLayout == nullptr)
    return;

  switch (typeLayout->getKind()) {
  case slang::TypeReflection::Kind::Struct: {
    // For a struct, iterate over its fields and process each one.
    for (SlangUInt i = 0; i < typeLayout->getFieldCount(); ++i) {
      find_bindings_in_variable_layout(typeLayout->getFieldByIndex(i), bindings);
    }
    break;
  }

  case slang::TypeReflection::Kind::ConstantBuffer:
  case slang::TypeReflection::Kind::ParameterBlock: {
    // For a container, we get the layout of its contents.
    // getElementVarLayout() is the key idiomatic call here.
    slang::VariableLayoutReflection *element_layout = typeLayout->getElementVarLayout();
    find_bindings_in_variable_layout(element_layout, bindings);
    break;
  }

  default:
    // Other types (Scalar, Vector, Array, etc.) don't contain resource bindings themselves.
    break;
  }
}

} // anonymous namespace

std::future<Slang::ComPtr<slang::IGlobalSession>> request_slang_global_session_async() {
  auto r = std::async(std::launch::async, []() {
    Slang::ComPtr<slang::IGlobalSession> global_session;
    const SlangGlobalSessionDesc global_session_desc;
    check_slang_result(createGlobalSession(&global_session_desc, global_session.writeRef()),
                       "Failed to create Slang global session!");
    return global_session;
  });

  return r;
}

CompiledKernelWGSL compile_kernel_code_to_wgsl(const MR::GPU::KernelSpec &kernel_spec,
                                               slang::ISession *session,
                                               ShaderCache &shader_cache) {
  Slang::ComPtr<slang::IBlob> diagnostics;
  Slang::ComPtr<slang::IModule> shader_module;

  auto log_diagnostics = [&diagnostics]() {
    if (diagnostics != nullptr) {
      const std::string diag_string =
          std::string(static_cast<const char *>(diagnostics->getBufferPointer()), diagnostics->getBufferSize());
      if (!diag_string.empty()) {
        DEBUG("Slang diagnostics:\n" + diag_string);
      }
    }
  };

  MR::match_v(
      kernel_spec.compute_shader.shader_source,
      [&](const ShaderFile &shaderFile) {
        const auto shader_path_string = shaderFile.file_path.string();
        const std::string module_name = std::filesystem::path(shader_path_string).stem().string();
        const auto shader_source = read_file(shaderFile.file_path);
        shader_module = session->loadModuleFromSourceString(
            module_name.c_str(), shader_path_string.c_str(), shader_source.c_str(), diagnostics.writeRef());
      },
      [&](const InlineShaderText &inlineString) {
        const std::string path_string = "inline_" + hash_string(inlineString.text);
        // Use the unique path string as the module name to prevent collisions between different inline shaders
        // that might otherwise share the same default name.
        shader_module = session->loadModuleFromSourceString(
            path_string.c_str(), path_string.c_str(), inlineString.text.c_str(), diagnostics.writeRef());
      });
  log_diagnostics();
  if (shader_module == nullptr) {
    throw SlangCodeGenException("Failed to load shader module: " + kernel_spec.compute_shader.name);
  }

  Slang::ComPtr<slang::IEntryPoint> entry_point;
  check_slang_result(
      shader_module->findEntryPointByName(kernel_spec.compute_shader.entryPoint.c_str(), entry_point.writeRef()),
      "Slang failed to findEntryPointByName",
      diagnostics);

  const auto &generic_type_args = kernel_spec.compute_shader.entry_point_args;

  // Specialisation arguments
  Slang::ComPtr<slang::IComponentType> specialized_entry_point;
  {
    std::vector<slang::SpecializationArg> slang_generic_args(generic_type_args.size());
    if (!generic_type_args.empty()) {
      auto *program_layout = shader_module->getLayout();
      std::transform(generic_type_args.begin(),
                     generic_type_args.end(),
                     slang_generic_args.begin(),
                     [program_layout](const std::string &arg) { // check_syntax off
                       auto *const spec_type = program_layout->findTypeByName(arg.c_str());
                       if (spec_type == nullptr) {
                         throw SlangCodeGenException("Failed to find specialization type: " + arg);
                       }
                       return slang::SpecializationArg{slang::SpecializationArg::Kind::Type, spec_type};
                     });
      check_slang_result(entry_point->specialize(slang_generic_args.data(),
                                                 static_cast<SlangInt>(slang_generic_args.size()),
                                                 specialized_entry_point.writeRef(),
                                                 diagnostics.writeRef()),
                         "Slang failed to specialise entry point",
                         diagnostics);
    }
  }

  Slang::ComPtr<slang::IComponentType> slang_program;
  std::vector<slang::IComponentType *> shader_components;
  shader_components.push_back(shader_module.get());
  if (!generic_type_args.empty()) {
    shader_components.push_back(specialized_entry_point.get());
  } else {
    shader_components.push_back(entry_point.get());
  }

  if (kernel_spec.compute_shader.workgroup_size.has_value()) {
    const auto &wg_size = *kernel_spec.compute_shader.workgroup_size;
    std::ostringstream oss;
    oss << "export static const uint kWorkgroupSizeX = " << wg_size.x << ";\n"
        << "export static const uint kWorkgroupSizeY = " << wg_size.y << ";\n"
        << "export static const uint kWorkgroupSizeZ = " << wg_size.z << ";\n";

    const std::string workgroup_size_constants = oss.str();
    const std::string workgroup_size_constants_name =
        "workgroup_size_constants_" + hash_string(workgroup_size_constants);
    Slang::ComPtr<slang::IModule> workgroup_size_constants_module;
    workgroup_size_constants_module = session->loadModuleFromSourceString(workgroup_size_constants_name.c_str(),
                                                                          workgroup_size_constants_name.c_str(),
                                                                          workgroup_size_constants.data(),
                                                                          diagnostics.writeRef());
    shader_components.push_back(workgroup_size_constants_module.get());
  }

  if (!kernel_spec.compute_shader.constants.empty()) {
    std::ostringstream oss;
    for (const auto &[name, value] : kernel_spec.compute_shader.constants) {
      MR::match_v(
          value,
          [&oss, name = name](int32_t v) { oss << "export static const int32_t " << name << " = " << v << ";\n"; },
          [&oss, name = name](uint32_t v) { oss << "export static const uint32_t " << name << " = " << v << ";\n"; },
          [&oss, name = name](float v) { oss << "export static const float " << name << " = " << v << ";\n"; });
    }

    const std::string constant_definitions = oss.str();
    const std::string constant_definitions_name = "constant_definitions_" + hash_string(constant_definitions);
    Slang::ComPtr<slang::IModule> constant_definitions_module;
    constant_definitions_module = session->loadModuleFromSourceString(constant_definitions_name.c_str(),
                                                                      constant_definitions_name.c_str(),
                                                                      constant_definitions.data(),
                                                                      diagnostics.writeRef());
    shader_components.push_back(constant_definitions_module.get());
  }

  check_slang_result(session->createCompositeComponentType(
      shader_components.data(), static_cast<SlangInt>(shader_components.size()), slang_program.writeRef()));

  Slang::ComPtr<slang::IComponentType> linked_slang_program;
  check_slang_result(slang_program->link(linked_slang_program.writeRef(), diagnostics.writeRef()),
                     "Slang failed to link program",
                     diagnostics);

  const auto entry_point_selection =
      select_entry_point(linked_slang_program->getLayout(), kernel_spec.compute_shader.entryPoint);

  Slang::ComPtr<slang::IBlob> hash_blob;
  linked_slang_program->getEntryPointHash(entry_point_selection.index, 0, hash_blob.writeRef());
  const std::string hash_key =
      std::string(static_cast<const char *>(hash_blob->getBufferPointer()), hash_blob->getBufferSize());

  std::string wgsl_code;
  Slang::ComPtr<slang::IBlob> slang_kernel_blob;
  if (shader_cache.contains(hash_key)) {
    wgsl_code = shader_cache.get(hash_key);
  } else {
    check_slang_result(linked_slang_program->getEntryPointCode(
                           entry_point_selection.index, 0, slang_kernel_blob.writeRef(), diagnostics.writeRef()),
                       "Slang failed to get entry point code",
                       diagnostics);
    wgsl_code = std::string(static_cast<const char *>(slang_kernel_blob->getBufferPointer()),
                            slang_kernel_blob->getBufferSize());
    shader_cache.insert(hash_key, wgsl_code);
  }

  DEBUG(kernel_spec.compute_shader.name + " WGSL code:\n" + wgsl_code);
  return CompiledKernelWGSL{
      .wgsl_source = wgsl_code, .linked_program = linked_slang_program, .entry_point_name = entry_point_selection.name};
}

std::unordered_map<std::string, ReflectedBindingInfo> reflect_bindings(slang::ProgramLayout *program_layout,
                                                                       std::string_view entry_point_name) {
  std::unordered_map<std::string, ReflectedBindingInfo> bindings_map;
  const auto entry_point_selection = select_entry_point(program_layout, entry_point_name);

  auto *global_var_layout = program_layout->getGlobalParamsVarLayout();
  if (global_var_layout != nullptr) {
    // If the program has global variables, we can find bindings in them.
    find_bindings_in_variable_layout(global_var_layout, bindings_map);
  }

  auto *entry_point_layout = entry_point_selection.layout;

  auto *entry_point_root_variable_layout = entry_point_layout->getVarLayout();
  if (entry_point_root_variable_layout == nullptr) {
    // This can happen if the entry point has no uniform parameters.
    return bindings_map;
  }

  auto *entry_point_root_type_layout = entry_point_root_variable_layout->getTypeLayout();
  if (entry_point_root_type_layout == nullptr) {
    throw SlangCodeGenException("Slang entry point variable layout has no type layout!");
  }

  find_bindings_in_variable_layout(entry_point_root_variable_layout, bindings_map);
  return bindings_map;
}

std::array<uint32_t, 3> workgroup_size(slang::ProgramLayout *program_layout, std::string_view entry_point_name) {
  const auto entry_point_selection = select_entry_point(program_layout, entry_point_name);
  auto *entry_point_layout = entry_point_selection.layout;

  std::array<SlangUInt, 3> wg_size{};
  entry_point_layout->getComputeThreadGroupSize(3, wg_size.data());

  return {static_cast<uint32_t>(wg_size[0]), static_cast<uint32_t>(wg_size[1]), static_cast<uint32_t>(wg_size[2])};
}

} // namespace MR::GPU::SlangCodegen
