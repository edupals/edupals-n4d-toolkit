# edupals-n4d-toolkit
A C++ toolkit based on edupals-base-toolkit for dealing with lliurex n4d (xml-rpc) connections

Depends on Variant class from edupals-base-toolkit.

## Example cmake:
```
cmake_minimum_required(VERSION 3.0)

project(n4d-test)

find_package(EdupalsBase REQUIRED)
find_package(EdupalsN4D REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main Edupals::Base Edupals::N4D)
```

## Example usage:

```
//from edupals-n4d
#include <n4d.hpp>

//from edupals-base 
#include <variant.hpp>

#include <iostream>

using namespace edupals;
using namespace std;

int main(int argc,char* argv[])
{
    
    n4d::Client client("https://server",9779);
    
    vector<variant::Variant> params = {"1",2,false};
    
    variant::Variant value = client.call("PluginName","method_name",params);
    
    clog<<"response->"<<value<<endl;
    
    return 0;
}
```

Default constructor creates an anonymous client to localhost and port 9779:
```
n4d::Client client;
```

Client with an user&password credential:
```
n4d::Client client(EDUPALS_N4D_DEFAULT_URL,"foouser","foopass");
```

Gets a ticket from a server and create another Client with that ticket
```
n4d::Client remote("https://server:9779","foouser","foopass");
n4d::auth::Ticket ticket = remote.get_ticket();

n4d::Client client(ticket);
```

Handy method that creates a client with a local ticket using current process user
```
n4d::Client client = n4d::Client::from_local_ticket();
```

