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

/*
Returns:
        -> if the arrived command is complete and valid
        -> the command
        -> the extras (e.g. glued next command) to be separated
*/
std::tuple<bool, std::string, std::string> fixupCommand(std::string cmd, char separator)
{
        if(cmd.size() < 9)
        {
                // No head and version found
                return {false, "", ""};
        }

        if(cmd[0] != separator)
        {
                // Null command - it was probably malformed, so let's reject it
                return {true, "", ""};
        }

        const auto adjBeg = cmd.begin() + 8;
        const auto begPl = std::find(adjBeg, cmd.end(), separator);

        if(begPl == cmd.end())
        {
                // No valid payload
                return {false, "", ""};
        }

        int noOfSections = std::atoi(cmd.substr(std::distance(cmd.begin(), adjBeg), std::distance(cmd.begin(), begPl)).c_str());
        int noOfArrivedSections = std::count(begPl, cmd.end(), separator) - 1;

        std::cout<<"[DEBUG] Sections: Expected "<<noOfSections<<" found "<<noOfArrivedSections<<std::endl;

        if(noOfSections > noOfArrivedSections)
        {
                // Not all of the command has arrived yet
                return {false, "", ""};
        }
        else if (noOfSections < noOfArrivedSections)
        {
                // More than one command glued together

                int secNo = 0;

                size_t endpos = 
                [&]() -> size_t {
                        for(auto i = begPl; i != cmd.end(); i++)
                        {
                                if(*i == separator)
                                {
                                        secNo++;
                                }
                                if(secNo == noOfSections + 1)
                                {
                                        return std::distance(cmd.begin(), i);
                                }
                        }
                        throw new std::runtime_error("[ERROR] Could not enumerate sections.");
                }();

                return {true, cmd.substr(0, endpos), cmd.substr(endpos + 1)};
        }
        else
        {
             return  {true, cmd.substr(0, cmd.find_last_of(separator)), ""};
        }

        throw new std::runtime_error("[ERROR] Could not validate command.");
}

void driver_func(int clientDesc)
{
        std::thread::id this_id = std::this_thread::get_id();

        std::cout<<"[DEBUG] Started thread to serve client; thread id: "<<this_id<<"; client id: "<<clientDesc<<std::endl;
        //write(clientDesc, "Dziala", sizeof(char) * 7);
        
        char * buf = new char[100];
        short last;
        char separator = ';';

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
                
                auto fixed = fixupCommand(command, separator);

                while(std::get<0>(fixed) == true)
                {
                        command = std::get<2>(fixed);

                        std::cout<<"[DEBUG] "<<command<<" vs "<<std::get<1>(fixed)<<std::endl;

                        // insert yet another driver here

                        fixed = fixupCommand(command, separator);
                }


        }while(last > 0);

        close(clientDesc);
        std::cout<<"[DEBUG] Disconnected"<<std::endl;

        delete [] buf;

        std::cout<<"[DEBUG] Stopped thread to serve client; thread id: "<<this_id<<"; client id: "<<clientDesc<<std::endl;
}

std::map<std::string, std::multimap<std::string, commEntry>> commContainer::msgs;

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
