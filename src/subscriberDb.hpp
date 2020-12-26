#ifndef _SUBSCRIBER_DB
#define _SUBSCRIBER_DB

#include <map>

class subscriberDb
{
    static std::mutex protector;
     // username, socket descriptor
    static std::map<std::string, int> subscribers;

    public:

    static void addSubscriber(std::string user, int desc)
    {
        std::lock_guard<std::mutex> lock(protector);

        subscribers.insert({user, desc});
    }
    static int getSubscriberDescriptor(std::string user)
    {
        std::lock_guard<std::mutex> lock(protector);

        const auto cs = subscribers.find(user);
        if(cs != subscribers.end())
        {
            return cs->second;
        }
        return -1;
    }

    static void removeSubscriber(std::string user)
    {
        std::lock_guard<std::mutex> lock(protector);

        subscribers.erase(user);
    }

    static void removeSubscriber(int desc)
    {
        std::lock_guard<std::mutex> lock(protector);
        
        for (auto it = subscribers.begin(); it != subscribers.end(); ++it)
        {
            if (it->second == desc)
            {
                subscribers.erase(it);
                return;
            }
        }
    }
};

#endif