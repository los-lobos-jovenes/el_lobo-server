#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <thread>
#include <string>
#include <algorithm>

extern void driver_func(int clientDesc);

int main(int argc, char * argv[])
{
        int port = 1300;
        int backlogSize = 2;
        char reuse_addr_val = 0;

        if(argc >= 2)
        {
                port = atoi(argv[1]);

        }
        sockaddr_in addr;
        int sock = socket(AF_INET, SOCK_STREAM, 0);

        if(sock < 0)
        {
                std::cout<<"[ERROR] Cannot create socket."<<std::endl;
        }

        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //inet_pton(AF_INET, INADDR_ANY, &(addr.sin_addr));

        if(bind(sock, (sockaddr *) &addr, sizeof(sockaddr_in)) != 0)
        {
                std::cout<<"[ERROR] Cannot bind socket."<<std::endl;
                exit(1);
        }


        if(listen(sock, backlogSize) != 0)
        {
                std::cout<<"[ERROR] Cannot listen on the socket."<<std::endl;
                exit(1);
        }

        while(1)
        {

                std::cout<<"[DEBUG] Listening for a new connection."<<std::endl;

                sockaddr_in client_data = {};
                socklen_t client_data_size = {};
                int clientDesc = accept(sock, (sockaddr *) &client_data, &client_data_size);
                if(clientDesc > 0)
                {
                        std::thread thr(driver_func, clientDesc);
                        thr.detach();
                }
                else
                {
                        std::cout<<"[ERROR] Accept failed. "<<clientDesc<<std::endl;
                }

                std::cout<<"[DEBUG] Fork Successful..."<<std::endl;

        }

}
