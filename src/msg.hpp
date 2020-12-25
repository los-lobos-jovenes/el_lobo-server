#ifndef _MSG_HPP
#define _MSG_HPP

#include <string>
#include <vector>
#include <sstream>

class msg{

    std::vector<std::string> parts;
    char separator = '\31';

    public:

        template <typename T>
        void form(const T &part)
        {
            parts.push_back(part);
        }

        template <typename T, typename ... Targs>
        void form(const T &part, Targs ...parts)
        {
            this->parts.push_back(part);
            form(parts...);
        }


        std::string concat() const
        {
            std::string sum;
            sum += separator;

            for(auto i : parts)
            {
                sum += i;
                sum += separator;
            }
            return sum;
        }

        std::string extract(std::size_t no) const
        {
            if(no < parts.size())
            {
                return parts[no];
            }
            return "";
        }

        void decode(std::string s)
        {
            std::stringstream ss;
            ss << s;
            std::string part;

            while (getline(ss, part, separator))
            {
                this->parts.push_back(part);
            }
        }

};

#endif