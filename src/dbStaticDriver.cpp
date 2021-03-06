#include "commDb.hpp"
#include "user.hpp"
#include "subscriberDb.hpp"
#include "logger.hpp"

// This is a place, where all static objects definitions go.

std::map<std::string, std::multimap<std::string, std::shared_ptr<commEntry>>> commContainer::msgs;
std::shared_mutex commContainer::protector;

std::shared_mutex userContainer::protector;
std::map<std::string, std::unique_ptr<user>> userContainer::users;

std::shared_mutex subscriberDb::protector;
std::map<std::string, int> subscriberDb::subscribers;

Logger::LogLevel Logger::GlobalLogLevel;
std::recursive_mutex Logger::protector;