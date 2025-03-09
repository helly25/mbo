// SPDX-FileCopyrightText: Copyright (c) The helly25/mbo authors (helly25.com)
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MBO_CONFIG_CONFIG_H_
#define MBO_CONFIG_CONFIG_H_

#if __has_include("mbo/config/config_gen.h")
# include "mbo/config/config_gen.h"  // IWYU pragma: export
#else
# include "mbo/config/internal/config.h.in"  // IWYU pragma: export
# if __STDC_VERSION__ >= 202'311L
#  if !defined(IS_CLANGD)
#   warning "BAD"
#  endif  // __STDC_VERSION__ >= 202311L
# endif   // !defined(IS_CLANGD)
#endif

#endif  // MBO_CONFIG_CONFIG_H_
