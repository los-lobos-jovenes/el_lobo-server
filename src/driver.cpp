
#include <iostream>
#include <sstream>
#include <thread>
#include <string>
#include <unistd.h>
#include <algorithm>
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

        msg message;
        message.decode(command.substr(1));

        auto writeWrapper = [](int desc, std::string s) -> void{
                int off = 0;
                int n;
                do
                {
                        n = write(desc, s.substr(off).c_str(), s.substr(0).size());
                        off += n;
                }
                while(n < static_cast<int>(s.substr(off).size()) && n > 0);

                if(n < 0)
                {
                        throw new std::runtime_error("[ERROR] Lost write permission to a socket!");
                }
        };

        auto formAndWrite = [&writeWrapper](int clientDesc, auto ...s) -> void{
                msg tmp;
                tmp.form(s...);
                writeWrapper(clientDesc, tmp.concat());
        };

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
                                        formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "USERNAME_EMPTY");
                                        break;
                                }

                                auto password = message[4];

                                if(password.empty())
                                {
                                        formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "NO_PSWD_PROVIDED");
                                        break;
                                }

                                auto result = userContainer::addUser(username, password);

                                if(!result)
                                {
                                        formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "USERNAME_TAKEN");
                                        break;
                                }
                                
                                formAndWrite(clientDesc, "1", "RETN", "1", "SUCCESS");
                        }
                        else if(header == "SEND")
                        {
                                auto username = message[3];
                                auto password = message[4];

                                if(!userContainer::authenticateUser(username, password))
                                {
                                        formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "AUTHENTICATION_FAILED");
                                        break;
                                }

                                auto payload = message[6];

                                if(payload.empty())
                                {
                                        formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "MESSAGE_IS_EMPTY");
                                        break;
                                }

                                auto target = message[5];

                                if(!userContainer::probeUser(target))
                                {
                                        formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "INVALID_TARGET");
                                        break;
                                }

                                commContainer::processAndAcceptComm(commEntry(username, target, payload));
                                formAndWrite(clientDesc, "1", "RETN", "1", "SUCCESS");
                        }
                        else if(header == "PEND")
                        {
                                auto username = message[3];
                                auto password = message[4];

                                if(!userContainer::authenticateUser(username, password))
                                {
                                        formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "AUTHENTICATION_FAILED");
                                        break;
                                }

                                auto ret = commContainer::peekPendingForUser(username);

                                for(const auto i : ret)
                                {
                                    formAndWrite(clientDesc, "1", "RETN", "1", i);
                                }
                                formAndWrite(clientDesc, "1", "ENDT", "0");
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
