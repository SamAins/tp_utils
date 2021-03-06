#ifndef tp_utils_DebugUtils_h
#define tp_utils_DebugUtils_h

#include "tp_utils/Globals.h"

#include <sstream>
#include <unordered_set>

#define tpWarning tp_utils::DebugHelper(tp_utils::DBG::Manager::instance().produceWarning())
#define tpDebug tp_utils::DebugHelper(tp_utils::DBG::Manager::instance().produceDebug())

//##################################################################################################
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
  os << "( ";
  for(const auto& i : v)
    os << i << ' ';
  os << ')';
  return os;
}

//##################################################################################################
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::unordered_set<T>& v)
{
  os << "( ";
  for(const auto& i : v)
    os << i << ' ';
  os << ')';
  return os;
}

namespace tp_utils
{

//##################################################################################################
enum class MessageType
{
  Warning,
  Debug
};

//##################################################################################################
enum class DebugType
{
  Console,
  Table
};

//##################################################################################################
void installSignalHandler();

//##################################################################################################
void installMessageHandler(const std::function<void(MessageType, const std::string&)>& callback);

//##################################################################################################
class DebugMode
{
public:
  //################################################################################################
  DebugMode(const std::string& classPath, DebugType debugType=DebugType::Console);

  //################################################################################################
  ~DebugMode();

  //################################################################################################
  bool operator()();

  //################################################################################################
  //! Sets a large blob to debug that may be served to the user in a single chunk
  void setTable(const std::string& table);

  //################################################################################################
  static void installTableCallback(std::function<void(const std::string&, DebugType, const std::string&)> callback);

  //################################################################################################
  static void enable(const std::string& classPath, DebugType debugType, bool enabled);

  //################################################################################################
  static std::vector<std::string> classPaths(DebugType debugType);

private:
  struct Private;
  friend struct Private;
  Private* d;
};

//##################################################################################################
class DebugBuffer : public std::stringbuf
{
public:
  //################################################################################################
    int sync() override;
};

namespace DBG
{
//##################################################################################################
struct Base
{
  TP_NONCOPYABLE(Base);

  Base()=default;
  virtual ~Base()=default;
  virtual std::ostream& operator()()=0;
};

//##################################################################################################
struct FactoryBase
{
  TP_NONCOPYABLE(FactoryBase);

  FactoryBase()=default;
  virtual ~FactoryBase()=default;
  virtual Base* produce()=0;
};

//##################################################################################################
template<typename T>
struct FactoryTemplate : public FactoryBase
{
  TP_NONCOPYABLE(FactoryTemplate);

  FactoryTemplate()=default;
  ~FactoryTemplate() override = default;
  Base* produce() override
  {
    return new T();
  }
};

//##################################################################################################
struct Manager
{
  Manager();
  ~Manager();

  void setWarning(FactoryBase* warningFactory);
  Base* produceWarning();

  void setDebug(FactoryBase* debugFactory);
  Base* produceDebug();

  static Manager& instance();

  struct Private;
  Private* d;
};
}

//##################################################################################################
struct DebugHelper
{
  TP_NONCOPYABLE(DebugHelper);

  DebugHelper(DBG::Base* dbg);
  ~DebugHelper();
  std::ostream& operator()();

  DBG::Base* m_dbg;
};

//##################################################################################################
void installDefaultMessageHandler();

}

#endif
