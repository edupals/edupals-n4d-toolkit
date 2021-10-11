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
#include <token.hpp>

#include <curl/curl.h>
#include <rapidxml/rapidxml.hpp>

#include <iostream>
#include <iomanip>
#include <cstring>
#include <sstream>
#include <fstream>

using namespace edupals;
using namespace edupals::variant;
using namespace edupals::parser;
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

//TODO: check about thread safety
CurlFactory curl_instance;

bool auth::Key::valid()
{
    // based on current N4D ticket generation method
    if (value.size()==50) {
        for (char c:value) {
            if (token::is_num(c) or
                token::is_alpha_lower(c) or
                token::is_alpha_upper(c)) {
                continue;
            }
            
            return false;
        }
        return true;
    }
    
    return false;
}

auth::Key auth::Key::master_key()
{
    string credential_path="/etc/n4d/key";
    ifstream file(credential_path);
    
    if(!file) {
        return auth::Key();
    }
    
    string data;
    std::getline(file,data);
    
    file.close();
    
    return auth::Key(data);
}

auth::Key auth::Key::user_key(string user)
{
    string credential_path="/run/n4d/tickets/"+user;
    ifstream file(credential_path);
    
    if(!file) {
        return auth::Key();
    }
    
    string data;
    std::getline(file,data);
    
    file.close();
    
    return auth::Key(data);
}

Variant auth::Credential::get()
{
    switch (type) {
        case auth::Type::Anonymous:
            return "";
        break;
        
        case auth::Type::Password:
            return {user, password};
        break;
        
        case auth::Type::Key:
            return {user, key.value};
        break;
        
        case auth::Type::MasterKey:
            return key.value;
        break;
    }
    
    // default to anonymous
    return "";
}

static vector<string> split(string in)
{
    vector<string> ret;
    string tmp;
    
    for (char c:in) {
        
        if (c==' ') {
            ret.push_back(tmp);
            tmp="";
        }
        else {
            tmp=tmp+c;
        }
    }
    
    if (tmp.size()>0) {
        ret.push_back(tmp);
    }
    
    return ret;
}

Ticket::Ticket(string address,auth::Credential credential)
{
    this->address=address;
    this->credential=credential;
    
    // should we perform a deeper validation?
    _valid = credential.key.valid();
}

Ticket::Ticket(string ticket)
{
    _valid=false;
    
    vector<string> tmp = split(ticket);
    
    if (tmp.size()>=4) {
        if (tmp[0]=="N4DTKV2") {
            // check uri?
            address=tmp[1];
            credential = auth::Credential(tmp[2],auth::Key(tmp[3]));
            
            if (credential.key) {
                _valid=true;
            }
        }
    }
}

string Ticket::to_string()
{
    if (_valid) {
        return "N4DTKV2 "+address+" "+credential.user+" "+credential.key.value;
    }
    else {
        return "";
    }
}

Client::Client(string address)
{
    this->address=address;
    this->flags=Option::None;
}

Client::Client(string address,int port) : Client(address)
{
}

Client::Client(string address,string user,string password)
{
    this->address=address;
    this->flags=Option::None;
    
    this->credential=auth::Credential(user,password);
}

Client::Client(string address,int port,string user,string password) : Client(address,user,password)
{
}

Client::Client(string address,string user,auth::Key key)
{
    this->address=address;
    this->flags=Option::None;
    
    this->credential=auth::Credential(user,key);
}

Client::Client(string address,int port,string user,auth::Key key) : Client(address,user,key)
{
}

Client::Client(string address, auth::Credential credential)
{
    this->address=address;
    this->credential=credential;
    this->flags=Option::None;
}

Client::Client(Ticket ticket)
{
    this->address=ticket.get_address();
    this->credential=ticket.get_credential();
    this->flags=Option::None;
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

Variant Client::call(string name,string method)
{
    vector<Variant> params;
    return call(name,method,params);
}

Variant Client::call(string name,string method,vector<Variant> params)
{
    Variant response;
    
    // Build N4D header
    vector<Variant> full_params;
    
    // push credentials
    full_params.push_back(credential.get());
    
    // push plugin name
    full_params.push_back(name);
    
    // push method params
    for (Variant& param : params) {
        full_params.push_back(param);
    }
    
    response=rpc_call(method,full_params);
    
    return validate(response,name,method);
}

Variant Client::call(string name,string method,vector<Variant> params, auth::Credential credential)
{
    return call(name,method,params);
}

Variant Client::builtin_call(string method,vector<Variant> params)
{
    Variant value = rpc_call(method,params);
    return validate(value,"N4D",method);
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
    //curl_easy_setopt(curl, CURLOPT_PORT, port);
    
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
        
        case variant::Type::None:
            out<<"<nil/>";
        break;
            
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

bool Client::validate_format(variant::Variant response)
{
    Variant v;
    
    try {
        v = response/"msg"/variant::Type::String;
        v = response/"status"/variant::Type::Int32;
        
        switch (v.get_int32()) {
            case ErrorCode::CallFailed:
                v = response/"error_code"/variant::Type::Int32;
            break;
            
            case ErrorCode::UnhandledError:
                v = response/"traceback"/variant::Type::String;
            break;
        }
        
        v = response/"return";
    }
    catch (variant::exception::NotFound& e) {
        return false;
    }
    
    return true;
}

Variant Client::validate(variant::Variant response,string name,string method)
{
    if (validate_format(response)) {
        
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
            
            case ErrorCode::UnhandledError:
                throw exception::UnhandledError(name,method,response["traceback"]);
            break;
            
            case ErrorCode::InvalidArguments:
                throw exception::InvalidArguments(name,method);
            break;
            
            case ErrorCode::CallFailed:
                throw exception::CallFailed(name,method,
                                            response["error_code"].get_int32(),
                                            response["msg"].get_string());
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

bool Client::validate_user(string name,string password)
{
    auth::Type type = credential.type;
    vector<Variant> args;
    
    if (type==auth::Type::Password) {
        args.push_back(credential.user);
        args.push_back(credential.password);
    }
    else {
        if (type==auth::Type::Key) {
            args.push_back(credential.user);
            args.push_back(credential.key.value);
        }
        else {
            throw exception::InvalidCredential();
        }
    }
    
    Variant value = builtin_call("validate_user",args);
    
    try {
        Variant response = value / 0 / variant::Type::Boolean;
        return response.get_boolean();
    }
    catch (variant::exception::NotFound& e) {
        throw exception::InvalidBuiltInResponse("validate_user","Expected boolean response");
    }
    
}

bool Client::validate_auth()
{
    Variant value = builtin_call("validate_auth",{credential.get()});
    
    try {
        Variant response = value / 0 / variant::Type::Boolean;
        return response.get_boolean();
    }
    catch (variant::exception::NotFound& e) {
        throw exception::InvalidBuiltInResponse("validate_auth","Expected boolean response");
    }
    catch (exception::AuthenticationFailed& e) {
        /* NOTE 
            This should be fixed in server on future releases
         */
        return false;
    }
}

bool Client::is_user_valid(vector<string> groups)
{
    auth::Type type = credential.type;
    
    if (type==auth::Type::Password or type==auth::Type::Key) {
        Variant args = credential.get();
        Variant vgroups = Variant::create_array(0);
        for (string& g : groups) {
            vgroups.append(g);
        }
        Variant value = builtin_call("is_user_valid",{args[0],args[1],vgroups});
        
        return value.to_boolean();
    }
    else {
        throw exception::InvalidCredential();
    }
}

vector<string> Client::get_groups(string name,string password)
{
    return get_groups();
}

vector<string> Client::get_groups()
{
    auth::Type type = credential.type;
    vector<Variant> args;
    
    if (type==auth::Type::Password) {
        args.push_back(credential.user);
        args.push_back(credential.password);
    }
    else {
        if (type==auth::Type::Key) {
            args.push_back(credential.user);
            args.push_back(credential.key.value);
        }
        else {
            throw exception::InvalidCredential();
        }
    }
    
    Variant value = builtin_call("validate_user",args);
    
    try {
        Variant list = value / 1 / variant::Type::Array;
        vector<string> groups;
        
        for (int n=0;n<list.count();n++) {
            Variant v = list[n];
            
            if (v.is_string()) {
                groups.push_back(v.get_string());
            }
        }
        
        return groups;
    }
    catch (variant::exception::NotFound& e) {
        throw exception::InvalidBuiltInResponse("get_groups","Exepcted array response");
    }
    
}

map<string,vector<string> > Client::get_methods()
{
    map<string, vector<string> > plugins;
    vector<Variant> params;
    
    Variant value = builtin_call("get_methods",params);
    
    try {
        for (string& key : value.keys()) {
            
            for (string& mkey : value[key].keys()) {
                
                plugins[key].push_back(mkey);
            }
        }
    }
    catch (std::exception& ex) {
        throw exception::InvalidBuiltInResponse("get_methods","Failed to parse response");
    }
    
    return plugins;
}

Ticket Client::create_ticket()
{
    auth::Type type = credential.type;
    
    if (type==auth::Type::Password or type==auth::Type::Key) {
        Variant value = builtin_call("create_ticket",{credential.user});
        
        auth::Key ticket = auth::Key::user_key(credential.user);
        
        return Ticket(address,auth::Credential(credential.user,ticket));
    }
    else {
        throw exception::InvalidCredential();
    }
    
}

Ticket Client::get_ticket()
{
    auth::Type type = credential.type;
    
    if (type==auth::Type::Password) {
        Variant value = builtin_call("get_ticket",{credential.user,credential.password});
        
        if (value.is_string()) {
            return Ticket(address,auth::Credential(credential.user,auth::Key(value.get_string())));
        }
        else {
            throw exception::InvalidBuiltInResponse("get_ticket","Expected string response");
        }
    }
    else {
        throw exception::InvalidCredential();
    }
    
}

void Client::handle_variable_error(VariableErrorCode code, string name)
{
    switch (code) {
        case VariableErrorCode::NotFound:
            throw exception::variable::NotFound(name);
        break;
        
        case VariableErrorCode::Protected:
            throw exception::variable::Protected(name);
        break;
        
        case VariableErrorCode::RemoteServerError:
            throw exception::variable::RemoteServerError();
        break;
        
        case VariableErrorCode::BackupError:
            throw exception::variable::BackupError();
        break;
        
        case VariableErrorCode::RestoreError:
            throw exception::variable::RestoreError();
        break;
        
        case VariableErrorCode::RemoteServerNotConfigured:
            throw exception::variable::RemoteServerNotConfigured();
        break;
    }
}

Variant Client::get_variable(string name, bool attribs)
{
    try {
        Variant response = builtin_call("get_variable",{name,attribs});
    
        return response;
    }
    catch (exception::CallFailed& e) {
        handle_variable_error(static_cast<VariableErrorCode>(e.code),name);
        
        throw;
    }
}

void Client::set_variable(string name,Variant value,Variant attribs)
{
    try {
        Variant response = builtin_call("set_variable",{credential.get(),name,value,attribs});
    }
    catch (exception::CallFailed& e) {
        handle_variable_error(static_cast<VariableErrorCode>(e.code),name);
        
        throw;
    }
}

void Client::delete_variable(string name)
{
    try {
        Variant response = builtin_call("delete_variable",{credential.get(),name});
    }
    catch (exception::CallFailed& e) {
        handle_variable_error(static_cast<VariableErrorCode>(e.code),name);
        
        throw;
    }
}

Variant Client::get_variables(bool attribs)
{
    try {
        Variant response = builtin_call("get_variables",{attribs});
    
        return response;
    }
    catch (exception::CallFailed& e) {
        handle_variable_error(static_cast<VariableErrorCode>(e.code),"");
        
        throw;
    }
}

bool Client::variable_exists(string name)
{
    try {
        Variant response = builtin_call("variable_exists",{name});
    
        return response.get_boolean();
    }
    catch (exception::CallFailed& e) {
        handle_variable_error(static_cast<VariableErrorCode>(e.code),name);
        
        throw;
    }
}

bool Client::running()
{
    bool status=true;
    
    try {
        version();
    }
    catch (std::exception& ex) {
        status=false;
    }
    
    return status;
}

string Client::version()
{
    vector<Variant> args;
    Variant response = builtin_call("get_version",args);
    
    return response.get_string();
}

void Client::set_flags(int flags)
{
    this->flags=flags;
}

int Client::get_flags()
{
    return flags;
}

void Client::set_credential(auth::Credential credential)
{
    this->credential=credential;
}

auth::Credential Client::get_credential()
{
    return credential;
}

string Client::get_address()
{
    return address;
}

void Client::set_address(string address)
{
    this->address=address;
}
