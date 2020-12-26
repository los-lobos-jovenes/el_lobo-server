#ifndef _USER_HPP
#define _USER_HPP

#include <string>
#include <map>
#include <tuple>
#include <mutex>

class user {
    std::string name, pswd;

    friend class userContainer;

    public:

        user(std::string name, std::string pswd)
        {
            this->name = name;
            this->pswd = pswd;
        }
};

class userContainer {

    static std::map<std::string, std::unique_ptr<user>> users;
    static std::mutex protector;

    public:

        userContainer() = delete;
        userContainer(userContainer&) = delete;
        userContainer(userContainer&&) = delete;
        void operator=(const userContainer&) = delete;

        static bool addUser(std::string name, std::string pswd)
        {
            std::lock_guard<std::mutex> lock(protector);

            if(pswd == "")
            {
                return false;
            }
            bool success;
            std::tie(std::ignore, success) = users.insert( { name, std::make_unique<user>(name, pswd) } );
            return success;
        }
        static bool authenticateUser(std::string name, std::string pswd)
        {
            std::lock_guard<std::mutex> lock(protector);

            if(pswd == "")
            {
                return false;
            }
            if(users.find(name) != users.end())
            {
                return users[name]->pswd == pswd;
            }
            return false;
        }
        static bool probeUser(std::string name)
        {
            std::lock_guard<std::mutex> lock(protector);

            if(users.find(name) != users.end())
            {
                return true;
            }
            return false;
        }

        static void removeUser(std::string name)
        {
            std::lock_guard<std::mutex> lock(protector);

            users.erase(name);
        }

};

#endif //_USER_HPP