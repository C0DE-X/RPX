#include <iostream>
#include <string>

#include <rpx/RemoteManager.h>
#include <rpx/RemoteObject.h>
#include <rpx/TCPClient.h>
#include <rpx/PipeNode.h>

class TestObject : public rpx::RemoteObject {
public:
  TestObject(std::string const &id) : rpx::RemoteObject(id) {
    registerRemote("echo", &TestObject::echo);
  };
  ~TestObject() override = default;

  void echo() { std::cout << "Echo!" << std::endl; }
  float add(float f, float ff) {
    auto [ret, success] = callRemote<float>(__FUNCTION__, 0.f, f, ff);
    return ret;
  }
  std::string append(std::string s, std::string app) {
    auto [ret, success] =
        callRemote<std::string>(__FUNCTION__, "default", s, app);
    return ret;
  }
};

int main() {

  /*rpx::communication::TCPClient client;
  client.connect("127.0.0.1.", 12345);
  rpx::RemoteManager::addCommunication(&client);*/
  rpx::communication::PipeNode node("/tmp/rpx1");
  node.connectTo("/tmp/rpx0");
  rpx::RemoteManager::addCommunication(&node);

  std::cout << "Remote client connected" << std::endl;
  TestObject obj("fID");

  float f = 3.5f, ff = 4.8f;
  std::cout << "Calling add -> " << f << " + " << ff << " = "
            << obj.add(3.5f, 4.8f) << std::endl;
  std::string s = "Hello ", app = "World";
  std::cout << "Calling append -> " << s << "and " << app << " = "
            << obj.append(s, app) << std::endl;

  std::cin.get();
  return EXIT_SUCCESS;
}
