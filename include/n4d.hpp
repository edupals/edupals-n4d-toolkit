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

#include <variant.hpp>

#include <string>
#include <vector>
#include <exception>

namespace edupals
{
    namespace n4d
    {
        namespace exception
        {
            class BadXML : public std::exception
            {
                private:
                std::string msg;
                
                public:
                
                BadXML(std::string info)
                {
                    const std::string header="Bad XML response: ";
                    msg=header+info;
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class FaultRPC : public std::exception
            {
                private:
                std::string msg;
                
                public:
                
                FaultRPC(std::string info)
                {
                    const std::string header="Fault RPC: ";
                    msg=header+info;
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class CurlError : public std::exception
            {
                private:
                std::string msg;
                
                public:
                
                CurlError(std::string info,long int id)
                {
                    const std::string header="Curl error: ";
                    msg=header+info+"("+std::to_string(id)+")";
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
        }
        
        namespace auth
        {
            enum class Type
            {
                None,
                Anonymous,
                Password,
                Key
            };
            
            class Credential
            {
                public:
                std::string user;
                std::string password;
                std::string key;
                
                Type type;
                
                /*!
                 * Anonymous constructor
                */
                Credential()
                {
                    type=Type::Anonymous;
                }
                
                /*!
                 * 
                 */
                Credential(std::string user,std::string password)
                {
                    type=Type::Password;
                    this->user=user;
                    this->password=password;
                }
                
                /*!
                 * Key constructor
                */
                Credential(std::string key)
                {
                    type=Type::Key;
                    this->key=key;
                }
            };
        }
        
        class Client
        {
            protected:
            
            std::string address;
            int port;
            auth::Credential credential;
            
            void rpc_call(std::string& in,std::string& out);
            
            void create_value(variant::Variant param,std::string& out);

            void create_request(std::string plugin,
                                std::string method,
                                std::vector<variant::Variant> params,
                                auth::Credential credential,
                                std::string& out);
            
            public:
            
            /*!
             * Default client to https://localhost 9779 and anonymous credential
            */
            Client(std::string address="https://localhost",int port=9779);
            
            Client(std::string address,int port,std::string user,std::string password);
            
            Client(std::string address,int port,std::string key);
            
            variant::Variant call(std::string plugin,std::string method);
            
            variant::Variant call(std::string plugin,std::string method,std::vector<variant::Variant> params);
            
            variant::Variant call(std::string plugin,std::string method,std::vector<variant::Variant> params, auth::Credential credential);
            
            virtual ~Client();
            
        };
    }
}
