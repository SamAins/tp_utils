#include "tp_utils/DebugUtils.h"
#include "tp_utils/MutexUtils.h"
#include "tp_utils/StackTrace.h"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <atomic>

#ifdef TDP_ANDROID
#include <android/log.h>
#endif

namespace tp_utils
{
namespace
{
TPMutex debugMutex{TPM};
std::function<void(MessageType, const std::string&)> debugCallback;
std::function<void(const std::string&, DebugType, const std::string&)> tableCallback;
std::unordered_map<std::string, std::unordered_map<int, bool>> enabledDebugModeObjects;
std::vector<DebugMode*> debugModeObjects;

//##################################################################################################
void handleSignal(int signum)
{
  tpWarning() << "Signal caught: " << signum;
  printStackTrace();
}
}

//##################################################################################################
void installSignalHandler()
{
  signal(SIGABRT, &handleSignal);
}

//##################################################################################################
void installMessageHandler(const std::function<void(MessageType, const std::string&)>& callback)
{
  TP_MUTEX_LOCKER(debugMutex);
  debugCallback = callback;
}

//##################################################################################################
struct DebugMode::Private
{
  const std::string& classPath;
  DebugType debugType;
  std::atomic_bool enabled{false};

  //################################################################################################
  Private(const std::string& classPath_, DebugType debugType_):
    classPath(classPath_),
    debugType(debugType_)
  {

  }
};

//##################################################################################################
DebugMode::DebugMode(const std::string& classPath, DebugType debugType):
  d(new Private(classPath, debugType))
{
  TP_MUTEX_LOCKER(debugMutex);
  debugModeObjects.push_back(this);
  auto i = enabledDebugModeObjects.find(classPath);
  if(i != enabledDebugModeObjects.end())
  {
    auto ii = i->second.find(int(debugType));
    if(ii != i->second.end())
      d->enabled = ii->second;
  }
}

//##################################################################################################
DebugMode::~DebugMode()
{
  TP_MUTEX_LOCKER(debugMutex);
  tpRemoveOne(debugModeObjects, this);
}

//##################################################################################################
bool DebugMode::operator()()
{
  return d->enabled;
}

//##################################################################################################
void DebugMode::setTable(const std::string& table)
{
  if(d->enabled)
  {
    TP_MUTEX_LOCKER(debugMutex);
    if(tableCallback)
      tableCallback(d->classPath, d->debugType, table);
  }
}

//##################################################################################################
void DebugMode::installTableCallback(std::function<void(const std::string&, DebugType, const std::string&)> callback)
{
  TP_MUTEX_LOCKER(debugMutex);
  tableCallback = std::move(callback);
}

//##################################################################################################
void DebugMode::enable(const std::string& classPath, DebugType debugType, bool enabled)
{
  TP_MUTEX_LOCKER(debugMutex);
  enabledDebugModeObjects[classPath][int(debugType)] = enabled;
  for(auto dm : debugModeObjects)
    if(dm->d->classPath == classPath && dm->d->debugType == debugType)
      dm->d->enabled = enabled;
}

//##################################################################################################
std::vector<std::string> DebugMode::classPaths(DebugType debugType)
{
  TP_MUTEX_LOCKER(debugMutex);
  std::vector<std::string> classPaths;

  for(DebugMode* dm : debugModeObjects)
    if(dm->d->debugType == debugType)
      classPaths.push_back(dm->d->classPath);

  return classPaths;
}

//##################################################################################################
int DebugBuffer::sync()
{
  TP_MUTEX_LOCKER(debugMutex);
  if(debugCallback)
    debugCallback(MessageType::Warning, str());
  else
  {
    std::cout << str();
    std::cout.flush();
  }

  str("");
  return 0;
}

namespace DBG
{

//##################################################################################################
struct Default : public Base
{
  TP_NONCOPYABLE(Default);

  Default();
  ~Default()override;
  std::ostream& operator()()override;

  DebugBuffer m_buffer;
  std::ostream m_stream;
};

using DefaultFactory = FactoryTemplate<Default>;

//##################################################################################################
Default::Default():
  m_stream(&m_buffer)
{

}

//##################################################################################################
Default::~Default()
{
  m_stream << std::endl;
}

//##################################################################################################
std::ostream& Default::operator()()
{
  return m_stream;
}

//##################################################################################################
struct Manager::Private
{
  std::mutex mutex;
  FactoryBase* warningFactory{new DefaultFactory()};
  FactoryBase* debugFactory{new DefaultFactory()};
};

//##################################################################################################
Manager::Manager():
  d(new Private())
{

}

//##################################################################################################
Manager::~Manager()
{
  delete d;
}

//##################################################################################################
void Manager::setWarning(FactoryBase* warningFactory)
{
  std::lock_guard<std::mutex> lg(d->mutex);
  TP_UNUSED(lg);
  delete d->warningFactory;
  d->warningFactory = warningFactory;
}

//##################################################################################################
Base* Manager::produceWarning()
{
  std::lock_guard<std::mutex> lg(d->mutex);
  TP_UNUSED(lg);
  return d->warningFactory->produce();
}

//##################################################################################################
void Manager::setDebug(FactoryBase* debugFactory)
{
  std::lock_guard<std::mutex> lg(d->mutex);
  TP_UNUSED(lg);
  delete d->debugFactory;
  d->debugFactory = debugFactory;
}

//##################################################################################################
Base* Manager::produceDebug()
{
  std::lock_guard<std::mutex> lg(d->mutex);
  TP_UNUSED(lg);
  return d->debugFactory->produce();
}

//##################################################################################################
Manager& Manager::instance()
{
  static Manager instance;
  return instance;
}
}

//##################################################################################################
DebugHelper::DebugHelper(DBG::Base* dbg):
  m_dbg(dbg)
{

}

//##################################################################################################
DebugHelper::~DebugHelper()
{
  delete m_dbg;
}

//##################################################################################################
std::ostream& DebugHelper::operator()()
{
  return (*m_dbg)();
}

//##################################################################################################
//## Platform Abstractions #########################################################################
//##################################################################################################

#ifdef TDP_ANDROID
namespace
{
//##################################################################################################
void messageHandler(tp_utils::MessageType messageType, const std::string& message)
{
  const char* tag="tpDebug";
  switch(messageType)
  {
  case tp_utils::MessageType::Debug:   tag="tpDebug";   break;
  case tp_utils::MessageType::Warning: tag="tpWarning"; break;
  }
  __android_log_print(ANDROID_LOG_DEBUG, tag, "%s", message.c_str());
}
}
#endif

//##################################################################################################
void installDefaultMessageHandler()
{
#ifdef __ANDROID_API__
  tp_utils::installMessageHandler(messageHandler);
#endif
}

}
