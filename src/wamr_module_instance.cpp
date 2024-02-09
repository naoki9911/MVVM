#include "wamr_module_instance.h"
#include "aot_runtime.h"
#include "wamr.h"

extern WAMRInstance *wamr;

void WAMRModuleInstance::dump_impl(WASMModuleInstance *env) {
    // The first thread will dump the memory

    // if (((WAMRExecEnv *)this)->cur_count == wamr->exec_env->handle) {
    for (int i = 0; i < env->memory_count; i++) {
        auto local_mem = WAMRMemoryInstance();
        dump(&local_mem, env->memories[i]);
        memories.push_back(local_mem);
    }
    // }
    global_data = std::vector<uint8>(env->global_data, env->global_data + env->global_data_size);
    dump(&wasi_ctx, &env->module->wasi_args);
    // for (int i = 0; i < env->global_data_size; i++) {
    //     fprintf(stderr, "%d", env->global_data[i]);
    // }
    fprintf(stderr, "\nDumped global data ptr: %p\n", env->global_data);
    if (wamr->is_aot) {
        auto module = (AOTModule *)env->module;
        aux_data_end_global_index = module->aux_data_end_global_index;
        aux_data_end = module->aux_data_end;
        aux_heap_base_global_index = module->aux_heap_base_global_index;
        aux_heap_base = module->aux_heap_base;
        aux_stack_top_global_index = module->aux_stack_top_global_index;
        aux_stack_bottom = module->aux_stack_bottom;
        aux_stack_size = module->aux_stack_size;
    } else {
        auto module = env->module;
        aux_data_end_global_index = module->aux_data_end_global_index;
        aux_data_end = module->aux_data_end;
        aux_heap_base_global_index = module->aux_heap_base_global_index;
        aux_heap_base = module->aux_heap_base;
        aux_stack_top_global_index = module->aux_stack_top_global_index;
        aux_stack_bottom = module->aux_stack_bottom;
        aux_stack_size = module->aux_stack_size;
    }
    dump(&global_table_data, env->global_table_data.memory_instances);
    fprintf(stderr, "\nDumped global data size: %d\n", env->table_count);
}

void WAMRModuleInstance::restore_impl(WASMModuleInstance *env) {
    if (!wamr->tmp_buf) {
    // auto m_ = (WASMMemoryInstance **)malloc(env->memory_count * sizeof(WASMMemoryInstance *));
    // wamr->tmp_buf = m_;
    // wamr->tmp_buf_size = env->memory_count;
    env->memory_count = memories.size();
    // for (int i = 0; i < env->memory_count; i++) {
    //     m_[i] = (WASMMemoryInstance *)malloc(sizeof(WASMMemoryInstance));
    // }
    for (int i = 0; i < env->memory_count; i++) {
        restore(&memories[i], env->memories[i]);
        // restore(&memories[i], m_[i]);
    }
    wamr->tmp_buf = env->memories;
    // env->memories = m_;
    wamr->tmp_buf_size = env->memory_count;

    env->global_data_size = global_data.size();
    // restore(&global_table_data, env->global_table_data.memory_instances);

    } else {
        env->memory_count = wamr->tmp_buf_size;
        env->memories = wamr->tmp_buf;
    }
    env->global_table_data.memory_instances[0] = **env->memories;
    env->global_data = (uint8 *)malloc(env->global_data_size);
    fprintf(stderr, "\nRestored global data ptr: %p\n", env->global_data);
    memcpy(env->global_data, global_data.data(), global_data.size());

    LOGV(DEBUG) << env->global_data_size;
    for (int i = 0; i < env->global_data_size; i++) {
        fprintf(stderr, "%d", env->global_data[i]);
    }
    if (wamr->is_aot) {
        auto module = (AOTModule *)env->module;
        module->aux_data_end_global_index = aux_data_end_global_index;
        module->aux_data_end = aux_data_end;
        module->aux_heap_base_global_index = aux_heap_base_global_index;
        module->aux_heap_base = aux_heap_base;
        module->aux_stack_top_global_index = aux_stack_top_global_index;
        module->aux_stack_bottom = aux_stack_bottom;
        module->aux_stack_size = aux_stack_size;
    } else {
        auto module = env->module;
        module->aux_data_end_global_index = aux_data_end_global_index;
        module->aux_data_end = aux_data_end;
        module->aux_heap_base_global_index = aux_heap_base_global_index;
        module->aux_heap_base = aux_heap_base;
        module->aux_stack_top_global_index = aux_stack_top_global_index;
        module->aux_stack_bottom = aux_stack_bottom;
        module->aux_stack_size = aux_stack_size;
    }
    restore(&wasi_ctx, &env->module->wasi_args);
}
