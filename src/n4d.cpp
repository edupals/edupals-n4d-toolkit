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

#include <curl/curl.h>

#include <iostream>

using namespace edupals;
using namespace edupals::variant;
using namespace edupals::n4d;

using namespace std;

int Client::curl_counter=0;

Client::Client()
{
    if (Client::curl_counter==0) {
        if (curl_global_init(CURL_GLOBAL_ALL)==0) {
            Client::curl_counter++;
            clog<<"Initialized curl"<<endl;
        }
        else {
            cerr<<"Failed to load curl"<<endl;
        }
    }
}

Client::Client(string address,int port) : Client()
{
    this->address=address;
    this->port=port;
}

Client::Client(string address,int port,string user,string password) : Client()
{
    this->address=address;
    this->port=port;
    
    credential=auth::Credential(user,password);
}

Client::Client(string address,int port,string key) : Client()
{
    this->address=address;
    this->port=port;
    
    credential=auth::Credential(key);
}

Variant Client::call(string plugin,string method)
{
    string out;
    string in;
    
    vector<Variant> params;
    
    create_request(plugin,method,params,credential,out);
    
    clog<<"call 0:"<<endl;
    clog<<out<<endl;
    
    rpc_call(in,out);
    
    clog<<"response:"<<endl;
    clog<<in<<endl;
}

Variant Client::call(string plugin,string method,vector<Variant> params)
{
    string out;
    string in;
    
    create_request(plugin,method,params,credential,out);
    
    clog<<"call 1:"<<endl;
    clog<<out<<endl;
    
    rpc_call(in,out);
    
    clog<<"response:"<<endl;
    clog<<in<<endl;
}

Variant Client::call(string plugin,string method,vector<Variant> params, auth::Credential credential)
{
    string out;
    
    create_request(plugin,method,params,credential,out);
}

Client::~Client()
{
    Client::curl_counter--;
    
    if (Client::curl_counter==0) {
        clog<<"destroying curl"<<endl;
        curl_global_cleanup();
    }
}

size_t response_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    string* in=static_cast<string *>(userdata);
    
    for (size_t n=0;n<nmemb;n++) {
        in->append(1,ptr[n]);
    }
}
 
void Client::rpc_call(string& in,string& out)
{
    CURL *curl;
    CURLcode res;
    
    string url="https://"+address;
    
    curl = curl_easy_init();
    if(!curl) {
        //TODO: throw a exception
        cerr<<"failed to create curl ctx"<<endl;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_PORT, port);
    
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,out.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,&in);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,response_cb);
    
    res=curl_easy_perform(curl);
    
    //TODO: throw a exception
    clog<<"status:"<<res<<endl;
    
    curl_easy_cleanup(curl);
}

void Client::create_value(Variant value, string& out)
{
    out.append("<value>");
    switch (value.type()) {
        
        case variant::Type::Boolean:
            out.append("<boolean>");
            if (value.get_boolean()) {
                out.append("1");
            }
            else {
                out.append("0");
            }
            out.append("</boolean>");
        break;
        
        case variant::Type::Int32:
            out.append("<int>");
            out.append(std::to_string(value.get_int32()));
            out.append("</int>");
        break;
        
        // floats are encoded as doubles (losing precission)
        case variant::Type::Float:
            out.append("<double>");
            out.append(std::to_string(value.get_float()));
            out.append("</double>");
        break;
        
        case variant::Type::Double:
            out.append("<double>");
            out.append(std::to_string(value.get_double()));
            out.append("</double>");
        break;
        
        case variant::Type::String:
            out.append("<string>");
            out.append(value.get_string());
            out.append("</string>");
        break;
        
        case variant::Type::Array:
            out.append("<array>");
            out.append("<data>");
                for(size_t n=0;n<value.count();n++) {
                    create_value(value[n],out);
                }
            out.append("</data>");
            out.append("</array>");
        break;
        
        case variant::Type::Struct:
            out.append("<struct>");
            
            for (string& key: value.keys()) {
                out.append("<member>");
                out.append("<name>");
                out.append(key);
                out.append("</name>");
                
                create_value(value[key],out);
                
                out.append("</member>");
            }
            
            out.append("</struct>");
        break;
    }
    out.append("</value>");
}

void Client::create_request(string plugin,string method,vector<Variant> params,auth::Credential credential,string& out)
{
    out.clear();
    
    out.append("<?xml version=\"1.0\"?>");
    out.append("<methodCall>");
        out.append("<methodName>");
            out.append(method);
        out.append("</methodName>");
    
            out.append("<params>");
                //credentials
                out.append("<param>");
                    switch (credential.type) {
                        case auth::Type::Anonymous:
                            out.append("<value><string></string></value>");
                        break;
                        
                        case auth::Type::Password:
                            out.append("<value><array><data>");
                            out.append("<value><string>");
                            out.append(credential.user);
                            out.append("</string></value>");
                            out.append("<value><string>");
                            out.append(credential.password);
                            out.append("</string></value>");
                            out.append("</data></array></value>");
                        break;
                        
                        case auth::Type::Key:
                            out.append("<value><string>");
                            out.append(credential.key);
                            out.append("</string></value>");
                        break;
                    }
                out.append("</param>");
                
                //plugin
                out.append("<param>");
                out.append("<value><string>");
                    out.append(plugin);
                out.append("</string></value>");
                out.append("</param>");
                
                //parameters
                for (Variant param: params) {
                    out.append("<param>");
                        create_value(param,out);
                    out.append("</param>");
                }
                
            out.append("</params>");
        out.append("</methodCall>");
}
