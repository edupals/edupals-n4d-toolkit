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
#include <rapidxml/rapidxml.hpp>

#include <iostream>
#include <cstring>

using namespace edupals;
using namespace edupals::variant;
using namespace edupals::n4d;

using namespace rapidxml;

using namespace std;

class CurlFactory
{
    public:
        
    bool ready;
    
    CurlFactory()
    {
        ready = (curl_global_init(CURL_GLOBAL_ALL)==0);
    }
    
    ~CurlFactory()
    {
        if (ready) {
            curl_global_cleanup();
        }
    }
};

static CurlFactory curl_instance;


Client::Client(string address,int port)
{
    this->address=address;
    this->port=port;
}

Client::Client(string address,int port,string user,string password)
{
    this->address=address;
    this->port=port;
    
    credential=auth::Credential(user,password);
}

Client::Client(string address,int port,string key)
{
    this->address=address;
    this->port=port;
    
    credential=auth::Credential(key);
}

Variant parse_value(rapidxml::xml_node<>* node_value)
{
    Variant ret;
    
    rapidxml::xml_node<>* node = node_value->first_node();
    
    //nothing?
    if (!node) {
        return ret;
    }
    
    string name = node->name();
    string value = node->value();
    
    if (name=="int" or name=="i4") {
        ret=std::stoi(value);
    }
    
    if (name=="double") {
        ret=std::stod(value);
    }
    
    if (name=="boolean") {
        int b=std::stoi(value);
        ret=(b==1);
    }
    
    if (name=="string") {
        ret=value;
    }
    
    // datetime and base64 not fully supported, return as string
    if (name=="dateTime.iso8601") {
        ret=value;
    }
    
    if (name=="base64") {
        ret=value;
    }
    
    if (name=="array") {
        rapidxml::xml_node<>* node_data = node->first_node("data");
        
        if (node_data) {
            ret=Variant::create_array(0);
            rapidxml::xml_node<>* node_value = node_data->first_node("value");
            
            while(node_value) {
                ret.append(parse_value(node_value));
                node_value = node_value->next_sibling("value");
            }
        }
    }
    
    if (name=="struct") {
        rapidxml::xml_node<>* node_member = node->first_node("member");
        ret=Variant::create_struct();
        
        while (node_member) {
            rapidxml::xml_node<>* node_name = node_member->first_node("name");
            rapidxml::xml_node<>* node_value = node_member->first_node("value");
            
            if (node_name and node_value) {
                ret[node_name->value()]=parse_value(node_value);
            }
            
            node_member=node_member->next_sibling("member");
        }
    }
    
    return ret;
}

Variant Client::call(string plugin,string method)
{
    vector<Variant> params;
    return call(plugin,method,params,this->credential);
}

Variant Client::call(string plugin,string method,vector<Variant> params)
{
    return call(plugin,method,params,this->credential);
}

Variant Client::call(string plugin,string method,vector<Variant> params, auth::Credential credential)
{
    string out;
    string in;
    
    Variant ret;
    
    create_request(plugin,method,params,credential,out);
    
    rpc_call(in,out);
    
    xml_document<> doc; 

    char* memxml=new char[in.size()+1];
    std::memcpy(memxml,in.c_str(),in.size()+1);
    
    try {
        doc.parse<0>(memxml);
    }
    catch (rapidxml::parse_error& ex) {
        delete [] memxml;
        throw exception::BadXML(ex.what());
    }
    
    delete [] memxml;
    
    rapidxml::xml_node<>* node_method = doc.first_node("methodResponse");
    
    if (!node_method) {
        throw exception::BadXML("missing methodResponse node");
    }
    
    rapidxml::xml_node<>* node_params = node_method->first_node();
    
    if (!node_params) {
        throw exception::BadXML("missing params or fault node");
    }
    
    string name=node_params->name();
    
    if (name=="fault") {
        throw exception::FaultRPC("");
    }
    
    if (name=="params") {
        rapidxml::xml_node<>* node_param=node_params->first_node("param");
        
        if (node_param) {
            
            rapidxml::xml_node<>* node_value=node_param->first_node("value");
            
            if (node_value) {
                ret=parse_value(node_value);
            }
        }
    }
    
    return ret;
}

Client::~Client()
{

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
    
    if (!curl_instance.ready) {
        throw exception::CurlError("curl_global_init",0);
    }
    
    curl = curl_easy_init();
    if(!curl) {
        throw exception::CurlError("curl_easy_init",0);
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, address.c_str());
    curl_easy_setopt(curl, CURLOPT_PORT, port);
    
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,out.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,&in);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,response_cb);
    
    res=curl_easy_perform(curl);
    
    if (res!=0) {
        throw exception::CurlError("curl_easy_perform",res);
    }
    
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

bool Client::validate_user(string name,string value)
{
    return false;
}

map<string,vector<string> > Client::get_methods()
{
    map<string, vector<string> > plugins;
    
    return plugins;
}
