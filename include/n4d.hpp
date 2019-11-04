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

#ifndef EDUPALS_N4D
#define EDUPALS_N4D

#include <variant.hpp>

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <exception>

namespace edupals
{
    namespace n4d
    {
        namespace exception
        {
            class BadN4DResponse : public std::exception
            {
                public:
                
                const char* what() const throw()
                {
                    return "Malformed N4D method response";
                }
            };
            
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
                 * User name and password constructor
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
        
        enum Option
        {
            None = 0x00,
            Verbose = 0x01, /*! dump xml traffic into stderr */
            All = 0xff
        };
        
        class Client
        {
            protected:
            int flags;
            
            std::string address;
            int port;
            auth::Credential credential;
            
            void post(std::stringstream& in,std::stringstream& out);
            
            void create_value(variant::Variant param,std::stringstream& out);

            void create_request(std::string method,
                                std::vector<variant::Variant> params,
                                std::stringstream& out);
            
            public:
            
            /*!
             * Default client to https://localhost 9779 and anonymous credential
            */
            Client(std::string address="https://localhost",int port=9779);
            
            /*!
             * Client using a user/password as default credential
            */
            Client(std::string address,int port,std::string user,std::string password);
            
            /*!
             * Client using a key as default credential
            */
            Client(std::string address,int port,std::string key);
            
            /*!
             * Perform a raw xml-rpc call
            */
            variant::Variant rpc_call(std::string method,std::vector<variant::Variant> params);
            
            /*!
             * Perform a sync n4d call to Plugin.method
            */
            variant::Variant call(std::string plugin,std::string method);
            
            /*!
             * Perform a sync n4d call to Plugin.method with given params
            */
            variant::Variant call(std::string plugin,std::string method,std::vector<variant::Variant> params);
            
            /*!
             * Perform a sync n4d call to Plugin.method with given params and
             * custom credentials
            */
            variant::Variant call(std::string plugin,std::string method,std::vector<variant::Variant> params, auth::Credential credential);
            
            virtual ~Client();
            
            /*!
             * Checks whenever a user/password is valid in that server
            */
            bool validate_user(std::string name,std::string password);
            
            /*!
                Get the list of groups an user belongs to
            */
            std::vector<std::string> get_groups(std::string name,std::string password);
            
            /*!
             * Gets a list of available methods on server
             * \returns a map of string->vector of strings
            */
            std::map<std::string,std::vector<std::string> > get_methods();
            
            /*!
                Checks whenever the server is running at specified address and port
                Internally it calls a get_methods but no exception is thrown
            */
            bool running();
            
            /*!
                Set flags
            */
            void set_flags(int flags);
            
            /*!
                Get current flags
            */
            int get_flags();
        };
    }
}

#endif
