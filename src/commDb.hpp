#ifndef _COMMDB_HPP
#define _COMMDB_HPP

#include <string>
#include <chrono>
#include <map>
#include <vector>
#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <memory>

#include "logger.hpp"

struct commEntry
{
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string from, to, payload;
    bool isUnread = true;

        commEntry(std::string from, std::string to, std::string payload)
        {
            this->from = from;
            this->to = to;
            this->payload = payload;

            this->timestamp = std::chrono::system_clock::now();
        }

        // https://stackoverflow.com/questions/31255486/c-how-do-i-convert-a-stdchronotime-point-to-long-and-back
        auto getTimestampStr() const
        {
            std::time_t t = std::chrono::system_clock::to_time_t(timestamp);
            return std::to_string(t);
            //return ctime(&t);
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
    static std::shared_mutex protector;
    static std::map<std::string, std::multimap<std::string, std::shared_ptr<commEntry>>> msgs;

    public:

        commContainer() = delete;
        commContainer(commContainer&) = delete;
        commContainer(commContainer&&) = delete;
        void operator=(const commContainer&) = delete;

        static void processAndAcceptComm(commEntry c)
        {
            std::shared_lock lock(protector);

            msgs[c.to].insert({ c.from, std::make_shared<commEntry>(c) });
        }

        static std::vector<std::shared_ptr<commEntry>> pullCommsForUserFromUser(std::string receipent, std::string fromWho, bool unreadOnly = false)
        {
            std::shared_lock lock(protector);

            auto cs = msgs[receipent].equal_range(fromWho);
            std::vector<std::shared_ptr<commEntry>> retcs = {};

            for(auto it = cs.first; it != cs.second; it++)
            {
                if(!unreadOnly || it->second->isUnread)retcs.push_back(it->second);
            }
            return retcs;
        }

        static std::vector<std::string> peekPendingForUser(std::string receipent)
        {
            std::shared_lock lock(protector);

            const auto &cs = msgs[receipent];
            std::vector<std::string> rets;

            for(auto it = cs.begin(); it != cs.end(); it = cs.upper_bound(it->first))
            {
                rets.push_back(it->first);
            }
            return rets;
        }

        static void deleteCommsForUserFromUser(std::string receipent, std::string fromWho, bool doDelete = true)
        {
            std::unique_lock lock(protector);

            if(doDelete)
            {
                msgs[receipent].erase(fromWho);
            }
            else
            {
                auto cs = msgs[receipent].equal_range(fromWho);
                for(auto it = cs.first; it != cs.second; it++)
                {
                    it->second->isUnread = false;
                }
            }
        }

        static void deleteCommsBySharedPtr(std::shared_ptr<commEntry> elem, std::string receipent, bool doDelete = true)
        {
            std::unique_lock lock(protector);

            auto& subset = msgs[receipent];

            if(doDelete)
            {
                for(auto i = subset.begin(); i != subset.end();)
                {
                    if(i->second.get() == elem.get())
                    {
                        i = subset.erase(i);
                    }
                    else
                    {
                        i++;
                    }
                    
                }
            }
            else
            {
                elem->isUnread = false;
            }
        }

        static void removeUser(std::string username)
        {
            std::unique_lock lock(protector);

            msgs.erase(username);
        }
};

#endif