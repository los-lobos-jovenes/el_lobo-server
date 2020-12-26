#ifndef _COMMDB_HPP
#define _COMMDB_HPP

#include <string>
#include <chrono>
#include <map>
#include <vector>
#include <algorithm>
#include <mutex>
#include <condition_variable>

class commEntry
{
    std::chrono::time_point<std::chrono::system_clock> timestamp;

    friend class commContainer;

    public:

    std::string from, to, payload;

        commEntry(std::string from, std::string to, std::string payload)
        {
            this->from = from;
            this->to = to;
            this->payload = payload;

            this->timestamp = std::chrono::system_clock::now();
        }

};

class commContainer
{
    /*
        Format:
        -> target user (the receipent of the message)
            \\-> the origin (the sender of the message)
            \\-> the message itself
    */
    static std::mutex protector;
    static std::condition_variable condition;
    static std::map<std::string, std::multimap<std::string, commEntry>> msgs;

    public:

        commContainer() = delete;
        commContainer(commContainer&) = delete;
        commContainer(commContainer&&) = delete;
        void operator=(const commContainer&) = delete;

        static void processAndAcceptComm(commEntry c)
        {
            msgs[c.to].insert({c.from, c});
        }

        static std::vector<commEntry> pullCommsForUserFromUser(std::string receipent, std::string fromWho)
        {
            auto cs = msgs[receipent].equal_range(fromWho);

            /*if(std::distance(cs.first, cs.second) == 0)
            {
                return {};
            }*/

            std::vector<commEntry> retcs = {};

            for(auto it = cs.first; it != cs.second; it++)
            {
                retcs.push_back(it->second);
            }
            return retcs;
        }

        static std::vector<std::string> peekPendingForUser(std::string receipent)
        {
            const auto &cs = msgs[receipent];
            std::vector<std::string> rets;

            for(auto it = cs.begin(); it != cs.end(); it = cs.upper_bound(it->first))
            {
                rets.push_back(it->first);
            }
            return rets;
        }

        static void deleteCommsForUserFromUser(std::string receipent, std::string fromWho)
        {
            auto cs = msgs[receipent].equal_range(fromWho);

            for(auto it = cs.first; it != cs.second; it++)
            {
                msgs[receipent].erase(it);
            }
        }

};

#endif