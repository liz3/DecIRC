/*
 *  IXUrlParser.h
 *  Author: Benjamin Sergeant
 *  Copyright (c) 2019 Machine Zone, Inc. All rights reserved.
 */

#ifndef DEC_URL_PARSER_H
#define DEC_URL_PARSER_H
#include <string>


    class UrlParser
    {
    public:
        static bool parse(const std::string& url,
                          std::string& protocol,
                          std::string& host,
                          std::string& path,
                          std::string& query,
                          std::string& fragment,
                          int& port);

        static bool parse(const std::string& url,
                          std::string& protocol,
                          std::string& host,
                          std::string& path,
                          std::string& query,
                          std::string& fragment,
                          int& port,
                          bool& isProtocolDefaultPort);
        static std::string urlDecode(const std::string& in);
    };
#endif