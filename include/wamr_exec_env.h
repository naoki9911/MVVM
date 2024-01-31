//
// Created by victoryang00 on 5/1/23.
//

#ifndef MVVM_WAMR_EXEC_ENV_H
#define MVVM_WAMR_EXEC_ENV_H
#include "wamr_interp_frame.h"
#include "wamr_module_instance.h"
#include "wasm_runtime.h"
#include <memory>
#include <ranges>
#include <tuple>
#include <vector>

struct WAMRExecEnv { // multiple
    /** We need to put Module Inst at the top. */
    WAMRModuleInstance module_inst{};
    /* The thread id of wasm interpreter for current thread. */
    uint32 cur_count{};
    uint32 flags;

    /* Auxiliary stack boundary */
    uint32 aux_boundary;
    /* Auxiliary stack bottom */
    uint32 aux_bottom;
    /* Auxiliary stack top */
    std::vector<std::unique_ptr<WAMRInterpFrame>> frames;

    void dump_impl(WASMExecEnv *env);
    void restore_impl(WASMExecEnv *env);
};

template <SerializerTrait<WASMExecEnv *> T> void dump(T t, WASMExecEnv *env) { t->dump_impl(env); }
template <SerializerTrait<WASMExecEnv *> T> void restore(T t, WASMExecEnv *env) { t->restore_impl(env); }

#endif // MVVM_WAMR_EXEC_ENV_H
