#pragma once
#include "atype.h"
#ifndef Q_MOC_RUN
#include <boost/tokenizer.hpp>
#include <boost/function.hpp>
#endif
#include <utility>
#include <vector>
#include <map>

typedef boost::tokenizer<boost::escaped_list_separator<char> > tokenizer_t;
typedef boost::tokenizer<boost::char_separator<char> > cstokenizer_t;

typedef std::pair<std::string,std::string> pr_ss_t;
typedef std::pair<std::string,bool> pr_sb_t;
typedef std::pair<std::string,int32_t> pr_si_t;
typedef std::pair<int32_t,int32_t> pr_ii_t;

typedef std::vector<std::pair<std::string,int> > vecpr_si_t;

typedef std::map<uap_t,uap_t> mp_ua_ua_t;
typedef std::map<int,std::string> mp_is_t;
typedef std::map<std::string,bool> mp_sb_t;
typedef std::map<std::string,int> mp_si_t;
typedef std::map<std::string,std::string> mp_ss_t;
typedef std::map<std::string,pr_ii_t> mp_s_pr_ii_t;
typedef std::map<std::string,pr_ss_t> mp_s_pr_ss_t;

typedef boost::function<void ()> bindvoid_t;
typedef std::map<uap_t,boost::function<void ()> > dispatch_t;
typedef std::vector<uap_t> oprinfo_t;

