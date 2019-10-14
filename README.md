# edupals-n4d-toolkit
A C++ toolkit based on edupals-base-toolkit for dealing with lliurex n4d (xml-rpc) connections

Depends on edupals-base-toolkit.

## Example cmake:
```
cmake_minimum_required(VERSION 3.0)

project(n4d-test)

find_package(EdupalsBase REQUIRED)
find_package(EdupalsN4D REQUIRED)

include_directories(${EDUPALS_BASE_INCLUDE_DIRS} ${EDUPALS_N4D_INCLUDE_DIRS})

#testing application
add_executable(main main.cpp)
target_link_libraries(main edupals-base edupals-n4d)
```

## Example usage:

```
#include <n4d.hpp>

#include <iostream>

using namespace edupals;
using namespace std;

int main(int argc,char* argv[])
{
    
    n4d::Client client("https://localhost",9779);
    
    variant::Variant value = client.call("VariablesManager","listvars");
    
    clog<<"response->"<<value<<endl;
    
    return 0;
}
```
