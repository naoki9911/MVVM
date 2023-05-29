//
// Created by yiwei yang on 5/2/23.
//

#ifndef MVVM_WAMR_WASI_CONTEXT_H
#define MVVM_WAMR_WASI_CONTEXT_H

#include "wamr_serializer.h"
#include "wasm_runtime.h"
#include <memory>
#include <vector>
#include <map>
// TODO: add more context
struct WAMRFDObjectEntry {
    __wasi_filetype_t type;
    int number;
    __wasi_rights_t rights_base;
    __wasi_rights_t rights_inheriting;
};
struct WAMRFDTable {
    std::vector<WAMRFDObjectEntry> entries;
    size_t size;
    size_t used;
};

struct WAMRFDPrestat {
    std::string dir{}; // the path link prestats for dir need to load target directory
};

struct WAMRPreStats {
    std::vector<WAMRFDPrestat> prestats;
    size_t size{};
    size_t used{};
};

struct WAMRArgvEnvironValues {
    std::string argv_buf{};
    std::vector<std::string> argv_list;
    std::vector<std::string> env_list;
};
// need to serialize the opened file in the file descripter and the socket opened also.

struct WAMRAddrPool {
    uint16 ip46[8];
    __wasi_addr_type_t type;
    uint8 mask;
};
// TODO: add more context
struct WAMRWASIContext {
    std::vector<WAMRFDTable> curfds;
    std::map<uint32, std::string> fd_map;
    std::vector<WAMRPreStats> prestats;
    std::unique_ptr<WAMRArgvEnvironValues> argv_environ;
    std::vector<WAMRAddrPool> addr_pool;
    std::vector<std::string> ns_lookup_list;
    uint32_t exit_code;
    void dump(WASIContext *env){

    };
    void restore(WASIContext *env){
        // Need to open the file and reinitialize the file descripter by map.
    };
};

template <SerializerTrait<WASIContext *> T> void dump(T t, WASIContext *env) { t->dump(env); }
template <SerializerTrait<WASIContext *> T> void restore(T t, WASIContext *env) { t->restore(env); }

#endif // MVVM_WAMR_WASI_CONTEXT_H
