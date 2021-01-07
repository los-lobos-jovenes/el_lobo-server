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
#include <fstream>
#include <filesystem>

#include "logger.hpp"

struct commEntry
{
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string from, to, payload;
    bool isUnread = true;

    commEntry(std::string from, std::string to, std::string payload, std::chrono::nanoseconds reps, bool isUnread) : timestamp(reps)
    {
        this->from = from;
        this->to = to;
        this->payload = payload;
        this->isUnread = isUnread;
    }

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
    commContainer(commContainer &) = delete;
    commContainer(commContainer &&) = delete;
    void operator=(const commContainer &) = delete;

    static void processAndAcceptComm(commEntry c)
    {
        std::shared_lock lock(protector);

        msgs[c.to].insert({c.from, std::make_shared<commEntry>(c)});
    }

    static std::vector<std::shared_ptr<commEntry>> pullCommsForUserFromUser(std::string receipent, std::string fromWho, bool unreadOnly = false)
    {
        std::shared_lock lock(protector);

        auto cs = msgs[receipent].equal_range(fromWho);
        std::vector<std::shared_ptr<commEntry>> retcs = {};

        for (auto it = cs.first; it != cs.second; it++)
        {
            if (!unreadOnly || it->second->isUnread)
                retcs.push_back(it->second);
        }
        return retcs;
    }

    static std::vector<std::string> peekPendingForUser(std::string receipent)
    {
        std::shared_lock lock(protector);

        const auto &cs = msgs[receipent];
        std::vector<std::string> rets;

        for (auto it = cs.begin(); it != cs.end(); it = cs.upper_bound(it->first))
        {
            rets.push_back(it->first);
        }
        return rets;
    }

    static void deleteCommsForUserFromUser(std::string receipent, std::string fromWho, bool doDelete = true)
    {
        std::unique_lock lock(protector);

        if (doDelete)
        {
            msgs[receipent].erase(fromWho);
        }
        else
        {
            auto cs = msgs[receipent].equal_range(fromWho);
            for (auto it = cs.first; it != cs.second; it++)
            {
                it->second->isUnread = false;
            }
        }
    }

    static void deleteCommsBySharedPtr(std::shared_ptr<commEntry> elem, std::string receipent, bool doDelete = true)
    {
        std::unique_lock lock(protector);

        auto &subset = msgs[receipent];

        if (doDelete)
        {
            for (auto i = subset.begin(); i != subset.end();)
            {
                if (i->second.get() == elem.get())
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

    static void dumpDb(std::string filename = "comms.db")
    {
        std::unique_lock lock(protector);

        if (std::filesystem::exists(filename))
        {
            std::filesystem::copy_file(filename, filename + ".bak", std::filesystem::copy_options::overwrite_existing);
        }

        std::ofstream out(filename, std::ios::trunc | std::ios::binary);

        auto writeHelper = [&out](auto type) {
            auto tt = type;
            out.write(reinterpret_cast<const char *>(&tt), sizeof(decltype(tt)));
        };

        for (const auto &ms : msgs)
        {
            writeHelper(ms.first.size());
            out.write(ms.first.c_str(), ms.first.size());

            writeHelper(ms.second.size());
            for (const auto &i : ms.second)
            {
                writeHelper(i.first.size());
                out.write(i.first.c_str(), i.first.size());

                writeHelper(i.second->to.size());
                out.write(reinterpret_cast<const char *>(i.second->to.c_str()), i.second->to.size());
                writeHelper(i.second->from.size());
                out.write(reinterpret_cast<const char *>(i.second->from.c_str()), i.second->from.size());
                writeHelper(i.second->payload.size());
                out.write(reinterpret_cast<const char *>(i.second->payload.c_str()), i.second->payload.size());

                writeHelper(i.second->timestamp.time_since_epoch().count());
                writeHelper(i.second->isUnread);
            }
        }
        out.close();
    }

    static bool loadDb(std::string filename = "comms.db")
    {
        std::unique_lock lock(protector);

        std::ifstream in(filename, std::ios::binary);
        if (!in.is_open())
        {
            return false;
        }
        if (in.peek() == EOF)
        {
            return false;
        }

        std::string rec, targ, ff, from, to, pl;
        bool unread = false;
        std::chrono::system_clock::rep clk_reps = 0;
        decltype(rec.size()) helper = 0;

        auto readStringHelper = [&](std::string &s) -> void {
            in.read(reinterpret_cast<char *>(&helper), sizeof(decltype(helper)));
            s.resize(helper);
            in.read(&s[0], s.size());
        };

        while (in.peek() != EOF)
        {
            readStringHelper(targ);
            std::size_t n = 0;
            in.read(reinterpret_cast<char *>(&n), sizeof(std::size_t));

            Debug.Log("Restoring messages to ", targ, n);

            while (n--)
            {
                readStringHelper(ff);
                readStringHelper(to);
                readStringHelper(from);
                readStringHelper(pl);
                in.read(reinterpret_cast<char *>(&clk_reps), sizeof(decltype(clk_reps)));
                in.read(reinterpret_cast<char *>(&unread), sizeof(decltype(unread)));
                Debug.Log("->It is", to, from, std::to_string(clk_reps), std::to_string(unread), pl);
                msgs[targ].insert({ff, std::make_shared<commEntry>(from, to, pl, std::chrono::nanoseconds(clk_reps), unread)});
            }
        }
        in.close();

        return true;
    }
};

#endif