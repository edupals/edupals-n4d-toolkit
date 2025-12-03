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
        /*! N4D server error codes */
        enum ErrorCode
        {
            UnknownClass = -40,
            UnknownMethod = -30,
            UserNotAllowed = -20,
            AuthenticationFailed = -10,
            InvalidResponse = -5,
            InvalidArguments = -3,
            UnhandledError = -2,
            CallFailed = -1,
            CallSuccessful = 0
        };
        
        enum VariableErrorCode
        {
            NotFound = -5,
            Protected = -10,
            RemoteServerError = -15,
            BackupError =  -30,
            RestoreError = -35,
            RemoteServerNotConfigured = -40
        };
        
        namespace exception
        {
            class UnknownClass: public std::exception
            {
                public:
                
                std::string msg;
                
                UnknownClass(std::string& name)
                {
                    msg="Class "+name+" not found";
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class UnknownMethod: public std::exception
            {
                public:
                
                std::string msg;
                
                UnknownMethod(std::string& name,std::string& method)
                {
                    msg="Method "+name+"::"+method+"() not found";
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class UserNotAllowed: public std::exception
            {
                public:
                
                std::string msg;
                
                UserNotAllowed(std::string& user,std::string& name,std::string& method)
                {
                    msg=user+" not allowed to "+name+"::"+method+"()";
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class AuthenticationFailed: public std::exception
            {
                public:
                
                std::string msg;
                
                AuthenticationFailed(std::string& user)
                {
                    msg="Authentication failed for user "+user;
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class InvalidMethodResponse: public std::exception
            {
                public:
                
                std::string msg;
                
                InvalidMethodResponse(std::string& name,std::string& method)
                {
                    msg="Invalid response from "+name+"::"+method+"()";
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class InvalidServerResponse: public std::exception
            {
                public:
                
                std::string msg;
                
                InvalidServerResponse(std::string& server)
                {
                    msg="Invalid response from server "+server;
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class InvalidArguments: public std::exception
            {
                public:
                
                std::string msg;
                
                InvalidArguments(std::string& name,std::string& method)
                {
                    msg="Invalid number of arguments for "+name+"::"+method+"()";
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class UnhandledError: public std::exception
            {
                public:
                
                std::string msg;
                
                UnhandledError(std::string& name,std::string& method, std::string traceback)
                {
                    msg="Unhandled error on "+name+"::"+method+"():\n\n"+traceback;
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class CallFailed: public std::exception
            {
                public:
                
                std::string msg;
                
                std::string message;
                int code;
                
                CallFailed(std::string& name,std::string& method,int code,std::string message)
                {
                    this->code=code;
                    this->message=message;
                    
                    msg=name+"::"+method+"() returned error code :"+ std::to_string(code);
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class UnknownCode: public std::exception
            {
                public:
                
                std::string msg;
                
                UnknownCode(std::string& name,std::string& method,int code)
                {
                    msg=name+"::"+method+"() returned an unknown error code "+ std::to_string(code);
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class ServerError : public std::exception
            {
                private:
                std::string msg;
                
                public:
                
                uint64_t code;
                
                ServerError(uint64_t code, std::string message)
                {
                    msg="["+std::to_string(code)+"]:"+message;
                    this->code=code;
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
                
            };
            
            class InvalidCredential : public std::exception
            {
                public:
                
                const char* what() const throw()
                {
                    return "Invalid credential type";
                }
            };
            
            class InvalidBuiltInResponse: public std::exception
            {
                public:
                
                std::string msg;
                
                InvalidBuiltInResponse(std::string method, std::string info)
                {
                    msg="Invalid response from "+method+"(): "+info;
                }
                
                const char* what() const throw()
                {
                    return msg.c_str();
                }
            };
            
            class TicketFailed : public std::exception
            {
                public:
                
                const char* what() const throw()
                {
                    return "Can not read ticket";
                }
            };
            
            namespace variable
            {
                class NotFound : public std::exception
                {
                    private:
                    std::string msg;
                    
                    public:
                    
                    NotFound(std::string name)
                    {
                        msg="Variable not found: "+name;
                    }
                    
                    const char* what() const throw()
                    {
                        return msg.c_str();
                    }
                    
                };
                
                class Protected : public std::exception
                {
                    private:
                    std::string msg;
                    
                    public:
                    
                    Protected(std::string name)
                    {
                        msg="Variable is protected: "+name;
                    }
                    
                    const char* what() const throw()
                    {
                        return msg.c_str();
                    }
                    
                };
                
                class RemoteServerError : public std::exception
                {
                    private:
                    std::string msg;
                    
                    public:
                    
                    RemoteServerError()
                    {
                        msg="Remote server error";
                    }
                    
                    const char* what() const throw()
                    {
                        return msg.c_str();
                    }
                    
                };
                
                class BackupError : public std::exception
                {
                    private:
                    std::string msg;
                    
                    public:
                    
                    BackupError()
                    {
                        msg="Backup error";
                    }
                    
                    const char* what() const throw()
                    {
                        return msg.c_str();
                    }
                    
                };
                
                class RestoreError : public std::exception
                {
                    private:
                    std::string msg;
                    
                    public:
                    
                    RestoreError()
                    {
                        msg="Restore error";
                    }
                    
                    const char* what() const throw()
                    {
                        return msg.c_str();
                    }
                    
                };
                
                class RemoteServerNotConfigured : public std::exception
                {
                    private:
                    std::string msg;
                    
                    public:
                    
                    RemoteServerNotConfigured()
                    {
                        msg="Remote server not configured";
                    }
                    
                    const char* what() const throw()
                    {
                        return msg.c_str();
                    }
                    
                };
            }
        }
        
        namespace auth
        {
            enum class Type
            {
                None,
                Anonymous,
                Password,
                Key,
                MasterKey
            };
            
            class Key
            {
                public:
                
                std::string value;
                
                /*!
                 * Create an empty (non valid) key
                */
                Key()
                {
                }
                
                /*!
                 * Create a N4D key from string
                */
                Key(std::string key)
                {
                    value=key;
                }
                
                /*!
                 * Checks whenever Key is properly generated
                */
                bool valid();
                
                /*!
                 * Gets master key. No exception thrown, just empty Key on error
                */
                static Key master_key();
                
                /*!
                 * Gets local user key. No exception thrown, just empty Key on error
                */
                static Key user_key(std::string user);
                
                operator bool()
                {
                    return valid();
                }
                
            };
            
            class Credential
            {
                public:
                std::string user;
                std::string password;
                Key key;
                
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
                    this->user=user;
                    this->password=password;
                    this->type=Type::Password;
                }
                
                /*!
                 * Key constructor
                */
                Credential(std::string user,Key key)
                {
                    this->type=Type::Key;
                    this->user=user;
                    this->key=key;
                }
                
                /*!
                 * Master key constructor
                 */
                Credential(Key key)
                {
                    this->type=Type::MasterKey;
                    this->key=key;
                }
                
                variant::Variant get();
            };
        }
        
        class Ticket
        {
            protected:
            
            bool _valid;
            std::string address;
            auth::Credential credential;
            
            public:
            
            Ticket(std::string address, auth::Credential credential);
            
            Ticket(std::string ticket);
            
            std::string to_string();
            
            bool valid()
            {
                return _valid;
            }
            
            operator bool()
            {
                return valid();
            }
            
            std::string get_address()
            {
                return address;
            }
            
            auth::Credential get_credential()
            {
                return credential;
            }
            
        };
        
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
            int timeout;
            
            std::string address;
            
            auth::Credential credential;
            
            void post(std::stringstream& in,std::stringstream& out);
            
            void create_value(variant::Variant param,std::stringstream& out);

            void create_request(std::string method,
                                std::vector<variant::Variant> params,
                                std::stringstream& out);
            
            bool validate_format(variant::Variant response);
            
            variant::Variant validate(variant::Variant response,std::string name,std::string method);
            
            void handle_variable_error(VariableErrorCode code, std::string name);
            
            public:
            
            /*!
             * Default client to https://127.0.0.1 9779 and anonymous credential
            */
            Client(std::string address="https://127.0.0.1:9779");
            
            [[deprecated("Port argument is deprecated, specify it on url address")]]
            Client(std::string address,int port);

            /*!
             * Client using a user/password as default credential
            */
            Client(std::string address,std::string user,std::string password);

            [[deprecated("Port argument is deprecated, specify it on url address")]]
            Client(std::string address,int port,std::string user,std::string password);

            /*!
             * Client using a key as default credential
            */
            Client(std::string address,std::string user,auth::Key key);
            
            [[deprecated("Port argument is deprecated, specify it on url address")]]
            Client(std::string address,int port,std::string user,auth::Key key);
            
            /*!
             * Client with supplied credential
             */
            Client(std::string address,auth::Credential);
            
            /*!
             * Client from ticket
             * TODO: throw exception?
            */
            Client(Ticket ticket);
            
            /*!
             * Perform a raw xml-rpc call
            */
            variant::Variant rpc_call(std::string method,std::vector<variant::Variant> params);
            
            /*!
             * Perform a sync n4d call to Plugin.method
            */
            variant::Variant call(std::string name,std::string method);
            
            /*!
             * Perform a sync n4d call to Plugin.method with given params
            */
            variant::Variant call(std::string name,std::string method,std::vector<variant::Variant> params);
            
            /*!
             * Perform a sync n4d call to Plugin.method with given params and
             * custom credentials
            */
            [[deprecated("credential argument will be ignored!")]]
            variant::Variant call(std::string name,std::string method,std::vector<variant::Variant> params, auth::Credential credential);
            
            /*!
             * Performs a N4D built in call: with no plugin name and no credential
            */
            variant::Variant builtin_call(std::string method,std::vector<variant::Variant> params);
            
            virtual ~Client();
            
            /*!
             * Checks whenever a user/password is valid in that server
            */
            [[deprecated("Use validate_auth instead. Name and password will be ignored!")]]
            bool validate_user(std::string name,std::string password);
            
            /*!
                Checks if current credentials are valid
            */
            bool validate_auth();
            
            /*!
                Checks if current user is valid and belongs to given groups
            */
            bool is_user_valid (std::vector<std::string> groups={});
            
            /*!
                Get the list of groups an user belongs to
            */
            [[deprecated("Name and password will be ignored!")]]
            std::vector<std::string> get_groups(std::string name,std::string password);
            
            /*!
                Get the list of groups an user belongs to
            */
            std::vector<std::string> get_groups();
            
            /*!
             * Gets a list of available methods on server
             * \returns a map of string->vector of strings
            */
            std::map<std::string,std::vector<std::string> > get_methods();
            
            /*!
                Creates a local n4d ticket
            */
            Ticket create_ticket();
            
            /*!
                Obtains a n4d ticket from a remote server. Needs a Password credential
            */
            Ticket get_ticket();
            
            /*!
                Get a variable
            */
            variant::Variant get_variable(std::string name,bool attribs = false);
            
            /*!
                Set a variable
            */
            void set_variable(std::string name,variant::Variant value,variant::Variant attribs);
            
            /*!
                Delete a variable
            */
            void delete_variable(std::string name);
            
            /*!
                Get a variable
            */
            variant::Variant get_variables(bool attribs=false);
            
            /*!
                Checks for a variable
            */
            bool variable_exists(std::string name);
            
            /*!
                Checks whenever the server is running at specified address and port
                Internally it calls a get_methods but no exception is thrown
            */
            bool running();
            
            /*!
                Gets server version
            */
            std::string version();
            
            /*!
                Set flags
            */
            void set_flags(int flags);
            
            /*!
                Get current flags
            */
            int get_flags();
            
            /*!
                Sets a new n4d credential
            */
            void set_credential(auth::Credential credential);
            
            /*!
                Gets current credential
            */
            auth::Credential get_credential();
            
            /*!
                Gets current URI address
            */
            std::string get_address();
            
            /*!
                Sets a new URI address of n4d server
            */
            void set_address(std::string address);
            
            /*!
             *  Gets current timeout in milliseconds
             */
            int get_timeout();

            /*!
             *  Sets timeout in milliseconds
             */
            void set_timeout(int ms);
        };
    }
}

#endif
