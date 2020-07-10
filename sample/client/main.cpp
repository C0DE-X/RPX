 #include <iostream>
 #include <string>

 #include <rpx/RemoteManager.h>
 #include <rpx/RemoteObject.h>

class TestObject : public rpx::RemoteObject {
public:
  TestObject(std::string const &id) : rpx::RemoteObject(id) {
    registerRemote("echo", &TestObject::echo);
  };
  ~TestObject() override = default;

  void echo(){ std::cout<<"Echo!"<<std::endl; } 
  float add(float f, float ff) {
      bool success = false;
      return callRemote<float>(__FUNCTION__, success, 0.f, f, ff);
  }
  std::string append(std::string s, std::string app) { 
      bool success = false;
      return callRemote<std::string>(__FUNCTION__, success, "default", s, app);
      }
};

int main()
{
  rpx::RemoteManager::connect("127.0.0.1.", 12345);
  std::cout<< "Remote client connected" << std::endl;
  TestObject obj("fID");

  float f = 3.5f, ff = 4.8f;
  std::cout<< "Calling add -> " << f << " + " << ff << " = " << obj.add(3.5f, 4.8f) << std::endl;
  std::string s = "Hello ", app = "World";
  std::cout<< "Calling append -> " << s << " and " << app << " = " << obj.append(s, app) << std::endl;

  std::cin.get();
  return EXIT_SUCCESS;
}  
