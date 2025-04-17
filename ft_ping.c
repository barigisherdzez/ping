#include "./ft_ping.h"

int keep_pinging = 1;

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }

    if (len == 1) {
        sum += *(unsigned char *)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    result = ~sum;
    return result;
}

void print_help() {
    printf("Usage: ft_ping [options] <IP Address/Hostname>\n");
    printf("Options:\n");
    printf("  -v     Verbose output\n");
    printf("  -?     Display this help message\n");
}

void sigint_handler(int sig) {
    (void)sig;
    keep_pinging = 0;
    printf("\nPing interrupted, exiting...\n");
}

void ping(const char *hostname, int verbose) {
    struct addrinfo hints, *res;
    int sockfd;
    struct icmp icmp_pkt;
    struct timeval start_time, end_time;
    int ttl = 64;
    int seq = 0;
    int sent = 0, received = 0;
    double total_rtt = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    int err = getaddrinfo(hostname, NULL, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "Error resolving host: %s\n", gai_strerror(err));
        exit(1);
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("Socket creation failed");
        if (errno == EPERM)
            fprintf(stderr, "You must run this program as root (sudo).\n");
        freeaddrinfo(res);
        exit(1);
    }

    if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("Setsockopt failed");
        freeaddrinfo(res);
        close(sockfd);
        exit(1);
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, sizeof(ip_str));
    printf("PING %s (%s): %d data bytes\n", hostname, ip_str, (int)sizeof(icmp_pkt));

    while (keep_pinging) {
        memset(&icmp_pkt, 0, sizeof(icmp_pkt));
        icmp_pkt.icmp_type = ICMP_ECHO;
        icmp_pkt.icmp_code = 0;
        icmp_pkt.icmp_id = getpid();
        icmp_pkt.icmp_seq = seq++;
        icmp_pkt.icmp_cksum = checksum(&icmp_pkt, sizeof(icmp_pkt));

        gettimeofday(&start_time, NULL);
        if (sendto(sockfd, &icmp_pkt, sizeof(icmp_pkt), 0, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
            perror("Send failed");
            break;
        }

        sent++;

        char buffer[MAX_PKT_SIZE];
        socklen_t len = sizeof(*addr);
        fd_set read_fds;
        struct timeval timeout;

        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ret;
        while ((ret = select(sockfd + 1, &read_fds, NULL, NULL, &timeout)) == -1 && errno == EINTR) {}

        if (ret <= 0) {
            if (ret == 0) {
                printf("Request timeout for icmp_seq %d\n", seq - 1);
            } else {
                perror("Select failed");
            }
            continue;
        }

        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)addr, &len);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }

        struct ip *ip_header = (struct ip *)buffer;
        struct icmp *icmp_reply = (struct icmp *)(buffer + (ip_header->ip_hl << 2));

        if (icmp_reply->icmp_type == ICMP_ECHOREPLY && icmp_reply->icmp_id == getpid()) {
            gettimeofday(&end_time, NULL);
            double rtt = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + (end_time.tv_usec - start_time.tv_usec) / 1000.0;
            total_rtt += rtt;
            received++;

            if (verbose) {
                printf("Ping response from %s: icmp_seq=%d ttl=%d time=%.1f ms\n", ip_str, icmp_reply->icmp_seq, ip_header->ip_ttl, rtt);
            } else {
                printf("Ping response from %s: %.1f ms\n", ip_str, rtt);
            }
        } else if (icmp_reply->icmp_type == ICMP_UNREACH) {
            printf("Destination unreachable (icmp_seq %d)\n", seq - 1);
        }

        sleep(PING_INTERVAL);
    }

    printf("\n--- %s ping statistics ---\n", hostname);
    printf("%d packets transmitted, %d received, ", sent, received);

    if (sent > 0) {
        printf("%d%% packet loss, ", (sent - received) * 100 / sent);
        if (received > 0) {
            printf("time %.0fms\n", (float)total_rtt / received);
        } else {
            printf("time N/A\n");
        }
    } else {
        printf("No packets transmitted\n");
    }

    freeaddrinfo(res);
    close(sockfd);
}

int main(int argc, char *argv[]) {
    int verbose = 0;
    int opt;

    signal(SIGINT, sigint_handler);

    while ((opt = getopt(argc, argv, "v?")) != -1) {
        switch (opt) {
            case 'v':
                verbose = 1;
                break;
            case '?':
                print_help();
                return 0;
            default:
                print_help();
                return 1;
        }
    }

    if (optind >= argc) {
        print_help();
        return 1;
    }

    ping(argv[optind], verbose);

    return 0;
}
