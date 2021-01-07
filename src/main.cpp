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
#include <signal.h>

#include "logger.hpp"
#include "commDb.hpp"
#include "subscriberDb.hpp"
#include "user.hpp"

extern void driver_func(int clientDesc);
extern void shut_down_proc(int signum);

Logger Debug("DEBUG", Logger::LogLevel::Debug);
Logger Warning("WARNING", Logger::LogLevel::Warning);
Logger Info("INFO", Logger::LogLevel::Info);
Logger Error("ERROR", Logger::LogLevel::Error);
Logger Fatal("FATAL ERROR", Logger::LogLevel::Fatal);

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL Debug
#endif

[[noreturn]]
void terminator()
{
        commContainer::dumpDb("comms.fail.db");
        userContainer::dumpDb("user.fail.db");
        Fatal.Log("Fatal termination event. Databases saved in emergency mode.");
        std::abort();
}

int main(int argc, char * argv[])
{
        Logger::GlobalLogLevel = Logger::LogLevel::DEBUG_LEVEL;

        signal(SIGINT, shut_down_proc);
        signal(SIGPIPE, SIG_IGN);

        std::set_terminate(&terminator);

        int port = 1300;
        int backlogSize = 5;
        char reuse_addr_val = 0;

        if(argc >= 2)
        {
                port = atoi(argv[1]);

        }
        sockaddr_in addr;
        int sock = socket(AF_INET, SOCK_STREAM, 0);

        if(sock < 0)
        {
                Error.Log("Cannot create socket.");
                exit(1);
        }

        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //inet_pton(AF_INET, INADDR_ANY, &(addr.sin_addr));

        if(bind(sock, (sockaddr *) &addr, sizeof(sockaddr_in)) != 0)
        {
                Error.Log("Cannot bind socket.");
                exit(1);
        }


        if(listen(sock, backlogSize) != 0)
        {
                Error.Log("Cannot listen on the socket.");
                exit(1);
        }

        if(commContainer::loadDb())
        {
                Info.Log("Restored message database.");
        }
        /*if(subscriberDb::loadDb())
        {
                Info.Log("Restored subscribers database.");
        }*/
        if(userContainer::loadDb())
        {
                Info.Log("Restored user database.");
        }

        Info.Log("Server is ready.");

        while(1)
        {

                Debug.Log("Listening for a new connection.");

                sockaddr_in client_data = {};
                socklen_t client_data_size = {};
                int clientDesc = accept(sock, (sockaddr *) &client_data, &client_data_size);
                if(clientDesc > 0)
                {
                        try
                        {
                                std::thread thr(driver_func, clientDesc);
                                thr.detach();
                        }
                        catch(std::system_error &e)
                        {
                                Fatal.Log("Could not create new thread:", e.what());
                                close(clientDesc);
                        }
                }
                else
                {
                        Error.Log("Accept failed.", clientDesc);
                }

                Debug.Log("Fork Successful...");

        }

        close(sock);
}
