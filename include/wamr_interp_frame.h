//
// Created by yiwei yang on 5/1/23.
//

#ifndef MVVM_WAMR_INTERP_FRAME_H
#define MVVM_WAMR_INTERP_FRAME_H
#include "wamr_branch_block.h"
#include "wamr_function_instance.h"
#include "wasm_interp.h"
#include "wasm_runtime.h"
#include <memory>
#include <optional>
struct WAMRInterpFrame {
    /* Instruction pointer of the bytecode array.  */
    std::optional<uint32> ip;

    // #if WASM_ENABLE_FAST_JIT != 0
    //     uint8 *jitted_return_addr;
    // #endif
    WAMRFunctionInstance function;

    // #if WASM_ENABLE_PERF_PROFILING != 0
    //     uint64 time_started;
    // #endif

    // #if WASM_ENABLE_FAST_INTERP != 0
    //     /* Return offset of the first return value of current frame,
    //    the callee will put return values here continuously */
    //     uint32 ret_offset;
    //     uint32 *lp;
    //     uint32 operand[1];
    // #else
    /* Operand stack top pointer of the current frame. The bottom of
       the stack is the next cell after the last local variable. */
    std::optional<uint32> sp; // all the sp that can be restart
    std::optional<std::vector<WAMRBranchBlock>> csp;

    /*
     * Frame data, the layout is:
     *  lp: parameters and local variables
     *  sp_bottom to sp_boundary: wasm operand stack
     *  csp_bottom to csp_boundary: wasm label stack
     *  jit spill cache: only available for fast jit
     */
    std::optional<uint32> lp; // this thing is dynamic allocated
    // #endif

    void dump(WASMInterpFrame *env);
    void restore(WASMInterpFrame *env){

    };
};
template <SerializerTrait<WASMInterpFrame *> T> void dump(T t, WASMInterpFrame *env) { t->dump(env); }
template <SerializerTrait<WASMInterpFrame *> T> void restore(T t, WASMInterpFrame *env) { t->restore(env); }

#endif // MVVM_WAMR_INTERP_FRAME_H