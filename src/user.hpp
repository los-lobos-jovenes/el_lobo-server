#ifndef _USER_HPP
#define _USER_HPP

#include <string>
#include <map>
#include <tuple>

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

    static std::map<std::string, user *> users;

    public:

        userContainer() = delete;
        userContainer(userContainer&) = delete;
        userContainer(userContainer&&) = delete;
        void operator=(const userContainer&) = delete;

        static bool addUser(std::string name, std::string pswd)
        {
            if(pswd == "")
            {
                return false;
            }
            bool success;
            std::tie(std::ignore, success) = users.insert( { name, new user(name, pswd) } );
            return success;
        }
        static bool authenticateUser(std::string name, std::string pswd)
        {
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
            if(users.find(name) != users.end())
            {
                return true;
            }
            return false;
        }

};

#endif //_USER_HPP