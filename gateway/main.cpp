#include "logging.h"
#include <chrono>
#include <crafter.h>
#include <cstddef>
#include <cstdlib>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <pcap/pcap.h>
#include <thread>

using namespace Crafter;

std::string client_ip;
std::string server_ip;
pcap_t *handle;
int linkhdrlen;
int packets;
int client_fd;
int fd;
std::jthread pcap_thread;
std::jthread keep_alive_thread;
bool is_forward = false;
bool is_tcp = false;

void packet_handler(u_char *user, const struct pcap_pkthdr *header, const u_char *packetptr) {
    // Analyze packet
    struct ip *iphdr;
    struct icmp *icmphdr;
    struct tcphdr *tcphdr;
    struct udphdr *udphdr;
    char iphdrInfo[256];
    char srcip[256];
    char dstip[256];

    if (!is_forward) { // Skip the datalink layer header and get the IP header fields.
        packetptr += linkhdrlen;
        iphdr = (struct ip *)packetptr;
        strcpy(srcip, inet_ntoa(iphdr->ip_src));
        strcpy(dstip, inet_ntoa(iphdr->ip_dst));
        sprintf(iphdrInfo, "ID:{} TOS:0x%x, TTL:{} IpLen:{} DgLen:{}", ntohs(iphdr->ip_id), iphdr->ip_tos,
                iphdr->ip_ttl, 4 * iphdr->ip_hl, ntohs(iphdr->ip_len));

        // Advance to the transport layer header then parse and display
        // the fields based on the type of hearder: tcp, udp or icmp.
        packetptr += 4 * iphdr->ip_hl;
        switch (iphdr->ip_p) {
        case IPPROTO_TCP:
            tcphdr = (struct tcphdr *)packetptr;
            LOGV(INFO) << fmt::format("TCP  {}:{} -> {}:{}", srcip, ntohs(tcphdr->th_sport), dstip,
                                      ntohs(tcphdr->th_dport));
            LOGV(INFO) << fmt::format("{}", iphdrInfo);
            LOGV(INFO) << fmt::format("%c%c%c%c%c%c Seq: 0x%x Ack: 0x%x Win: 0x%x TcpLen: {}",
                                      (tcphdr->th_flags & TH_URG ? 'U' : '*'), (tcphdr->th_flags & TH_ACK ? 'A' : '*'),
                                      (tcphdr->th_flags & TH_PUSH ? 'P' : '*'), (tcphdr->th_flags & TH_RST ? 'R' : '*'),
                                      (tcphdr->th_flags & TH_SYN ? 'S' : '*'), (tcphdr->th_flags & TH_SYN ? 'F' : '*'),
                                      ntohl(tcphdr->th_seq), ntohl(tcphdr->th_ack), ntohs(tcphdr->th_win),
                                      4 * tcphdr->th_off);
            LOGV(INFO) << fmt::format("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
            packets += 1;
            break;

        case IPPROTO_UDP:
            udphdr = (struct udphdr *)packetptr;
            LOGV(INFO) << fmt::format("UDP  {}:{} -> {}:{}", srcip, ntohs(udphdr->uh_sport), dstip,
                                      ntohs(udphdr->uh_dport));
            LOGV(INFO) << fmt::format("{}", iphdrInfo);
            LOGV(INFO) << fmt::format("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
            packets += 1;
            break;

        case IPPROTO_ICMP:
            icmphdr = (struct icmp *)packetptr;
            LOGV(INFO) << fmt::format("ICMP {} -> {}", srcip, dstip);
            LOGV(INFO) << fmt::format("{}", iphdrInfo);
            LOGV(INFO) << fmt::format("Type:{} Code:{} ID:{} Seq:{}", icmphdr->icmp_type, icmphdr->icmp_code,
                                      ntohs(icmphdr->icmp_hun.ih_idseq.icd_id),
                                      ntohs(icmphdr->icmp_hun.ih_idseq.icd_seq));
            LOGV(INFO) << fmt::format("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
            packets += 1;
            break;
        }
    }
    // Modify packet if needed
}

void keep_alive(std::string source_ip, int source_port, std::string dest_ip, int dest_port) {
    // Send keep alive message to socket
    // Initialize Libcrafter
    InitCrafter();

    while (true) {
        // Create an IP layer
        IP ip_layer;
        ip_layer.SetSourceIP(source_ip);
        ip_layer.SetDestinationIP(dest_ip);

        // Create a TCP layer
        TCP tcp_layer;
        tcp_layer.SetSrcPort(source_port);
        tcp_layer.SetDstPort(dest_port);
        tcp_layer.SetFlags(TCP::ACK);

        // Craft the packet
        Packet packet = ip_layer / tcp_layer;

        // Send the packet
        packet.Send();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // Clean up Libcrafter
    CleanCrafter();
}
void sigterm_handler(int sig) {
    struct pcap_stat stats {};

    if (pcap_stats(handle, &stats) >= 0) {
        LOGV(INFO) << fmt::format("{} packets captured", packets);
        LOGV(INFO) << fmt::format("{} packets received by filter", stats.ps_recv);
        LOGV(INFO) << fmt::format("{} packets dropped", stats.ps_drop);
    }
    pcap_close(handle);
    close(client_fd);
    close(fd);
    pcap_thread.join();
    keep_alive_thread.join();
    LOGV(INFO) << "Bye";
    exit(0);
}

int main() {
    struct sockaddr_in address {};
    int opt = 1;
    int rc;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char errbuf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp {};
    char filter_exp[] = ""; // The filter expression
    // char filter_exp[] = "net 172.17.0.0/24";
    bpf_u_int32 netmask;

    struct mvvm_op_data op_data {};

    signal(SIGTERM, sigterm_handler);
    signal(SIGQUIT, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    fd = socket(AF_INET, SOCK_STREAM, 0); // Create a socket
    // ... code to set up the socket address structure and bind the socket ...

    // Forcefully attaching socket to the port
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        LOGV(ERROR) << "setsockopt";
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(MVVM_SOCK_PORT);
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, MVVM_SOCK_ADDR, &address.sin_addr) <= 0) {
        LOGV(ERROR) << "Invalid address/ Address not supported";
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the network address and port
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        LOGV(ERROR) << "bind failed";
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(fd, 3) < 0) {
        LOGV(ERROR) << "listen";
        exit(EXIT_FAILURE);
    }

    handle = pcap_open_live(MVVM_SOCK_INTERFACE, BUFSIZ, 1, 1000, errbuf);
    if (handle == nullptr) {
        fprintf(stderr, "pcap_open_live(): {}", errbuf);
        exit(EXIT_FAILURE);
    }

    // Compile and apply the filter
    if (pcap_compile(handle, &fp, filter_exp, 1, PCAP_NETMASK_UNKNOWN) == -1) {
        LOGV(ERROR) << fmt::format("Couldn't parse filter {}: {}", filter_exp, pcap_geterr(handle));
        exit(-1);
    }

    if (pcap_setfilter(handle, &fp) == -1) {
        LOGV(ERROR) << fmt::format("Couldn't install filter {}:", filter_exp, pcap_geterr(handle));
        exit(-1);
    }

    // Capture packets
    pcap_thread = std::jthread{[&] { pcap_loop(handle, -1, packet_handler, nullptr); }};

    while (true) { // Open the device for sniffing
        client_fd = accept(fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client_fd < 0) {
            LOGV(ERROR) << "accept";
            exit(EXIT_FAILURE);
        }
        // offload info from client
        if ((rc = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
            memcpy(&op_data, buffer, sizeof(op_data));
            if (op_data.src_addr.is_4) {
                server_ip = fmt::format("{}.{}.{}.{}", op_data.src_addr.ip4[0], op_data.src_addr.ip4[1],
                                        op_data.src_addr.ip4[2], op_data.src_addr.ip4[3]);
                client_ip = fmt::format("{}.{}.{}.{}", op_data.dest_addr.ip4[0], op_data.dest_addr.ip4[1],
                                        op_data.dest_addr.ip4[2], op_data.dest_addr.ip4[3]);
            } else {
                server_ip = fmt::format("{:04x}:{:04x}:{:04x}:{:04x}:{:04x}:{:04x}:{:04x}:{:04x}",
                                        op_data.src_addr.ip6[0], op_data.src_addr.ip6[1], op_data.src_addr.ip6[2],
                                        op_data.src_addr.ip6[3], op_data.src_addr.ip6[4], op_data.src_addr.ip6[5],
                                        op_data.src_addr.ip6[6], op_data.src_addr.ip6[7]);
                client_ip = fmt::format("{:04x}:{:04x}:{:04x}:{:04x}:{:04x}:{:04x}:{:04x}:{:04x}",
                                        op_data.dest_addr.ip6[0], op_data.dest_addr.ip6[1], op_data.dest_addr.ip6[2],
                                        op_data.dest_addr.ip6[3], op_data.dest_addr.ip6[4], op_data.dest_addr.ip6[5],
                                        op_data.dest_addr.ip6[6], op_data.dest_addr.ip6[7]);
            }
            LOGV(INFO) << "server_ip:" << server_ip << " client_ip:" << client_ip; // 且有输入了
            switch (op_data.op) {
            case MVVM_SOCK_SUSPEND:
                // suspend
                LOGV(ERROR) << "suspend";

                if (is_tcp)
                    keep_alive_thread - std::jthread{[&] { keep_alive(server_ip, op_data.src_addr.port, client_ip, op_data.dest_addr.port); }};
                break;
            case MVVM_SOCK_RESUME:
                // resume
                LOGV(ERROR) << "resume";
                if (is_tcp)
                    keep_alive_thread.join();
                is_forward = true;
                // for udp forward from source to remote
                // drop to new ip
                // stop keep_alive
                break;
            }
        }
    }
}