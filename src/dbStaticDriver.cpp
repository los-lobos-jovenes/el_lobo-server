#include "commDb.hpp"
#include "user.hpp"
#include "subscriberDb.hpp"

std::map<std::string, std::multimap<std::string, commEntry>> commContainer::msgs;
std::mutex commContainer::protector;
std::mutex userContainer::protector;
std::map<std::string, user *> userContainer::users;
std::mutex subscriberDb::protector;
std::map<std::string, int> subscriberDb::subscribers;