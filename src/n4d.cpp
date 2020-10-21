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
#include <iomanip>
#include <cstring>
#include <sstream>

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

bool auth::Key::valid()
{
    // based on current N4D ticket generation method
    if (value.size()==50) {
        for (char c:value) {
            if ( (c>='0' and c<='9') or
                (c>='a' and c<='z') or
                (c>='A' and c<='Z')) {
                continue;
            }
            
            return false;
        }
        return true;
    }
    
    return false;
}

Client::Client(string address,int port)
{
    this->address=address;
    this->port=port;
    this->flags=Option::None;
}

Client::Client(string address,int port,string user,string password)
{
    this->address=address;
    this->port=port;
    this->flags=Option::None;
    
    credential=auth::Credential(user,password);
}

Client::Client(string address,int port,string user,auth::Key key)
{
    this->address=address;
    this->port=port;
    this->flags=Option::None;
    
    credential=auth::Credential(user,key);
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
        stringstream in;
        in.imbue(std::locale("C"));
        in.str(value);
        int ivalue;
        in>>ivalue;
        
        ret=ivalue;
    }
    
    if (name=="double") {
        stringstream in;
        in.imbue(std::locale("C"));
        in.str(value);
        double dvalue;
        in>>dvalue;
        ret=dvalue;
    }
    
    if (name=="boolean") {
        stringstream in;
        in.imbue(std::locale("C"));
        in.str(value);
        int ivalue;
        
        in>>ivalue;
        
        ret=(ivalue==1);
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

Variant Client::rpc_call(string method,vector<Variant> params)
{
    stringstream out;
    stringstream in;
    
    out.imbue(std::locale("C"));
    out<<std::setprecision(10)<<std::fixed;
    
    in.imbue(std::locale("C"));
    in<<std::setprecision(10)<<std::fixed;
    
    Variant ret;
    
    create_request(method,params,out);
    
    if (flags & Option::Verbose) {
        clog<<"**** OUT ****"<<endl;
        clog<<out.str()<<endl;
        clog<<"*************"<<endl;
    }
    
    post(in,out);
    
    if (flags & Option::Verbose) {
        clog<<"****  IN  ****"<<endl;
        clog<<in.str()<<endl;
        clog<<"**************"<<endl;
    }
    
    xml_document<> doc;
    
    /*
        I guess, It depends on compiler but from the theory up to three
        input string copies are hold into memory
    */
    string incoming=in.str();

    char* memxml=new char[incoming.size()+1];
    std::memcpy(memxml,incoming.c_str(),incoming.size()+1);
    
    try {
        doc.parse<0>(memxml);
    }
    catch (rapidxml::parse_error& ex) {
        delete [] memxml;
        throw exception::ServerError(0,ex.what());
    }
    
    rapidxml::xml_node<>* node_method = doc.first_node("methodResponse");
    
    if (!node_method) {
        delete [] memxml;
        throw exception::ServerError(0,"xml-rpc: missing methodResponse node");
    }
    
    rapidxml::xml_node<>* node_params = node_method->first_node();
    
    if (!node_params) {
        delete [] memxml;
        throw exception::ServerError(0,"xml-rpc: missing params or fault node");
    }
    
    string name=node_params->name();
    
    if (name=="fault") {
        delete [] memxml;
        //TODO: Add fault string
        throw exception::ServerError(0,"xml-rpc: fault response not supported");
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
    
    if (ret.none()) {
        throw exception::ServerError(0,"xml-rpc: missing return value");
        delete [] memxml;
    }
    
    delete [] memxml;
    
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

Variant Client::call(string name,string method,vector<Variant> params, auth::Credential credential)
{
    Variant response;
    
    //Build N4D header
    vector<Variant> full_params;
    
    switch (credential.type) {
        case auth::Type::Anonymous:
            full_params.push_back("");
        break;
        
        case auth::Type::Password:
            full_params.push_back({credential.user, credential.password});
        break;
        
        case auth::Type::Key:
            full_params.push_back({credential.user, credential.key.value});
        break;
    }
    
    // push plugin name
    full_params.push_back(name);
    
    // push method params
    for (Variant& param : params) {
        full_params.push_back(param);
    }
    
    response=rpc_call(method,full_params);
    
    if (validate_response(response)) {
        
        int status = response["status"].get_int32();
        
        switch (status) {
            case ErrorCode::UnknownClass:
                throw exception::UnknownClass(name);
            break;
            
            case ErrorCode::UnknownMethod:
                throw exception::UnknownMethod(name,method);
            break;
            
            case ErrorCode::UserNotAllowed:
                throw exception::UserNotAllowed(credential.user,name,method);
            break;
            
            case ErrorCode::AuthenticationFailed:
                throw exception::AuthenticationFailed(credential.user);
            break;
            
            case ErrorCode::InvalidResponse:
                throw exception::InvalidMethodResponse(name,method);
            break;
            
            case ErrorCode::InvalidArguments:
                throw exception::InvalidArguments(name,method);
            break;
            
            case ErrorCode::CallFailed:
                throw exception::CallFailed(name,method,-1);
            break;
            
            case ErrorCode::CallSuccessful:
                return response["return"];
            break;
            
            default:
                throw exception::UnknownCode(name,method,status);
        }
        
    }
    else {
        throw exception::InvalidServerResponse(address);
    }
    
}

Client::~Client()
{

}

size_t response_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    stringstream* in=static_cast<stringstream*>(userdata);
    
    for (size_t n=0;n<nmemb;n++) {
        in->put(ptr[n]);
    }
    
    return nmemb;
}

void Client::post(stringstream& in,stringstream& out)
{
    CURL *curl;
    CURLcode res;
    
    if (!curl_instance.ready) {
        throw exception::ServerError(0,"curl_global_init");
    }
    
    curl = curl_easy_init();
    if(!curl) {
        throw exception::ServerError(0,"curl_easy_init");
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, address.c_str());
    curl_easy_setopt(curl, CURLOPT_PORT, port);
    
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    string data=out.str();
    
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,data.c_str());
    
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,&in);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,response_cb);
    
    res=curl_easy_perform(curl);
    
    if (res!=0) {
        curl_easy_cleanup(curl);
        throw exception::ServerError(res,"curl_easy_perform");
    }
    
    curl_easy_cleanup(curl);
}

void Client::create_value(Variant value, stringstream& out)
{
    
    out<<"<value>";
    switch (value.type()) {
        
        case variant::Type::Boolean:
            out<<"<boolean>";
            if (value.get_boolean()) {
                out<<"1";
            }
            else {
                out<<"0";
            }
            out<<"</boolean>";
        break;
        
        case variant::Type::Int32:
            out<<"<int>";
            out<<value.get_int32();
            out<<"</int>";
        break;
        
        // floats are encoded as doubles (losing precission)
        case variant::Type::Float:
            out<<"<double>";
            out<<value.get_float();
            out<<"</double>";
        break;
        
        case variant::Type::Double:
            out<<"<double>";
            out<<value.get_double();
            out<<"</double>";
        break;
        
        case variant::Type::String:
            out<<"<string>";
            out<<value.get_string();
            out<<"</string>";
        break;
        
        case variant::Type::Array:
            out<<"<array>";
            out<<"<data>";
                for(size_t n=0;n<value.count();n++) {
                    create_value(value[n],out);
                }
            out<<"</data>";
            out<<"</array>";
        break;
        
        case variant::Type::Struct:
            out<<"<struct>";
            
            for (string& key: value.keys()) {
                out<<"<member>";
                out<<"<name>";
                out<<key;
                out<<"</name>";
                
                create_value(value[key],out);
                
                out<<"</member>";
            }
            
            out<<"</struct>";
        break;
    }
    out<<"</value>";
}

void Client::create_request(string method,vector<Variant> params,stringstream& out)
{
    
    out<<"<?xml version=\"1.0\"?>";
    out<<"<methodCall>";
        out<<"<methodName>";
            out<<method;
        out<<"</methodName>";
        out<<"<params>";
            for (Variant& param : params) {
                out<<"<param>";
                    create_value(param,out);
                out<<"</param>";
            }
        out<<"</params>";
    out<<"</methodCall>";
}

bool Client::validate_response(variant::Variant response)
{
    Variant v = response/"msg"/variant::Type::String;
    if (v.none()) {
        return false;
    }
    
    v = response/"status"/variant::Type::Int32;
    if (v.none()) {
        return false;
    }
    
    v = response/"return";
    if (v.none()) {
        return false;
    }
    
    return true;
}

bool Client::validate_user(string name,string password)
{
    vector<Variant> params ;
    
    params.push_back(name);
    params.push_back(password);
    
    Variant value = rpc_call("validate_user",params);
    
    try {
        return value[0].get_boolean();
    }
    catch (std::exception& ex) {
        throw exception::ServerError(0,"ToDO");
    }
    
    return false;
}

vector<string> Client::get_groups(string name,string password)
{
    vector<string> groups;
    vector<Variant> params;
    
    params.push_back(name);
    params.push_back(password);
    
    Variant value = rpc_call("validate_user",params);
    
    try {
        int num=value[1].count();
        
        for (int n=0;n<num;n++) {
            groups.push_back(value[1][n].get_string());
        }
    }
    catch (std::exception& ex) {
        throw exception::ServerError(0,"ToDo");
    }
    
    return groups;
}

map<string,vector<string> > Client::get_methods()
{
    map<string, vector<string> > plugins;
    vector<Variant> params;
    
    Variant value = rpc_call("get_sorted_methods",params);
    
    try {
        for (string& key : value.keys()) {
            
            for (size_t n=0;n<value[key].count();n++) {
                
                plugins[key].push_back(value[key][n].get_string());
            }
        }
    }
    catch (std::exception& ex) {
        throw exception::ServerError(0,"ToDo");
    }
    
    return plugins;
}

bool Client::running()
{
    bool status=true;
    
    try {
        auto none=get_methods();
    }
    catch (std::exception& ex) {
        status=false;
    }
    
    return status;
}

void Client::set_flags(int flags)
{
    this->flags=flags;
}

int Client::get_flags()
{
    return flags;
}
