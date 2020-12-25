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

#include "logger.hpp"
#include "msg.hpp"
#include "user.hpp"
#include "commDb.hpp"

#define UNUSED(x) (void)x

// At this point we have something that looks like a valid command. Let's parse it!
void serve_command(int clientDesc, std::string command)
{
        if(command.empty())
        {
                return;
        }

        msg tmp;
        msg message;
        message.decode(command);

        auto version = std::atoi(message[0].c_str()); // Returns 0 on error - fine - we have 1 as lowes number
        auto header  = message[1];

        std::cout<<"[DEBUG] Serving protocol version "<<version<<std::endl;
        
        switch(version)
        {
                case 1:
                {
                        if(header == "CREA")
                        {
                                auto username = message[3];

                                if(username.empty())
                                {
                                        tmp.form("1", "RETN", "2", "ERROR", "USERNAME_EMPTY");
                                        auto ret = tmp.concat();
                                        write(clientDesc, ret.c_str(), ret.size());
                                        break;
                                }

                                auto password = message[4];

                                if(password.empty())
                                {
                                        tmp.form("1", "RETN", "2", "ERROR", "NO_PSWD_PROVIDED");
                                        auto ret = tmp.concat();
                                        write(clientDesc, ret.c_str(), ret.size());
                                        break;
                                }

                                auto result = userContainer::addUser(username, password);

                                if(!result)
                                {
                                        tmp.form("1", "RETN", "2", "ERROR", "USERNAME_TAKEN");
                                        auto ret = tmp.concat();
                                        write(clientDesc, ret.c_str(), ret.size());
                                        break;
                                }
                                
                                tmp.form("1", "RETN", "1", "SUCCESS");
                                auto ret = tmp.concat();
                                write(clientDesc, ret.c_str(), ret.size()); 
                        }
                }
                break;

                default:
                std::cout<<"[ERROR] Unknown protocol version."<<std::endl;
                break;
        }
        
}

void driver_func(int clientDesc)
{
        std::thread::id this_id = std::this_thread::get_id();

        std::cout<<"[DEBUG] Started thread to serve client; thread id: "<<this_id<<"; client id: "<<clientDesc<<std::endl;
        //write(clientDesc, "Dziala", sizeof(char) * 7);
        
        char * buf = new char[100];
        short last;

        std::string command = "";

        do
        {
                last = read(clientDesc, (void *)buf, sizeof(char) * 100);
                if(command.size() > 550)
                {
                        std::cout<<"[DEBUG] Disconnecting - command size exceeds all expectations; thread id: "<<this_id<<"; client id: "<<clientDesc<<std::endl;
                        break;
                }
                if(last <= 0)
                {
                        std::cout<<"[DEBUG] Reacting to disconnect; thread id: "<<this_id<<"; client id: "<<clientDesc<<std::endl;
                }

                command.append(buf, last);
                
                auto fixed = msg::fixupCommand(command);

                while(std::get<0>(fixed) == true)
                {
                        command = std::get<2>(fixed);

                        std::cout<<"[DEBUG] "<<command<<" vs "<<std::get<1>(fixed)<<std::endl;

                        serve_command(clientDesc, std::get<1>(fixed));

                        fixed = msg::fixupCommand(command);
                }


        }while(last > 0);

        close(clientDesc);
        std::cout<<"[DEBUG] Disconnected"<<std::endl;

        delete [] buf;

        std::cout<<"[DEBUG] Stopped thread to serve client; thread id: "<<this_id<<"; client id: "<<clientDesc<<std::endl;
}

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
