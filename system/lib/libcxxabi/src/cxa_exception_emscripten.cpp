//===------------------- cxa_exception_emscripten.cpp ---------------------===//
//
// This code contains Emscripten specific code for exception handling.
// Emscripten has two modes of exception handling: Emscripten EH, which uses JS
// glue code, and Wasm EH, which uses the new Wasm exception handling proposal
// and meant to be faster. Code for different modes is demarcated with
// '__USING_EMSCRIPTEN_EXCEPTIONS__' and '__USING_WASM_EXCEPTIONS__'.
//
//===----------------------------------------------------------------------===//

#include "cxa_exception.h"
#include "private_typeinfo.h"
#include <stdio.h>

#if defined(__USING_EMSCRIPTEN_EXCEPTIONS__) ||                                \
  defined(__USING_WASM_EXCEPTIONS__)

using namespace __cxxabiv1;

//  Some utility routines are copied from cxa_exception.cpp
static inline __cxa_exception*
cxa_exception_from_thrown_object(void* thrown_object) {
  return static_cast<__cxa_exception*>(thrown_object) - 1;
}

// Note:  This is never called when exception_header is masquerading as a
//        __cxa_dependent_exception.
static inline void*
thrown_object_from_cxa_exception(__cxa_exception* exception_header) {
  return static_cast<void*>(exception_header + 1);
}

//  Get the exception object from the unwind pointer.
//  Relies on the structure layout, where the unwind pointer is right in
//  front of the user's exception object
static inline __cxa_exception* cxa_exception_from_unwind_exception(
  _Unwind_Exception* unwind_exception) {
  return cxa_exception_from_thrown_object(unwind_exception + 1);
}

static inline void* thrown_object_from_unwind_exception(
  _Unwind_Exception* unwind_exception) {
  __cxa_exception* exception_header =
    cxa_exception_from_unwind_exception(unwind_exception);
  return thrown_object_from_cxa_exception(exception_header);
}

extern "C" {

void* __thrown_object_from_unwind_exception(
  _Unwind_Exception* unwind_exception) {
  return thrown_object_from_unwind_exception(unwind_exception);
}

char* __get_exception_message(void* thrown_object, bool terminate=false) {
  __cxa_exception* exception_header =
    cxa_exception_from_thrown_object(thrown_object);
  const __shim_type_info* thrown_type =
    static_cast<const __shim_type_info*>(exception_header->exceptionType);
  const char* type_name = thrown_type->name();

  int status = 0;
  char* demangled_buf = __cxa_demangle(type_name, 0, 0, &status);
  if (status == 0 && demangled_buf) {
    type_name = demangled_buf;
  }

  const __shim_type_info* catch_type =
    static_cast<const __shim_type_info*>(&typeid(std::exception));
  int can_catch = catch_type->can_catch(thrown_type, thrown_object);
  char* result = NULL;
  if (can_catch) {
    const char* what =
      static_cast<const std::exception*>(thrown_object)->what();
    asprintf(&result,
             (terminate ? "terminating with uncaught exception of type %s: %s"
                        : "exception of type %s: %s"),
             type_name,
             what);
  } else {
    asprintf(&result,
             (terminate ? "terminating with uncaught exception of type %s"
                        : "exception of type %s"),
             type_name);
  }

  if (demangled_buf) {
    free(demangled_buf);
  }
  return result;
}

char* __get_exception_terminate_message(void *thrown_object) {
  return __get_exception_message(thrown_object, true);
}

}

#endif // __USING_EMSCRIPTEN_EXCEPTIONS__ || __USING_WASM_EXCEPTIONS__
