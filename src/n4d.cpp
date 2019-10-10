/*
 * Copyright (C) 2019 Edupals project
 *
 * Author:
 *  Enrique Medina Gremaldos <quiqueiii@gmail.com>
 *
 * Source:
 *  https://github.com/edupals/edupals-n4d-toolkit
 *
 * This file is a part of edupals-n4d-toolkit.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#include <n4d.hpp>

using namespace edupals;
using namespace edupals::variant;
using namespace edupals::n4d;

using namespace std;

Client::Client()
{
}

Client::Client(string address,int port)
{
}


Client::Client(string address,int port,string user,string password)
{
}

Client::Client(string address,int port,string key)
{
}

Variant Client::call(string plugin,string method)
{
}

Variant Client::call(string plugin,string method,vector<Variant> args)
{
}

Variant Client::call(string plugin,string method,vector<Variant> args, auth::Credential credential)
{
}

Client::~Client()
{
}