#ifndef _USER_HPP
#define _USER_HPP

#include <string>
#include <map>
#include <tuple>
#include <shared_mutex>
#include <filesystem>
#include <fstream>

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
    static std::shared_mutex protector;

    static bool isAlnum(const std::string s)
    {
        for(auto i : s)
        {
            if(!isalnum(i))
            {
                return false;
            }
        }
        return true;
    }

    public:

        userContainer() = delete;
        userContainer(userContainer&) = delete;
        userContainer(userContainer&&) = delete;
        void operator=(const userContainer&) = delete;

        static bool isValidString(const std::string name, const std::string pswd)
        {
            return (isAlnum(name) && isAlnum(pswd));
        }

        static bool addUser(std::string name, std::string pswd)
        {
            if(pswd == "")
            {
                return false;
            }

            std::unique_lock lock(protector);

            bool success;
            std::tie(std::ignore, success) = users.insert( { name, std::make_unique<user>(name, pswd) } );
            return success;
        }
        static bool authenticateUser(std::string name, std::string pswd)
        {
            if(pswd == "")
            {
                return false;
            }

            std::shared_lock lock(protector);

            if(users.find(name) != users.end())
            {
                return users[name]->pswd == pswd;
            }
            return false;
        }
        static bool probeUser(std::string name)
        {
            std::shared_lock lock(protector);

            if(users.find(name) != users.end())
            {
                return true;
            }
            return false;
        }

        static void removeUser(std::string name)
        {
            std::unique_lock lock(protector);

            users.erase(name);
        }

        static void dumpDb(std::string filename = "users.db")
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

            for(const auto &i : users)
            {
                writeHelper(i.first.size());
                out.write(i.first.c_str(), i.first.size());
                writeHelper(i.second->name.size());
                out.write(i.second->name.c_str(), i.second->name.size());
                writeHelper(i.second->pswd.size());
                out.write(i.second->pswd.c_str(), i.second->pswd.size());
            }
            out.close();
        }

        static bool loadDb(std::string filename = "users.db")
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

            std::string s, name, pswd;
            decltype(s.size()) helper = 0;

            auto readStringHelper = [&](std::string &s) -> void {
                in.read(reinterpret_cast<char*>(&helper), sizeof(decltype(helper)));
                s.resize(helper);
                in.read(&s[0], s.size());
            };

            while(in.peek() != EOF)
            {
                readStringHelper(s);
                readStringHelper(name);
                readStringHelper(pswd);
                Debug.Log("Restored user ", s, name);
                users.insert({s, std::make_unique<user>(name, pswd)});
            }
            in.close();

            return true;
        }

};

#endif //_USER_HPP