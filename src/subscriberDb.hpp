#ifndef _SUBSCRIBER_DB
#define _SUBSCRIBER_DB

#include <map>
#include <shared_mutex>
#include <mutex>
#include <fstream>
#include <filesystem>


// This class stores information about subscriptions
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

};

#endif