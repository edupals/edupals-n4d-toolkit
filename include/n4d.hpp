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

namespace edupals
{
    namespace n4d
    {
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
            
            public:
            
            /*!
             * Default client to https://localhost 9779 and anonymous credential
            */
            Client();
            
            Client(std::string address,int port);
            
            Client(std::string address,int port,std::string user,std::string password);
            
            Client(std::string address,int port,std::string key);
            
            variant::Variant call(std::string plugin,std::string method);
            
            variant::Variant call(std::string plugin,std::string method,std::vector<variant::Variant> args);
            
            variant::Variant call(std::string plugin,std::string method,std::vector<variant::Variant> args, auth::Credential credential);
            
            virtual ~Client();
            
        };
    }
}