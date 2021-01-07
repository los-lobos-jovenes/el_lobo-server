#ifndef _SUBSCRIBER_DB
#define _SUBSCRIBER_DB

#include <map>
#include <shared_mutex>
#include <mutex>
#include <fstream>
#include <filesystem>

class subscriberDb
{
    static std::shared_mutex protector;
    // username, socket descriptor
    static std::map<std::string, int> subscribers;

public:
    static void addSubscriber(std::string user, int desc)
    {
        std::shared_lock lock(protector);

        subscribers.insert({user, desc});
    }
    static int getSubscriberDescriptor(std::string user)
    {
        std::shared_lock lock(protector);

        const auto cs = subscribers.find(user);
        if (cs != subscribers.end())
        {
            return cs->second;
        }
        return -1;
    }

    static void removeSubscriber(std::string user)
    {
        std::unique_lock lock(protector);

        subscribers.erase(user);
    }

    static void removeSubscriber(int desc)
    {
        std::unique_lock lock(protector);

        for (auto it = subscribers.begin(); it != subscribers.end(); ++it)
        {
            if (it->second == desc)
            {
                subscribers.erase(it);
            }
        }
    }

// rejected, forever
#if 0
    static void dumpDb(std::string filename = "subscribers.db")
    {
        std::unique_lock lock(protector);

        if(std::filesystem::exists(filename))
        {
            std::filesystem::copy_file(filename, filename + ".bak", std::filesystem::copy_options::overwrite_existing);
        }

        std::ofstream out(filename, std::ios::trunc | std::ios::binary);

        auto writeHelper = [&out](auto type)
        {
            auto tt = type;
            out.write(reinterpret_cast<const char *>(&tt), sizeof(decltype(tt)));
        };

        for(const auto &i : subscribers)
        {
            //out.write(reinterpret_cast<const char *>(i.first.size()), sizeof(i.first.size()));
            writeHelper(i.first.size());
            out.write(i.first.c_str(), i.first.size());
            //out.write(reinterpret_cast<const char *>(i.second), sizeof(i.second));
            writeHelper(i.second);
        }
        out.close();
    }

    static bool loadDb(std::string filename = "subscribers.db")
    {
        std::unique_lock lock(protector);

        std::ifstream in(filename, std::ios::binary);
        if(!in.is_open())
        {
            return false;
        }
        if(in.peek() == EOF)
        {
            return false;
        }

        int desc;
        std::string s;
        decltype(s.size()) size;

        while(in.peek() != EOF)
        {
            in.read(reinterpret_cast<char*>(&size), sizeof(decltype(size)));
            s.resize(size);
            in.read(&s[0], s.size());
            in.read(reinterpret_cast<char*>(&desc), sizeof(decltype(desc)));
            subscribers.insert({s, desc});
        }
        in.close();

        return true;
    }
#endif
};

#endif