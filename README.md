# RPX

Remote process call framework using libmsgpack (serialization based on https://github.com/msgpack/msgpack-c/tree/cpp-3.3.0)

## Usage

Class needs to inherit rpx::RemoteObject
```cpp
class TestObject : public rpx::RemoteObject {
};

```
To provide functions to the framework
```cpp
registerRemote("testfunction", &TestObject::testfunction);
```
To call remote functions
```cpp
float testfunction(float f, float ff)
{
    auto [ret, success] = callRemote<float>(__FUNCTION__, 0.f, f, ff);
    return ret;
}
```
## Call syntax
Arg|Desc
---|:---
RETURNTYPE| Type of return value
FUNCTIONNAME | Name of the function as string (_\_FUNCTION__ macro possible)
DEFAULTRETURN | Default return value
ARGS... | Arguments for function call

```cpp
auto [ret, success] = callRemote<RETURNTYPE>(FUNCTIONNAME, DEFAULTRETURN, ARGS...)
```

void function
Arg|Desc
---|:---
FUNCTIONNAME | Name of the function
ARGS... | Arguments for function call
```cpp
bool success = callRemote<void>(FUNCTIONNAME, ARGS...)
```
## Connection

You can use library provided TCP server and client or use the ICommunication interface to implement your own connection.

### TCP client server
```cpp
rpx::communication::TCPServer server;
server.listen(12345);
rpx::RemoteManager::addCommunication(&server);
```

```cpp
rpx::communication::TCPClient client;
client.connect("127.0.0.1.", 12345);
rpx::RemoteManager::addCommunication(&client);
```
For further usage checkout the sample.