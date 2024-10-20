#include <array>
#include <string>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>


static void msg(const std::string &msg) {
    std::cerr << msg << std::endl;
}

static void die(const std::string &msg) {
    throw std::runtime_error(msg + ": " + std::strerror(errno));
}

static void do_something(int connfd) {
    std::array<char, 64> rbuf = {};
    ssize_t n = read(connfd, rbuf.data(), rbuf.size() - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    std::cout << "client says: " << rbuf.data() << std::endl;

    std::string response = "world";
    write(connfd, response.c_str(), response.size());
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // this is needed for most server applications
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0


    if (bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0) {
        die("bind()");
    }

    // listen
    if (listen(fd, SOMAXCONN) < 0) {
        die("listen()");
    }

    while (true) {
        // accept
        sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, reinterpret_cast<sockaddr*>(&client_addr), &socklen);
        if (connfd < 0) {
            continue;   // error
        }

        do_something(connfd);
        close(connfd);
    }

    close(fd);
    return 0;
}
