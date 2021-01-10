#ifndef _MSG_HPP
#define _MSG_HPP

#include <string>
#include <vector>
#include <sstream>

#include "logger.hpp"

// Config entry point
#ifndef MSG_SEPARATOR
#define MSG_SEPARATOR '\31'
#endif

// This class helps in parsing commands
class msg
{

        std::vector<std::string> parts;
        static constexpr char separator = MSG_SEPARATOR;

public:
        msg() = default;

        template <typename T>
        msg &form(const T &part)
        {
                parts.push_back(part);
                return *this;
        }

        template <typename T, typename... Targs>
        msg &form(const T &part, Targs... parts)
        {
                this->parts.push_back(part);
                form(parts...);
                return *this;
        }

        std::string concat() const
        {
                std::string sum = "";
                sum += separator;

                for (auto i : parts)
                {
                        sum += i;
                        sum += separator;
                }
                return sum;
        }

        std::string operator[](std::size_t no) const
        {
                return this->extract(no);
        }

        std::string extract(std::size_t no) const
        {
                if (no < parts.size())
                {
                        return parts[no];
                }
                return "";
        }

        msg &decode(std::string s)
        {
                std::stringstream ss;
                ss << s;
                std::string part;

                while (getline(ss, part, separator))
                {
                        this->parts.push_back(part);
                }
                return *this;
        }

        /*
        Returns:
                -> if the arrived command is complete and valid
                -> the command
                -> the extras (e.g. glued next command) to be separated
        */
        static std::tuple<bool, std::string, std::string> fixupCommand(std::string cmd)
        {
                if (cmd.size() < 9)
                {
                        // No head and version found
                        return {false, "", ""};
                }

                if (cmd[0] != separator)
                {
                        // Invalid command format - no sep version - it was probably malformed, so let's reject some section and give it a chance
                        auto pp = cmd.find_first_of(separator);
                        if (pp == std::string::npos)
                        {
                                // Probably not a payload at all.
                                return {true, "", ""};
                        }
                        return {true, "", cmd.substr(pp)};
                }
                /*
                XXX - better culling - non critical, no crashes will occur
                else
                {
                        // It might be a sep ver
                        auto cmd2 = cmd.substr(1); // Reject first sep and find next
                        auto pp = cmd2.find_first_of(separator);
                        if(pp == std::string::npos)
                        {
                                // Probably not a payload at all.
                                return {true, "", ""};
                        }
                        return {true, "", cmd2.substr(pp) };
                }*/

                const auto adjBeg = cmd.begin() + 8;
                const auto begPl = std::find(adjBeg, cmd.end(), separator);

                if (begPl == cmd.end())
                {
                        // No valid payload
                        return {false, "", ""};
                }

                int noOfSections = std::atoi(cmd.substr(std::distance(cmd.begin(), adjBeg), std::distance(cmd.begin(), begPl)).c_str());
                int noOfArrivedSections = std::count(begPl, cmd.end(), separator) - 1;

               // Debug.Log("Sections: Expected ", noOfSections, " found ", noOfArrivedSections);

                if (noOfSections > noOfArrivedSections)
                {
                        // Not all of the command has arrived yet
                        return {false, "", ""};
                }
                else if (noOfSections < noOfArrivedSections)
                {
                        // More than one command glued together

                        int secNo = 0;

                        size_t endpos =
                            [&]() -> size_t {
                                for (auto i = begPl; i != cmd.end(); i++)
                                {
                                        if (*i == separator)
                                        {
                                                secNo++;
                                        }
                                        if (secNo == noOfSections + 1)
                                        {
                                                return std::distance(cmd.begin(), i);
                                        }
                                }
                                throw std::runtime_error("[ERROR] Could not enumerate sections.");
                        }();

                        return {true, cmd.substr(0, endpos + 1), cmd.substr(endpos + 1)};
                }
                else
                {
                        return {true, cmd.substr(0, cmd.find_last_of(separator) + 1), ""};
                }

                throw std::runtime_error("[ERROR] Could not validate command.");
        }
};

#undef MSG_SEPARATOR

#endif