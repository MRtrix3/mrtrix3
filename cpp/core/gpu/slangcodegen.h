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

#pragma once

#include "exception.h"
#include <array>
#include <cstdint>
#include <future>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <slang-com-ptr.h>
#include <slang.h>

namespace MR::GPU {
struct KernelSpec;
class ShaderCache;
} // namespace MR::GPU

namespace MR::GPU::SlangCodegen {

struct ReflectedBindingInfo {
  uint32_t binding_index = 0;
  slang::VariableLayoutReflection *layout = nullptr;
  slang::ParameterCategory category = slang::ParameterCategory::None;
};

struct SlangCodeGenException : public Exception {
  explicit SlangCodeGenException(std::string_view message)
      : Exception(std::string("Slang codegen error: ") + message.data()) {}
};

// Request a Slang global session asynchronously.
std::future<Slang::ComPtr<slang::IGlobalSession>> request_slang_global_session_async();

// Compile a Slang kernel to WGSL.
// Returns a pair containing the WGSL source string and the linked component type
// for subsequent reflection.
std::pair<std::string, Slang::ComPtr<slang::IComponentType>> compile_kernel_code_to_wgsl(
    const MR::GPU::KernelSpec &kernel_spec, slang::ISession *session, ShaderCache &shader_cache);

// Reflect resource bindings from a linked Slang program layout.
// Produces a map from binding names to their binding index and layout details.
std::unordered_map<std::string, ReflectedBindingInfo> reflect_bindings(slang::ProgramLayout *program_layout);

// Returns the workgroup size specified in the compiled Slang program layout.
std::array<uint32_t, 3> workgroup_size(slang::ProgramLayout *program_layout);
} // namespace MR::GPU::SlangCodegen
