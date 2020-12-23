#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <map>
#include "json.hpp"

#define PORT 8888
#define QUEUE 20
#define MAX_BUF_SIZE 1024
namespace ns {

struct photo_cmd {
    int dsID;
    int reqType;
    int dsData;
};
}  // namespace ns
class JsonPhaser {
public:
    JsonPhaser() {}
    ~JsonPhaser() {}
    int DealBorunteData(const char *recv_data) {
        /*
         * {
         *   "dsID" : "www.hc-system.com.cam" ,
         *   "reqType" : "photo" ,
         *   "camID" : 0
         *  }
         */
        int cmd_type = 0;
        try {
            auto j = Json11::json::parse(recv_data);
            std::cout << j << std::endl;
            auto dsID = j["dsID"].get<std::string>();
            auto reqType = j["reqType"].get<std::string>();
            if (reqType == "photo") {
                cmd_type = 1;
            }
            printf("reqType is %s\n", reqType.c_str());
        } catch (const std::exception &e) {
            printf("%s\n", e.what());
            return cmd_type;
        }
        return cmd_type;
    }
    int CreateSuccessMsg(int id, const std::vector<float> &pose) {
        /*
        {
            "dsID":"www.hc-system.com.cam",
            "reqType":"AddPoints",
            "dsData":[
                {
                    "camID":"0",
                    "data":[
                        {
                            "ModelID": "0",
                            "X" : "888.001",
                            "Y" : "1345.001",
                            "Z" : "1000.001",
                            "U" : "0.000",
                            "V" : "0.000",
                            "Angel" : "123.123",
                        },
                    ]
                }
            ]
        }
        */
        char buf[MAX_BUF_SIZE];
        Json11::json data;
        sprintf(buf, "%d", id);
        data["ModelID"] = buf;
        sprintf(buf, "%f", pose[0]);
        data["X"] = buf;
        sprintf(buf, "%f", pose[1]);
        data["Y"] = buf;
        sprintf(buf, "%f", pose[2]);
        data["Z"] = buf;
        sprintf(buf, "%f", pose[3]);
        data["U"] = buf;
        sprintf(buf, "%f", pose[4]);
        data["V"] = buf;
        sprintf(buf, "%f", pose[5]);
        data["Angel"] = buf;

        Json11::json dsData;
        dsData["camID"] = "0";
        dsData["data"] = Json11::json::array({data});

        Json11::json j;
        j["dsID"] = dsID;
        j["reqType"] = "AddPoints";
        j["dsData"] = Json11::json::array({dsData});
        str = j.dump();
        return true;
    }

    int CreatePhotoMsg(bool success) {
        /*
         * {
         *   "dsID" : "www.hc-system.com.cam" ,
         *   "reqType" : "photo" ,
         *   "camID" : 0 ,
         *   "ret" : 1
         *  }
         */

        Json11::json j;
        j["dsID"] = dsID;
        j["reqType"] = "AddPoints";
        j["camID"] = 0;
        j["ret"] = success;
        str = j.dump();
        return true;
    }

    std::string GetString() { return str; }

private:
    std::string str;
    const std::string dsID{"www.hc-system.com.cam"};
};

inline bool read_file_binary(const std::string &pathToFile, char *buf, unsigned int buf_size) {
    std::ifstream file(pathToFile, std::ios::binary);
    unsigned int read_buf_size = 0;

    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        size_t sizeBytes = file.tellg();
        file.seekg(0, std::ios::beg);
        read_buf_size = buf_size > sizeBytes ? sizeBytes : buf_size;
        if (file.read(buf, read_buf_size))
            return true;
    } else
        throw std::runtime_error("could not open binary ifstream to path " + pathToFile);
    return false;
}
#include <regex>
int Split(const std::string &str_in, std::vector<double> &tokens, const std::string &delim = "") {
    tokens.clear();
    std::regex re(delim);
    std::sregex_token_iterator first{str_in.begin(), str_in.end(), re, -1}, last;

    std::vector<std::string> result = {first, last};
    int nsize = static_cast<int>(result.size());
    tokens.resize(nsize);
    for (int i = 0; i < nsize; i++) {
        tokens[i] = std::stod(result[i]);
    }
    return nsize;
}
int main() {
    // std::ifstream f;
    // f.open("test.json");
    // if (f.is_open() == false) {
    //     printf("out\n");
    //     return -1;
    // }
    // f.read();
    // char buf[MAX_BUF_SIZE];
    // read_file_binary("test.json", buf, MAX_BUF_SIZE);
    // JsonPhaser j;
    // j.DealBorunteData(buf);
    // // printf("%s\n", buf);
    // j.CreateSuccessMsg(1, {1, 2, 3, 4, 5, 6});
    // printf("%s\n", j.GetString().c_str());
    // return 0;
    fd_set rfds;
    struct timeval tv;
    int retval, maxfd;
    int ss = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;

    if (setsockopt(ss, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf("Error: setsockopt SO_REUSEADDR | SO_REUSEPORT");
        return -2;
    }
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(PORT);
    // printf("%d\n",INADDR_ANY);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(ss, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }
    printf("bind port %d success.\n", PORT);
    if (listen(ss, QUEUE) == -1) {
        perror("listen");
        exit(1);
    }
    printf("start listen\n");

    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);
    ///成功返回非负描述字，出错返回-1
    int conn = accept(ss, (struct sockaddr *)&client_addr, &length);
    /*没有用来存储accpet返回的套接字的数组，所以只能实现server和单个client双向通信*/
    if (conn < 0) {
        perror("connect");
        exit(1);
    }

    /*把可读文件描述符的集合清空*/
    FD_ZERO(&rfds);
    printf("success accept from client\n");
    while (1) {
        /*把标准输入的文件描述符加入到集合中*/
        FD_SET(0, &rfds);
        maxfd = 0;
        /*把当前连接的文件描述符加入到集合中*/
        FD_SET(conn, &rfds);
        /*找出文件描述符集合中最大的文件描述符*/
        if (maxfd < conn)
            maxfd = conn;
        /*设置超时时间*/
        tv.tv_sec = 5;  //设置倒计时
        tv.tv_usec = 0;
        /*等待聊天*/
        retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (retval == -1) {
            printf("select出错，客户端程序退出\n");
            break;
        } else if (retval == 0) {
            printf("服务端没有任何输入信息，并且客户端也没有信息到来，waiting...\n");
            continue;
        } else {
            /*客户端发来了消息*/
            if (FD_ISSET(conn, &rfds)) {
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                int len = recv(conn, buffer, sizeof(buffer), 0);
                if (strcmp(buffer, "exit\n") == 0)
                    break;
                if (strlen(buffer) == 0) {
                    printf("exit\n");
                    break;
                }
                printf("aa%saa\n", buffer);
                // send(conn, buffer, len, 0);  // 把数据回发给客户端
            }
            /*用户输入信息了,开始处理信息并发送*/
            if (FD_ISSET(0, &rfds)) {
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                fgets(buffer, sizeof(buffer), stdin);

                // send(conn, buf, sizeof(buf), 0);
                std::string str = buffer;
                if (str == "capture\n") {
                    JsonPhaser jph;
                    jph.CreatePhotoMsg(true);
                    sprintf(buffer, "%s", jph.GetString().c_str());
                }
                int len = send(conn, buffer, strlen(buffer), 0);
                printf("you are typing %s send len = %d \n", buffer, len);
            }
        }
    }
    close(conn);
    close(ss);
    return 0;
}