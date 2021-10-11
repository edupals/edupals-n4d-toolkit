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
#include <user.hpp>

#include <iostream>

using namespace edupals;
using namespace std;

int main(int argc,char* argv[])
{
    
    n4d::Client client;
    
    system::User me = system::User::me();
    
    clog<<"looking for a key for "<<me.name<<endl;
    n4d::auth::Key key = n4d::auth::Key::user_key(me.name);
    
    if (key) {
        clog<<"Found a local key"<<endl;
        
        //build a fake ticket
        string ticketstr = "N4DTKV2 https://127.0.0.1:9779 "+me.name+" "+key.value;
        
        n4d::Ticket ticket(ticketstr);
        
        if (ticket) {
            clog<<"ticket parsed successfully"<<endl;
            clog<<"* "<<ticket.get_address()<<endl;
            clog<<"* "<<ticket.get_credential().user<<endl;
            clog<<"* "<<ticket.get_credential().key.value<<endl;
            
            client = n4d::Client(ticket);
        }
    }
    
    clog<<"valid: "<<client.is_user_valid({"patata"})<<endl;
    //client.set_flags(n4d::Option::Verbose);
    
    variant::Variant value = client.get_variable("patata",true);
    
    clog<<"patata:"<<value["value"]<<endl;
    clog<<"attribs:"<<endl;
    for (string k:value.keys()) {
        if (k!="value") {
            clog<<"    "<<k<<":"<<value[k]<<endl;
        }
    }
    
    client.version();
    client.get_methods();
    
    /*
    for (int n=0;n<1000;n++) {
        value = client.get_variable("patata");
    }
    */
    
    /*
    clog<<"Checking server...";
    
    if (!client.running()) {
        
        clog<<"fail"<<endl;
        return 1;
    }
    clog<<"ok"<<endl;
    
    
    clog<<"Method list:"<<endl;
    map<string,vector<string> > plugins = client.get_methods();
    
    for (auto plugin: plugins) {
        clog<<"Plugin: "<<plugin.first<<endl;
        
        for (string method : plugins[plugin.first]) {
            clog<<"    "<<method<<endl;
        }
        
        clog<<endl;
    }
    
    variant::Variant value = client.call("VariablesManager","listvars");
    
    clog<<"response->"<<value<<endl;
    */
    return 0;
}
