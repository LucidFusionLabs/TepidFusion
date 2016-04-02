/*
 * $Id$
 * Copyright (C) 2009 Lucid Fusion Labs

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/app/app.h"
#include "core/app/gui.h"
#include "core/app/ipc.h"

#include "json/json.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/BitWriter.h>

namespace LFL {
  LLVMMemoryBufferRef code_buf;
  LLVMModuleRef mod; // = LLVMModuleCreateWithName("my_module");
  LLVMExecutionEngineRef engine;

bool LoadBitcode(const string &fn) {
  char *error = nullptr;
  if (LLVMCreateMemoryBufferWithContentsOfFile(fn.c_str(), &code_buf, &error) || error)
    return ERRORv(false, "LLVMCreateMemoryBufferWithContentsOfFile: ", BlankNull(error));

  if (LLVMParseBitcode(code_buf, &mod, &error) || error)
  { LLVMDisposeMemoryBuffer(code_buf); return ERRORv(false, "LLVMParseBitcode: ", BlankNull(error)); }

  // LLVMLinkInInterpreter();
  LLVMLinkInMCJIT();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeTarget();
  if (LLVMCreateExecutionEngineForModule(&engine, mod, &error) || error)
  { LLVMDisposeMemoryBuffer(code_buf); return ERRORv(false, "LLVMCreateExecutionEngineForModule: ", BlankNull(error)); }
  
  LLVMValueRef app_create = LLVMGetNamedFunction(mod, "MyAppCreate");
  LLVMValueRef app_main   = LLVMGetNamedFunction(mod, "MyAppMain");
  if (!app_create) { LLVMDisposeExecutionEngine(engine); LLVMDisposeMemoryBuffer(code_buf); return ERRORv(false, "LLVMGetNamedFunction MyAppCreate"); }
  if (!app_main)   { LLVMDisposeExecutionEngine(engine); LLVMDisposeMemoryBuffer(code_buf); return ERRORv(false, "LLVMGetNamedFunction MyAppMain"); }
  
  LLVMGenericValueRef rv = LLVMRunFunction(engine, app_create, 0, nullptr);
  printf("rv %p\n", rv);

  vector<const char*> av{
    // "main", 
    "/Users/p/lfl/core/app/app.h", nullptr };
    // "-project", "/Users/p/lfl/osx/compile_commands.json",
  int ret = LLVMRunFunctionAsMain(engine, app_main, av.size()-1, av.data(), nullptr);
  printf("main ret: %d\n", ret);

  return true;
  LLVMDisposeExecutionEngine(engine);
  LLVMDisposeMemoryBuffer(code_buf);
  return true;
}
}; // namespace LFL
using namespace LFL;

extern "C" void MyAppCreate() {}
extern "C" int MyAppMain(int argc, const char* const* argv) {
  if (argc < 2) { fprintf(stderr, "usage: %s <bitcode file>\n", argv[0]); return -1; }
  LoadBitcode(argv[1]);
  return 0;
}
