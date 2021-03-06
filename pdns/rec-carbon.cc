#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "mtasker.hh"
#include "syncres.hh"
#include "rec_channel.hh"
#include "iputils.hh"
#include "logger.hh"
#include "arguments.hh"
#include "lock.hh"


void doCarbonDump(void*)
try
{
  string hostname;
  string instance_name;
  string namespace_name;
  vector<string> carbonServers;

  {
    std::lock_guard<std::mutex> l(g_carbon_config_lock);
    stringtok(carbonServers, arg()["carbon-server"], ", ");
    namespace_name=arg()["carbon-namespace"];
    hostname=arg()["carbon-ourname"];
    instance_name=arg()["carbon-instance"];
  }

  if(carbonServers.empty())
    return;

  if(namespace_name.empty()) {
    namespace_name="pdns";
  }
  if (hostname.empty()) {
    try {
      hostname = getCarbonHostName();
    }
    catch(const std::exception& e) {
      throw std::runtime_error(std::string("The 'carbon-ourname' setting has not been set and we are unable to determine the system's hostname: ") + e.what());
    }
  }
  if(instance_name.empty()) {
    instance_name="recursor";
  }

  registerAllStats();
  string msg;
  for(const auto& carbonServer: carbonServers) {
    ComboAddress remote(carbonServer, 2003);
    Socket s(remote.sin4.sin_family, SOCK_STREAM);

    s.setNonBlocking();
    s.connect(remote);  // we do the connect so the first attempt happens while we gather stats
 
    if(msg.empty()) {
      typedef map<string,string> all_t;
      all_t all=getAllStatsMap(StatComponent::Carbon);
      
      ostringstream str;
      time_t now=time(0);
      
      for(const all_t::value_type& val :  all) {
        str<<namespace_name<<'.'<<hostname<<'.'<<instance_name<<'.'<<val.first<<' '<<val.second<<' '<<now<<"\r\n";
      }
      msg = str.str();
    }

    auto ret = asendtcp(msg, &s);     // this will actually do the right thing waiting on the connect
    if (ret == LWResult::Result::Timeout) {
      g_log<<Logger::Warning<<"Timeout connecting/writing carbon data to "<<remote.toStringWithPort()<<endl;
    }
    else if (ret != LWResult::Result::Success) {
      g_log<<Logger::Warning<<"Error writing carbon data to "<<remote.toStringWithPort()<<": "<<stringerror()<<endl;
    }
  }
 }
catch(PDNSException& e)
{
  g_log<<Logger::Error<<"Error in carbon thread: "<<e.reason<<endl;
}
catch(std::exception& e)
{
  g_log<<Logger::Error<<"Error in carbon thread: "<<e.what()<<endl;
}
catch(...)
{
  g_log<<Logger::Error<<"Unknown error in carbon thread"<<endl;
}
