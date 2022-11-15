#ifndef _SKIP_LIST_SERVER_H_
#define _SKIP_LIST_SERVER_H_

#include "skip_list.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <algorithm>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include "thread_pool.h"
#include <memory>
#include <functional>

const int CLIENT_MAX_NUM = 1024;

void sys_err(const char* str) {
    perror(str);
    exit(1);
}

class skip_list_server {
private:
    static std::unique_ptr<thread_pool> tp;
private:
    skip_list<std::string, std::string>* lists[CLIENT_MAX_NUM];
    struct client_info {
        char ip[16];
        int fd;
        int port;
        struct sockaddr_in addr;
        static const int BUF_SIZE = 4096;
        char buf[client_info::BUF_SIZE];

        bool is_left;
        char leftchars[256];
    } clients[CLIENT_MAX_NUM];
    int max_idx;

    static const std::string delimiter;
private:
    int server_port = 7700;
    int lfd;
    int epfd;
    struct epoll_event evs[CLIENT_MAX_NUM];
public:
    skip_list_server();
    void loop();
private:
    void init();    // initial listening fd and epfd
    void do_accept();
    void do_communicate(int fd);

    void epfd_add(int fd, int event);
    void disconnect(int idx);
    void solve(char* buf, int n, int idx);
    void insert(char* buf, int cnt, int idx);   // insert buf[i..j)
};

std::unique_ptr<thread_pool> skip_list_server::tp(new thread_pool(2));

const std::string skip_list_server::delimiter = ":";

void skip_list_server::insert(char* buf, int cnt, int idx) {
    std::string tmp(buf, cnt);
    int tmp_idx = tmp.find(delimiter);
    if (tmp_idx == std::string::npos) {
        std::cout << "Structure invalid : " << tmp << std::endl;
        return;
    }
    lists[idx]->insert_node(tmp.substr(0, tmp_idx), tmp.substr(tmp_idx + 1, tmp.size()));
}

void skip_list_server::solve(char* buf, int n, int idx) {
    int i = 0;
    int j = 0;
    if (clients[idx].is_left) {
        while (clients[idx].leftchars[i] != 0) i++;
        while (buf[j] != '\n') clients[idx].leftchars[i++] = buf[j++];
        insert(clients[idx].leftchars, i, idx);
    }

    for (; j < n; j++) {
        if (buf[j] == '\n') {
            insert(buf + i, j - i, idx);
            i = ++j;
        }
    }
    if (i != n) {
        clients[idx].is_left = 1;
        int k;
        for (k = 0; i < n; k++, i++)
            clients[idx].leftchars[k] = buf[i];
        clients[idx].leftchars[k] = 0; 
    }
}

void skip_list_server::disconnect(int idx) {
    fprintf(stdout, "disconnect with %s:%d\n", clients[idx].ip, clients[idx].port);
    int connfd = clients[idx].fd;
    clients[idx].fd = -1;
    epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);   //delete node from epfd rb-tree
    close(connfd);
    if (idx == max_idx)
        while (max_idx != -1 && clients[--max_idx].fd != -1)
            ;
    delete lists[idx];
    lists[idx] = nullptr;
}

void skip_list_server::do_communicate(int fd) {
    int i;
    for (i = 0; i < CLIENT_MAX_NUM; i++)
        if (clients[i].fd == fd) break;
    char* buf = clients[i].buf;
    int n = recv(fd, buf, 4096, 0);
    if (n == 0)
        disconnect(i);
    else if (n > 0) {
        // 思路,每个key-value对以'\n'结尾, 在client_info中再加两个成员, isleft和leftchars
        // isleft用来判断上次传输最后是否为一个key-value对, leftchars用来保存上次传输一半
        solve(buf, n, i);
    }
}

void skip_list_server::epfd_add(int fd, int event) {
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
        sys_err("epoll_ctl fail");
}

void skip_list_server::do_accept() {
    int i = 0;
    for (; i < CLIENT_MAX_NUM; i++)
        if (clients[i].fd == -1)
            break;
    if (i == CLIENT_MAX_NUM) {
        fprintf(stderr, "clients are full\n");
        exit(1);
    }
    socklen_t len = sizeof(struct sockaddr_in);
    clients[i].fd = accept(lfd, (struct sockaddr*)&clients[i].addr, &len);
    if (clients[i].fd == -1) {
        perror("accept error");
    }
    
    inet_ntop(AF_INET, &clients[i].addr.sin_addr.s_addr, clients[i].ip, 16);
    clients[i].port = ntohs(clients[i].addr.sin_port);
    printf("connect with %s:%d\n", clients[i].ip, clients[i].port);

    max_idx = std::max(max_idx, i);

    epfd_add(clients[i].fd, EPOLLIN);

    // deal with fast_list
    std::string filename = "./server_dump/dump_";
    filename += std::to_string(clients[i].port);
    struct stat sbuf;

    if (stat(filename.c_str(), &sbuf) == -1)
        lists[i] = new skip_list<std::string, std::string>();
    else
        lists[i] = new skip_list<std::string, std::string>(filename.c_str());
    lists[i]->set_dump_filepath(filename.c_str());
}

void skip_list_server::loop() {
    while (true) {
        int num = epoll_wait(epfd, evs, CLIENT_MAX_NUM, -1);
        for (int i = 0; i < num; i++) {
            if (evs[i].data.fd == lfd) {
                // do_accept();
                tp->add_job(std::function<void()>(std::bind(&skip_list_server::do_accept, this)));
            } else {
                // do_communicate(evs[i].data.fd);
                int tmp = evs[i].data.fd;
                tp->add_job(std::function<void()>(std::bind(&skip_list_server::do_communicate, this, tmp)));
            }
            this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
}

skip_list_server::skip_list_server() {
    init();
    max_idx = -1;
    for (int i = 0; i < CLIENT_MAX_NUM; i++) {
        clients[i].fd = -1;
        lists[i] = nullptr;
        clients[i].is_left = false;
    }
}

void skip_list_server::init() {
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
        sys_err("socket error");

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        sys_err("bind error");
    if (listen(lfd, CLIENT_MAX_NUM) == -1)
        sys_err("listen error");
    
    epfd = epoll_create(CLIENT_MAX_NUM);
    
    epfd_add(lfd, EPOLLIN);
}

#endif