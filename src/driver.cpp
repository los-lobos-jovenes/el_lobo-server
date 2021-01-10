
#include <iostream>
#include <sstream>
#include <thread>
#include <string>
#include <unistd.h>
#include <algorithm>

#include "msg.hpp"
#include "user.hpp"
#include "commDb.hpp"
#include "subscriberDb.hpp"
#include "logger.hpp"

#define UNUSED(x) (void)x
#define DO_LIMIT_COMMAND_SIZE 0

#ifndef OLD_PULL_DISABLED
#define OLD_PULL_DISABLED 0
#endif

static void writeWrapper(int desc, std::string s)
{
        Debug.Log(Logger::bind("[","Client: ", desc, "]"), "Trying to send", s);

        int off = 0, n;
        do
        {
                n = write(desc, s.substr(off).c_str(), s.substr(off).size());
                off += n;
                if (n <= 0)
                {
                        break;
                }

        } while (n < static_cast<int>(s.substr(off).size()));

        if (n <= 0)
        {
                throw std::runtime_error("[ERROR] Lost write permission to a socket or client abruptly disconnected!");
        }
}

// These are write helpers
template <typename... T>
static void formAndWrite(int clientDesc, T... s)
{
        msg tmp;
        tmp.form(s...);
        writeWrapper(clientDesc, tmp.concat());
}

//notify subscribers
void notify(std::string target, std::string fromWho)
{
        try
        {
                auto ret = subscriberDb::getSubscriberDescriptor(target);
                if (ret != -1)
                {
                        formAndWrite(ret, "1", "ALRT", "2", "SUBSCRIPTION_ALERT", fromWho.c_str());
                }
        }
        catch (std::exception &e)
        {
                Error.Log("Could not notify subscription client. Releasing subscription for", target);
                subscriberDb::removeSubscriber(target);
        }
}

// At this point we have something that looks like a valid command. Let's parse it!
// It is a core driver - reacts to user commands
void serve_command(int clientDesc, std::string command)
{
        if (command.empty())
        {
                return;
        }

        msg message;
        message.decode(command.substr(1));

        auto version = std::atoi(message[0].c_str()); // Returns 0 on error - fine - we have 1 as lowes number
        auto header = message[1];

        Debug.Log(Logger::bind("[","Client: ", clientDesc, "]"), "Serving protocol version ", version);

        switch (version)
        {
        case 1:
        {
                if (header == "CREA") // Create user
                {
                        auto username = message[3];
                        if (username.empty())
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "USERNAME_EMPTY");
                                break;
                        }

                        auto password = message[4];
                        if (password.empty())
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "NO_PSWD_PROVIDED");
                                break;
                        }

                        if (!userContainer::isValidString(username, password))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "INVALID_CHARACTERS_IN_DATA");
                                break;
                        }

                        auto result = userContainer::addUser(username, password);
                        if (!result)
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "USERNAME_TAKEN");
                                break;
                        }
                        formAndWrite(clientDesc, "1", "RETN", "1", "SUCCESS");
                }
                else if (header == "SEND") // Send message
                {
                        auto username = message[3];
                        auto password = message[4];

                        if (!userContainer::authenticateUser(username, password))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "AUTHENTICATION_FAILED");
                                break;
                        }

                        auto payload = message[6];

                        if (payload.empty())
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "MESSAGE_IS_EMPTY");
                                break;
                        }

                        auto target = message[5];

                        if (!userContainer::probeUser(target))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "INVALID_TARGET");
                                break;
                        }

                        commContainer::processAndAcceptComm(commEntry(username, target, payload));
                        formAndWrite(clientDesc, "1", "RETN", "1", "SUCCESS");

                        std::thread notificator(notify, target, username);
                        notificator.detach();
                }
                else if (header == "PEND") // Get list of pending messages from users
                {
                        auto username = message[3];
                        auto password = message[4];

                        if (!userContainer::authenticateUser(username, password))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "AUTHENTICATION_FAILED");
                                break;
                        }

                        auto ret = commContainer::peekPendingForUser(username);

                        for (const auto i : ret)
                        {
                                formAndWrite(clientDesc, "1", "RETN", "1", i);
                        }
                        formAndWrite(clientDesc, "1", "ENDT", "0");
                }
                else if (header == "PULL" && !OLD_PULL_DISABLED) // Pull pending messages from user
                {
                        auto username = message[3];
                        auto password = message[4];

                        if (!userContainer::authenticateUser(username, password))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "AUTHENTICATION_FAILED");
                                break;
                        }

                        auto target = message[5];

                        if (!userContainer::probeUser(target))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "INVALID_TARGET");
                                break;
                        }

                        auto msgs = commContainer::pullCommsForUserFromUser(username, target);

                        for (const auto i : msgs)
                        {
                                formAndWrite(clientDesc, "1", "RETN", "3", i->getTimestampStr(), i->from, i->payload);
                                commContainer::deleteCommsBySharedPtr(i, username);
                        }
                        formAndWrite(clientDesc, "1", "ENDT", "0");
                        //commContainer::deleteCommsForUserFromUser(username, target); // Only when the tranmission ended succesfully purge buffers. If we lose connections, all new messages will still wait for us.
                }
                else if (header == "UNRG") // Delete own user (unregister)
                {
                        auto username = message[3];
                        auto password = message[4];

                        if (!userContainer::authenticateUser(username, password))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "AUTHENTICATION_FAILED");
                                break;
                        }
                        userContainer::removeUser(username);
                        subscriberDb::removeSubscriber(username);
                        commContainer::removeUser(username);
                        formAndWrite(clientDesc, "1", "RETN", "1", "BYE");
                }
                else if (header == "SUBS")
                {
                        auto username = message[3];
                        auto password = message[4];

                        if (!userContainer::authenticateUser(username, password))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "AUTHENTICATION_FAILED");
                                break;
                        }
                        subscriberDb::addSubscriber(username, clientDesc);
                        formAndWrite(clientDesc, "1", "RETN", "1", "SUBSCRIBED");
                }
                else if (header == "USUB")
                {
                        auto username = message[3];
                        auto password = message[4];

                        if (!userContainer::authenticateUser(username, password))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "AUTHENTICATION_FAILED");
                                break;
                        }
                        subscriberDb::removeSubscriber(username);
                        formAndWrite(clientDesc, "1", "RETN", "1", "UNSUBSCRIBED");
                }
        }
        break;

        case 2:
        {
                if (header == "PULL") // Pull unread messages, mark as read, do not delete
                {
                        auto username = message[3];
                        auto password = message[4];

                        if (!userContainer::authenticateUser(username, password))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "AUTHENTICATION_FAILED");
                                break;
                        }

                        auto target = message[5];

                        if (!userContainer::probeUser(target))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "INVALID_TARGET");
                                break;
                        }

                        auto msgs = commContainer::pullCommsForUserFromUser(username, target, true);

                        for (const auto i : msgs)
                        {
                                formAndWrite(clientDesc, "1", "RETN", "3", i->getTimestampStr(), i->from, i->payload);
                                commContainer::deleteCommsBySharedPtr(i, username, false);
                        }
                        formAndWrite(clientDesc, "1", "ENDT", "0");
                        //commContainer::deleteCommsForUserFromUser(username, target, false);
                }
                else if (header == "APLL") // Pull all messages, do not delete
                {
                        auto username = message[3];
                        auto password = message[4];

                        if (!userContainer::authenticateUser(username, password))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "AUTHENTICATION_FAILED");
                                break;
                        }

                        auto target = message[5];

                        if (!userContainer::probeUser(target))
                        {
                                formAndWrite(clientDesc, "1", "RETN", "2", "ERROR", "INVALID_TARGET");
                                break;
                        }

                        auto msgs1 = commContainer::pullCommsForUserFromUser(username, target);
                        auto msgs2 = commContainer::pullCommsForUserFromUser(target, username); // inverse of what is above - so messages I've sent to someone
                        auto sender = [&clientDesc](auto &msgs) {
                                for (const auto i : msgs)
                                {
                                        formAndWrite(clientDesc, "1", "RETN", "3", i->getTimestampStr(), i->from, i->payload);
                                }
                        };
                        sender(msgs1);
                        sender(msgs2);
                        formAndWrite(clientDesc, "1", "ENDT", "0");
                }
        }
        break;

        default:
                Debug.Log(Logger::bind("[","Client: ", clientDesc, "]"), "Unknown protocol version.");
                break;
        }
}


// Function to handle reading from sockets and TCP oddities (glueing and fragmentation)
void driver_func(int clientDesc)
{
        std::thread::id this_id = std::this_thread::get_id();
        try
        {
                Debug.Log(Logger::bind("[","Client: ", clientDesc, "]"), "Started thread to serve client; thread id:", this_id, "; client id:", clientDesc);

                // Arrays have contiguous mem allocation, so better option then some dynamic array (cleans nicer too)
                std::array<char, 100> buf;
                short last;
                std::string command = "";

                do
                {
                        last = read(clientDesc, reinterpret_cast<void *>(&buf[0]), sizeof(char) * 100);
                        if (command.size() > 550 && DO_LIMIT_COMMAND_SIZE)
                        {
                                Debug.Log(Logger::bind("[","Client: ", clientDesc, "]"), "Disconnecting - command size exceeds all expectations; thread id:", this_id, "; client id:", clientDesc);
                                break;
                        }
                        if (last <= 0)
                        {
                                Debug.Log(Logger::bind("[","Client: ", clientDesc, "]"), "Reacting to disconnect; thread id:", this_id, "; client id:", clientDesc);
                        }

                        command.append(buf.data(), last);
                        auto fixed = msg::fixupCommand(command);

                        while (std::get<0>(fixed) == true)
                        {
                                command = std::get<2>(fixed);
                                Debug.Log(Logger::bind("[","Client: ", clientDesc, "]"), "Queued part of command is:", command, " vs real:", std::get<1>(fixed));

                                serve_command(clientDesc, std::get<1>(fixed));
                                fixed = msg::fixupCommand(command);
                        }

                } while (last > 0);
        }
        catch (std::exception &e)
        {
                Fatal.Log("Exception in thread - killing thread, closing connection; thread id:", this_id, "; client id:", clientDesc, "; reason:", e.what());
        }

        close(clientDesc);
        subscriberDb::removeSubscriber(clientDesc);
        Debug.Log("Stopped thread to serve client; thread id:", this_id, "; client id:", clientDesc);
}

// Graceful shutdown procedure
void shut_down_proc(int signum)
{
        UNUSED(signum);

        Info.Log("Saving databases...");
        userContainer::dumpDb();
        commContainer::dumpDb();

        Info.Log("Shutting down server...");
        exit(0);
}