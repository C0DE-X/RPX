#include <iostream>
#include <string>

#include <rpx/RemoteManager.h>
#include <rpx/RemoteObject.h>
#include <rpx/TCPServer.h>
#include <rpx/PipeNode.h>

class TestObject : public rpx::RemoteObject {
public:
  TestObject(std::string const &id) : rpx::RemoteObject(id) {
    registerRemote("add", &TestObject::add);
    registerRemote("append", &TestObject::append);
    registerRemote("echo", &TestObject::echo);
  };
  ~TestObject() override = default;

  float add(float f, float ff) { return f + ff; }
  std::string append(std::string s, std::string app) { return s + app; }
  void echo() { bool success = callRemote<void>(__FUNCTION__); }
};

int main() {
  /*rpx::communication::TCPServer server;
  server.listen(12345);
  rpx::RemoteManager::addCommunication(&server);*/

  rpx::communication::PipeNode node("/tmp/rpx0");
  rpx::RemoteManager::addCommunication(&node);
  std::cout << "Remote server started" << std::endl;
  TestObject obj("fID");

  std::cin.get();

  std::cout << "Calling remote echo" << std::endl;
  obj.echo();

  std::cin.get();
  return EXIT_SUCCESS;
}
