/****************************************************************************
**
** Copyright (C) 2015-2016 Michael Yang
** Contact: ohmyarchlinux@gmail.com
**
** This file is part of the examples of the twitter_cpp.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "twitter/client.h"
#include "twitter/utility.h"
#include <fstream>

static const twitter::string_t consumer_key(u("0eDcIJLNpeeSosBdvNQk5cq3u"));
static const twitter::string_t
    consumer_secret(u("zltRuNwj1twFxKBMb22Kz8DSRqgW9W3lxCO9EstlqDCysJc6le"));
static const twitter::string_t callback_uri(u("http://127.0.0.1:8890/"));
static const twitter::string_t proxy_uri(u("http://localhost:8118/"));

static bool open_browser(const twitter::string_t &uri) {
#if defined(_WIN32) && !defined(__cplusplus_winrt)
    // NOTE: Windows desktop only.
    auto r = ShellExecuteA(NULL, "open", twitter::utf16_to_utf8(uri), NULL,
                           NULL, SW_SHOWNORMAL);

    return true;
#elif defined(__APPLE__)
    // NOTE: OS X only.
    std::string browser_cmd("open \"" + uri + "\"");
    std::system(browser_cmd.c_str());

    return true;
#else
    // NOTE: Linux/X11 only.
    std::string browser_cmd("xdg-open \"" + uri + "\"");
    std::system(browser_cmd.c_str());

    return true;
#endif
}

#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    twitter::twitter_client client(consumer_key, consumer_secret, u("oob"));
    client.set_proxy(proxy_uri);

    twitter::string_t access_token;
    twitter::string_t access_token_secret;

    {
        twitter::ifstream_t access_token_file("access_token");
        twitter::ifstream_t access_token_secret_file("access_token_secret");

        if (access_token_file.is_open() && access_token_secret_file.is_open()) {
            access_token_file >> access_token;
            access_token_secret_file >> access_token_secret;
        }
    }

    twitter::token token(access_token, access_token_secret);
    if (token.is_valid_access_token()) {
        client.set_token(token);
    } else {
        twitter::string_t &&auth_uri = client.build_authorization_uri();

        if (auth_uri.empty())
            return 0;

        open_browser(auth_uri);

        std::cout << "Enter the PIN provided by twitter: ";

        twitter::string_t pin;
        ucin >> pin;

        if (!client.token_from_pin(pin)) {
            std::cout << "Authorization failed." << std::endl;

            return 0;
        }

        token = client.token();

        twitter::ofstream_t access_token_file("access_token");
        twitter::ofstream_t access_token_secret_file("access_token_secret");

        access_token_file << token.access_token();
        access_token_secret_file << token.secret();
    }

    std::vector<twitter::language> languages = client.get_help_languages();
    if (languages.empty()) {
        std::cout << "Failed." << std::endl;
    }

    std::cout << "Languages supported by Twitter:" << std::endl << std::endl;

    std::size_t index = 0;
    std::size_t size = languages.size();

    for (auto &e : languages) {
        ucout << u("code: ") << e.code() << std::endl
              << u("name: ") << e.name() << std::endl
              << u("status: ") << e.status() << std::endl;

        if (++index < size)
            std::cout << std::endl;
    }
}
