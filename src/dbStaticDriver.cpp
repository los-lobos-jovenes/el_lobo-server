#include "commDb.hpp"
#include "user.hpp"

std::map<std::string, std::multimap<std::string, commEntry>> commContainer::msgs;
std::map<std::string, user *> userContainer::users;