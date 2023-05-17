//
// Created by victoryang00 on 4/8/23.
//

#include "wamr.h"

#include <csignal>
#include <cstdio>
auto wamr = new WAMRInstance("test.wasm");
auto writer = fwrite_stream("test.bin");

void serialize_to_file(WASMExecEnv *instance) {
    std::vector<WAMRExecEnv> as;
    WAMRExecEnv a;
    auto curr_instance = instance;
    while (!curr_instance) {
        dump(&a, curr_instance);
        as.push_back(a);
        curr_instance = curr_instance->next;
    }

    struct_pack::serialize_to(writer, a);
    exit(0);
}

#ifndef MVVM_DEBUG
void sigtrap_handler(int sig) {
    printf("Caught signal %d, performing custom logic...\n", sig);

    // You can exit the program here, if desired
    std::vector<WAMRExecEnv> a;
    dump(&a, wamr->get_exec_env());
    struct_pack::serialize_to(writer, a);
    exit(0);
}

// Signal handler function for SIGINT
void sigint_handler(int sig) {
    // Your logic here
    printf("Caught signal %d, performing custom logic...\n", sig);
    wamr->get_exec_env()->is_checkpoint = true;
    struct sigaction sa {};

    // Clear the structure
    sigemptyset(&sa.sa_mask);

    // Set the signal handler function
    sa.sa_handler = sigtrap_handler;

    // Set the flags
    sa.sa_flags = SA_RESTART;

    // Register the signal handler for SIGTRAP
    if (sigaction(SIGTRAP, &sa, nullptr) == -1) {
        perror("Error: cannot handle SIGTRAP");
        exit(-1);
    }
}
#endif
int main() {
#ifndef MVVM_DEBUG
    // Define the sigaction structure
    struct sigaction sa {};

    // Clear the structure
    sigemptyset(&sa.sa_mask);

    // Set the signal handler function
    sa.sa_handler = sigint_handler;

    // Set the flags
    sa.sa_flags = SA_RESTART;

    // Register the signal handler for SIGINT
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        perror("Error: cannot handle SIGINT");
        return 1;
    }
#endif
    // Main program loop
    wamr->invoke_main();
    return 0;
}