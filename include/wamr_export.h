//
// Created by victoryang00 on 6/17/23.
//
#include "wasm_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MVVM_WASI)
#define MVVM_WASI
struct SocketAddrPool {
    uint16 ip4[4];
    uint16 ip6[8];
    bool is_4;
    uint16 port;
};
enum fd_op {
    FOPEN = 0,
    FWRITE = 1,
    FREAD = 2,
    FSEEK = 4,
    FTELL = 5,
};
#if !defined(__WINCRYPT_H__)
typedef struct iovec_app {
    uint32 buf_offset;
    uint32 buf_len;
} iovec_app_t;
#endif
void serialize_to_file(struct WASMExecEnv *);
#endif

#if !defined(__WINCRYPT_H__)
void insert_sock_send_to_data(uint32_t sock, const iovec_app_t *si_data, uint32 si_data_len, uint16_t si_flags,
                              const __wasi_addr_t *dest_addr, uint32 *so_data_len);
void insert_sock_open_data(uint32_t, int, int, uint32_t);
void insert_sock_recv_from_data(uint32_t sock, iovec_app_t *ri_data, uint32 ri_data_len, uint16_t ri_flags,
                                __wasi_addr_t *src_addr, uint32 *ro_data_len);
#endif
void insert_fd(int, const char *, int, int, enum fd_op op);
void remove_fd(int);
void rename_fd(int, char const *, int, char const *);
void insert_socket(int, int, int, int);
void update_socket_fd_address(int, struct SocketAddrPool *);
void insert_lock(char const *, int);
void insert_sem(char const *, int);
void remove_lock(char const *);
void remove_sem(char const *);
void wasm_thread_restart();

void restart_execution(uint32 targs);
extern int pthread_create_wrapper(wasm_exec_env_t exec_env, uint32 *thread, /* thread_handle */
                                  const void *attr, /* not supported */
                                  uint32 elem_index, /* entry function */
                                  uint32 arg) /* arguments buffer */
    ;
#ifdef __cplusplus
}
#endif
