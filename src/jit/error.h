// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
/*****************************************************************************/

#ifndef _ERROR_H_
#define _ERROR_H_
/*****************************************************************************/

#include <corjit.h>   // for CORJIT_INTERNALERROR
#include <safemath.h> // For FitsIn, used by SafeCvt methods.

#define FATAL_JIT_EXCEPTION 0x02345678
class Compiler;

struct ErrorTrapParam
{
    int                errc;
    ICorJitInfo*       jitInfo;
    EXCEPTION_POINTERS exceptionPointers;
    ErrorTrapParam()
    {
        jitInfo = nullptr;
    }
};

// Only catch JIT internal errors (will not catch EE generated Errors)
extern LONG __JITfilter(PEXCEPTION_POINTERS pExceptionPointers, LPVOID lpvParam);

#define setErrorTrap(compHnd, ParamType, paramDef, paramRef)                                                           \
    struct __JITParam : ErrorTrapParam                                                                                 \
    {                                                                                                                  \
        ParamType param;                                                                                               \
    } __JITparam;                                                                                                      \
    __JITparam.errc    = CORJIT_INTERNALERROR;                                                                         \
    __JITparam.jitInfo = compHnd;                                                                                      \
    __JITparam.param   = paramRef;                                                                                     \
    PAL_TRY(__JITParam*, __JITpParam, &__JITparam)                                                                     \
    {                                                                                                                  \
        ParamType paramDef = __JITpParam->param;

// Only catch JIT internal errors (will not catch EE generated Errors)
#define impJitErrorTrap()                                                                                              \
    }                                                                                                                  \
    PAL_EXCEPT_FILTER(__JITfilter)                                                                                     \
    {                                                                                                                  \
        int __errc = __JITparam.errc;                                                                                  \
        (void)__errc;

#define endErrorTrap()                                                                                                 \
    }                                                                                                                  \
    PAL_ENDTRY

#define finallyErrorTrap()                                                                                             \
    }                                                                                                                  \
    PAL_FINALLY                                                                                                        \
    {

/*****************************************************************************/

// clang-format off

extern void debugError(const char* msg, const char* file, unsigned line);
extern void DECLSPEC_NORETURN badCode();
extern void DECLSPEC_NORETURN badCode3(const char* msg, const char* msg2, int arg, __in_z const char* file, unsigned line);
extern void DECLSPEC_NORETURN noWay();
extern void DECLSPEC_NORETURN NOMEM();
extern void DECLSPEC_NORETURN fatal(int errCode);

extern void DECLSPEC_NORETURN noWayAssertBody();
extern void DECLSPEC_NORETURN noWayAssertBody(const char* cond, const char* file, unsigned line);

// Conditionally invoke the noway assert body. The conditional predicate is evaluated using a method on the tlsCompiler.
// If a noway_assert is hit, we ask the Compiler whether to raise an exception (i.e., conditionally raise exception.)
// To have backward compatibility between v4.5 and v4.0, in min-opts we take a shot at codegen rather than rethrow.
extern void noWayAssertBodyConditional(
#ifdef FEATURE_TRACELOGGING
    const char* file, unsigned line
#endif
    );
extern void noWayAssertBodyConditional(const char* cond, const char* file, unsigned line);

// Define MEASURE_NOWAY to 1 to enable code to count and rank individual noway_assert calls by occurrence.
// These asserts would be dynamically executed, but not necessarily fail. The provides some insight into
// the dynamic prevalence of these (if not a direct measure of their cost), which exist in non-DEBUG as
// well as DEBUG builds.
#ifdef DEBUG
#define MEASURE_NOWAY 1
#else // !DEBUG
#define MEASURE_NOWAY 0
#endif // !DEBUG

#if MEASURE_NOWAY
extern void RecordNowayAssertGlobal(const char* filename, unsigned line, const char* condStr);
#define RECORD_NOWAY_ASSERT(condStr) RecordNowayAssertGlobal(__FILE__, __LINE__, condStr);
#else
#define RECORD_NOWAY_ASSERT(condStr)
#endif

#ifdef DEBUG

#define NO_WAY(msg) (debugError(msg, __FILE__, __LINE__), noWay())
// Used for fallback stress mode
#define NO_WAY_NOASSERT(msg) noWay()
#define BADCODE(msg) (debugError(msg, __FILE__, __LINE__), badCode())
#define BADCODE3(msg, msg2, arg) badCode3(msg, msg2, arg, __FILE__, __LINE__)
// Used for an assert that we want to convert into BADCODE to force minopts, or in minopts to force codegen.
#define noway_assert(cond)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        RECORD_NOWAY_ASSERT(#cond)                                                                                     \
        if (!(cond))                                                                                                   \
        {                                                                                                              \
            noWayAssertBodyConditional(#cond, __FILE__, __LINE__);                                                     \
        }                                                                                                              \
    } while (0)
#define unreached() noWayAssertBody("unreached", __FILE__, __LINE__)

#define NOWAY_MSG(msg) noWayAssertBodyConditional(msg, __FILE__, __LINE__)

#else // !DEBUG

#define NO_WAY(msg) noWay()
#define BADCODE(msg) badCode()
#define BADCODE3(msg, msg2, arg) badCode()

#ifdef FEATURE_TRACELOGGING
#define NOWAY_ASSERT_BODY_ARGUMENTS __FILE__, __LINE__
#else
#define NOWAY_ASSERT_BODY_ARGUMENTS
#endif

#define noway_assert(cond)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        RECORD_NOWAY_ASSERT(#cond)                                                                                     \
        if (!(cond))                                                                                                   \
        {                                                                                                              \
            noWayAssertBodyConditional(NOWAY_ASSERT_BODY_ARGUMENTS);                                                   \
        }                                                                                                              \
    } while (0)
#define unreached() noWayAssertBody()

#define NOWAY_MSG(msg) noWayAssertBodyConditional(NOWAY_ASSERT_BODY_ARGUMENTS)

#endif // !DEBUG

// IMPL_LIMITATION is called when we encounter valid IL that is not
// supported by our current implementation because of various
// limitations (that could be removed in the future)
#define IMPL_LIMITATION(msg) NO_WAY(msg)

#if !defined(_TARGET_X86_) || !defined(LEGACY_BACKEND)

#if defined(ALT_JIT)

// This guy can return based on Config flag/Debugger
extern void notYetImplemented(const char* msg, const char* file, unsigned line);
#define NYIRAW(msg) notYetImplemented(msg, __FILE__, __LINE__)

#else // !defined(ALT_JIT)

#define NYIRAW(msg) NOWAY_MSG(msg)

#endif // !defined(ALT_JIT)

#define NYI(msg)                    NYIRAW("NYI: " msg)
#define NYI_IF(cond, msg) if (cond) NYIRAW("NYI: " msg)

#ifdef _TARGET_AMD64_

#define NYI_AMD64(msg)  NYIRAW("NYI_AMD64: " msg)
#define NYI_X86(msg)    do { } while (0)
#define NYI_ARM(msg)    do { } while (0)
#define NYI_ARM64(msg)  do { } while (0)

#elif defined(_TARGET_X86_)

#define NYI_AMD64(msg)  do { } while (0)
#define NYI_X86(msg)    NYIRAW("NYI_X86: " msg)
#define NYI_ARM(msg)    do { } while (0)
#define NYI_ARM64(msg)  do { } while (0)

#elif defined(_TARGET_ARM_)

#define NYI_AMD64(msg)  do { } while (0)
#define NYI_X86(msg)    do { } while (0)
#define NYI_ARM(msg)    NYIRAW("NYI_ARM: " msg)
#define NYI_ARM64(msg)  do { } while (0)

#elif defined(_TARGET_ARM64_)

#define NYI_AMD64(msg)  do { } while (0)
#define NYI_X86(msg)    do { } while (0)
#define NYI_ARM(msg)    do { } while (0)
#define NYI_ARM64(msg)  NYIRAW("NYI_ARM64: " msg)

#else

#error "Unknown platform, not x86, ARM, or AMD64?"

#endif

#else // NYI not available; make it an assert.

#define NYI(msg)        assert(!msg)
#define NYI_AMD64(msg)  do { } while (0)
#define NYI_ARM(msg)    do { } while (0)
#define NYI_ARM64(msg)  do { } while (0)

#endif // NYI not available

#if !defined(_TARGET_X86_) && !defined(FEATURE_STACK_FP_X87)

#define NYI_FLAT_FP_X87(msg)    NYI(msg)
#define NYI_FLAT_FP_X87_NC(msg) NYI(msg)

#else

#define NYI_FLAT_FP_X87(msg)    do { } while (0)
#define NYI_FLAT_FP_X87_NC(msg) do { } while (0)

#endif // !_TARGET_X86_ && !FEATURE_STACK_FP_X87

// clang-format on

#if defined(_HOST_X86_) && !defined(FEATURE_PAL)

// While debugging in an Debugger, the "int 3" will cause the program to break
// Outside, the exception handler will just filter out the "int 3".

#define BreakIfDebuggerPresent()                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        __try                                                                                                          \
        {                                                                                                              \
            __asm {int 3}                                                                                              \
        }                                                                                                              \
        __except (EXCEPTION_EXECUTE_HANDLER)                                                                           \
        {                                                                                                              \
        }                                                                                                              \
    } while (0)

#else
#define BreakIfDebuggerPresent()                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (IsDebuggerPresent())                                                                                       \
            DebugBreak();                                                                                              \
    } while (0)
#endif

#ifdef DEBUG
DWORD getBreakOnBadCode();
#endif

// For narrowing numeric conversions, the following two methods ensure that the
// source value fits in the destination type, using either "assert" or
// "noway_assert" to validate the conversion.  Obviously, each returns the source value as
// the destination type.

// (There is an argument that these should be macros, to let the preprocessor capture
// a more useful file/line for the error message.  But then we have to use comma expressions
// so that these can be used in expressions, etc., which is ugly.  So I propose we rely on
// getting stack traces in other ways.)
template <typename Dst, typename Src>
inline Dst SafeCvtAssert(Src val)
{
    assert(FitsIn<Dst>(val));
    return static_cast<Dst>(val);
}

template <typename Dst, typename Src>
inline Dst SafeCvtNowayAssert(Src val)
{
    noway_assert(FitsIn<Dst>(val));
    return static_cast<Dst>(val);
}

#endif
