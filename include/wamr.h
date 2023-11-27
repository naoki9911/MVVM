//
// Created by victoryang00 on 5/6/23.
//

#ifndef MVVM_WAMR_H
#define MVVM_WAMR_H

#if WASM_ENABLE_AOT != 0
#include "aot_runtime.h"
#endif
#include "bh_read_file.h"
#include "logging.h"
#include "wamr_exec_env.h"
#include "wamr_export.h"
#include "wamr_read_write.h"
#include "wamr_wasi_context.h"
#include "wasm_runtime.h"
#include <ranges>
#include <tuple>

class WAMRInstance {
    WASMExecEnv *exec_env{};
    WASMExecEnv *cur_env{};
    WASMModuleInstanceCommon *module_inst{};
    WASMModuleCommon *module;
    WASMFunctionInstanceCommon *func{};

public:
    std::string aot_file_path;
    std::string wasm_file_path;
    std::vector<std::size_t> int3_addr;
    std::vector<const char *> dir_;
    std::vector<const char *> map_dir_;
    std::vector<const char *> env_;
    std::vector<const char *> arg_;
    std::vector<const char *> addr_;
    std::vector<const char *> ns_pool_;
    std::map<int, std::tuple<std::string, std::vector<std::tuple<int, int, fd_op>>>> fd_map_;
    // add offset to pair->tuple, 3rd param 'int'
    std::map<int, SocketMetaData> socket_fd_map_;

    bool is_jit;
    bool is_aot{};
    char error_buf[128]{};
    uint32 buf_size{}, stack_size = 32768, heap_size = 32768;
    typedef struct ThreadArgs {
        wasm_exec_env_t exec_env;
    } ThreadArgs;
    std::vector<ThreadArgs> thread_arg;
    std::vector<wasm_thread_t> tid;
    void (*thread_callback)(wasm_exec_env_t, void *){};

    explicit WAMRInstance(const char *wasm_path, bool is_jit);

    void instantiate();
    void recover(std::vector<std::unique_ptr<WAMRExecEnv>> *execEnv);
    bool load_wasm_binary(const char *wasm_path, char **buffer_ptr);
    bool get_int3_addr();
    bool replace_int3_with_nop();
    bool replace_nop_with_int3();
    WASMFunction *get_func();
    void set_func(WASMFunction *);
#if WASM_ENABLE_AOT != 0
    std::vector<uint32> get_args();
    AOTFunctionInstance *get_func(int index);
#endif
    WASMExecEnv *get_exec_env();
    WASMModuleInstance *get_module_instance();

#if WASM_ENABLE_AOT != 0
    AOTModule *get_module();
#endif

    void set_wasi_args(WAMRWASIContext &addrs);
    void set_wasi_args(const std::vector<std::string> &dir_list, const std::vector<std::string> &map_dir_list,
                       const std::vector<std::string> &env_list, const std::vector<std::string> &arg_list,
                       const std::vector<std::string> &addr_list, const std::vector<std::string> &ns_lookup_pool);

    int invoke_main();
    void invoke_init_c();
    int invoke_fopen(std::string &path, uint32 option);
    int invoke_frenumber(uint32 fd, uint32 to);
    int invoke_fseek(uint32 fd, uint32 offset);
    int invoke_ftell(uint32 fd, uint32 offset, uint32 whench);
    int invoke_preopen(uint32 fd, const std::string &path);
#if !defined(__WINCRYPT_H__)
    int invoke_sock_sendto(uint32_t sock, const iovec_app_t *si_data, uint32 si_data_len, uint16_t si_flags,
                           const __wasi_addr_t *dest_addr, uint32 *so_data_len);
    int invoke_sock_recvfrom(uint32_t sock, iovec_app_t *ri_data, uint32 ri_data_len, uint16_t ri_flags,
                             __wasi_addr_t *src_addr, uint32 *ro_data_len);
#endif
    int invoke_sock_open(uint32_t poolfd, int af, int socktype, uint32_t *sockfd);
    ~WAMRInstance();
};
#endif // MVVM_WAMR_H
