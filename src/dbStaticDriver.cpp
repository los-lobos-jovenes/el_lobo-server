#include "commDb.hpp"
#include "user.hpp"
#include "subscriberDb.hpp"
#include "logger.hpp"

std::map<std::string, std::multimap<std::string, commEntry>> commContainer::msgs;
std::mutex commContainer::protector;

std::mutex userContainer::protector;
std::map<std::string, std::unique_ptr<user>> userContainer::users;

std::mutex subscriberDb::protector;
std::map<std::string, int> subscriberDb::subscribers;

Logger::LogLevel Logger::GlobalLogLevel;
std::recursive_mutex Logger::protector;