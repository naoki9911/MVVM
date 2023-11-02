//
// Created by victoryang00 on 5/2/23.
//
#include "wamr_wasi_context.h"
#include "wamr.h"
#include <string>
extern WAMRInstance *wamr;
void WAMRWASIContext::dump_impl(WASIContext *env) {
    for (int i = 0; i < wamr->dir_.size(); i++) {
        dir.emplace_back(wamr->dir_[i]);
    }
    for (int i = 0; i < wamr->map_dir_.size(); i++) {
        map_dir.emplace_back(wamr->map_dir_[i]);
    }
    this->exit_code = env->exit_code;
    for (auto [fd, res] : wamr->fd_map_) {
        auto [path, flags, offset] = res;
        auto dumped_res = std::make_tuple(path, flags, offset);
        this->fd_map[fd] = dumped_res;
    }

    for (auto [fd, socketMetaData] : wamr->socket_fd_map_) {
        SocketMetaData socketMetaDataCopy = socketMetaData;
        this->socket_fd_map[fd] = socketMetaDataCopy;
    }
}
void WAMRWASIContext::restore_impl(WASIContext *env) {
    int r;
    // std::string stdin_fd= "/dev/stdin";
    // wamr->invoke_fopen(stdin_fd, 0);
    // std::string stdout_fd= "/dev/stdout";
    // wamr->invoke_fopen(stdout_fd, 0);
    // std::string stderr_fd= "/dev/stderr";
    // wamr->invoke_fopen(stderr_fd, 0);
    for (auto [fd, res] : this->fd_map) {
        // differ from path from file
        LOGV(INFO) << "fd: " << fd << " path: " << std::get<0>(res) << " flags: " << std::get<1>(res)
                   << " offset: " << std::get<2>(res);
        r = wamr->invoke_fopen(std::get<0>(res), std::get<1>(res));
        if (r != fd)
            wamr->invoke_frenumber(r, fd);
        wamr->invoke_fseek(fd, std::get<2>(res));
    }

    for (auto [fd, socketMetaData] : this->socket_fd_map) {
        LOGV(INFO) << "fd: " << fd << " SocketMetaData[domain]: " << socketMetaData.domain
                //    << " SocketMetaData[socketAddress]: " << socketMetaData.socketAddress
                   << " SocketMetaData[protocol]: " << socketMetaData.protocol
                   << " SocketMetaData[type]: " << socketMetaData.type;
        uint32 tmp_sock_fd = socketMetaData.socketOpenData.sockfd;
        wamr->invoke_sock_open(socketMetaData.socketOpenData.poolfd, socketMetaData.socketOpenData.af,
                               socketMetaData.socketOpenData.socktype, &tmp_sock_fd);
        // renumber or not?
        if (tmp_sock_fd != fd)
            wamr->invoke_frenumber(tmp_sock_fd, fd);
        // if socket不一样 renumber
        auto * tmp_addr = (__wasi_addr_t*)malloc(sizeof(__wasi_addr_t));
        tmp_addr->kind = socketMetaData.socketSentToData.dest_addr.ip.is_4?IPv4:IPv6;
        if(tmp_addr->kind == IPv4) {
            tmp_addr->addr.ip4.addr.n0 = socketMetaData.socketSentToData.dest_addr.ip.ip4[0];
            tmp_addr->addr.ip4.addr.n1 = socketMetaData.socketSentToData.dest_addr.ip.ip4[1];
            tmp_addr->addr.ip4.addr.n2 = socketMetaData.socketSentToData.dest_addr.ip.ip4[2];
            tmp_addr->addr.ip4.addr.n3 = socketMetaData.socketSentToData.dest_addr.ip.ip4[3];

            tmp_addr->addr.ip4.port = socketMetaData.socketSentToData.dest_addr.port;
        } else {
            tmp_addr->addr.ip6.addr.n0 = socketMetaData.socketSentToData.dest_addr.ip.ip6[0];
            tmp_addr->addr.ip6.addr.n1 = socketMetaData.socketSentToData.dest_addr.ip.ip6[1];
            tmp_addr->addr.ip6.addr.n2 = socketMetaData.socketSentToData.dest_addr.ip.ip6[2];
            tmp_addr->addr.ip6.addr.n3 = socketMetaData.socketSentToData.dest_addr.ip.ip6[3];
            tmp_addr->addr.ip6.addr.h0 = socketMetaData.socketSentToData.dest_addr.ip.ip6[4];
            tmp_addr->addr.ip6.addr.h1 = socketMetaData.socketSentToData.dest_addr.ip.ip6[5];
            tmp_addr->addr.ip6.addr.h2 = socketMetaData.socketSentToData.dest_addr.ip.ip6[6];
            tmp_addr->addr.ip6.addr.h3 = socketMetaData.socketSentToData.dest_addr.ip.ip6[7];

            tmp_addr->addr.ip6.port = socketMetaData.socketSentToData.dest_addr.port;
        }

        uint32_t tmp_so_len = 0;
         wamr->invoke_sock_sendto(socketMetaData.socketSentToData.sock, &socketMetaData.socketSentToData.si_data,
                                  socketMetaData.socketSentToData.si_data_len, socketMetaData.socketSentToData.si_flags,
                                  tmp_addr,//翻译
                                 &tmp_so_len );



         //----------------------------------------------------------------------------------------------------------------

         auto * tmp_addr_rev = (__wasi_addr_t*)malloc(sizeof(__wasi_addr_t));
         tmp_addr_rev->kind = socketMetaData.socketRecvFromData.src_addr.ip.is_4?IPv4:IPv6;
         if(tmp_addr_rev->kind == IPv4) {
            tmp_addr_rev->addr.ip4.addr.n0 = socketMetaData.socketRecvFromData.src_addr.ip.ip4[0];
            tmp_addr_rev->addr.ip4.addr.n1 = socketMetaData.socketRecvFromData.src_addr.ip.ip4[1];
            tmp_addr_rev->addr.ip4.addr.n2 = socketMetaData.socketRecvFromData.src_addr.ip.ip4[2];
            tmp_addr_rev->addr.ip4.addr.n3 = socketMetaData.socketRecvFromData.src_addr.ip.ip4[3];

            tmp_addr_rev->addr.ip4.port = socketMetaData.socketRecvFromData.src_addr.port;
         } else {
            tmp_addr_rev->addr.ip6.addr.n0 = socketMetaData.socketRecvFromData.src_addr.ip.ip6[0];
            tmp_addr_rev->addr.ip6.addr.n1 = socketMetaData.socketRecvFromData.src_addr.ip.ip6[1];
            tmp_addr_rev->addr.ip6.addr.n2 = socketMetaData.socketRecvFromData.src_addr.ip.ip6[2];
            tmp_addr_rev->addr.ip6.addr.n3 = socketMetaData.socketRecvFromData.src_addr.ip.ip6[3];
            tmp_addr_rev->addr.ip6.addr.h0 = socketMetaData.socketRecvFromData.src_addr.ip.ip6[4];
            tmp_addr_rev->addr.ip6.addr.h1 = socketMetaData.socketRecvFromData.src_addr.ip.ip6[5];
            tmp_addr_rev->addr.ip6.addr.h2 = socketMetaData.socketRecvFromData.src_addr.ip.ip6[6];
            tmp_addr_rev->addr.ip6.addr.h3 = socketMetaData.socketRecvFromData.src_addr.ip.ip6[7];

            tmp_addr_rev->addr.ip6.port = socketMetaData.socketRecvFromData.src_addr.port;
         }

         uint32_t tmp_ro_data_len = 0;
         wamr->invoke_sock_recvfrom(socketMetaData.socketRecvFromData.sock, &socketMetaData.socketRecvFromData.ri_data,
                                    socketMetaData.socketRecvFromData.ri_data_len, socketMetaData.socketRecvFromData.ri_flags,
                                    tmp_addr_rev,
                                    &tmp_ro_data_len);
    }
};