add_llvm_library( LLVMLoopsInfo MODULE
  LoopsInfo.cpp

  PLUGIN_TOOL
  opt
  )
