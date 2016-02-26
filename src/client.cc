/****************************************************************************
**
** Copyright (C) 2015-2016 Michael Yang
** Contact: ohmyarchlinux@gmail.com
**
** This file is part of the twitter_cpp.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "twitter/client.h"
#include <cpprest/http_listener.h>
#include <cpprest/json.h>

namespace twitter {
twitter_client::twitter_client(const string_t &consumer_key,
                               const string_t &consumer_secret,
                               const string_t &callback_uri)
    : oauth1_config_(
          consumer_key, consumer_secret,
          u("https://api.twitter.com/oauth/request_token"),
          u("https://api.twitter.com/oauth/authorize"),
          u("https://api.twitter.com/oauth/access_token"), callback_uri,
          web::http::oauth1::experimental::oauth1_methods::hmac_sha1) {}

/* void twitter_client::listen_for_code() {
    auto listener =
        std::make_unique<web::http::experimental::listener::http_listener>(
            web::http::uri(oauth1_config_.callback_uri()));
    listener->support([this](web::http::http_request request) {
        string_t s = request.request_uri().path();
        string_t ss = request.request_uri().query();
    });
    listener->open().wait();
} */

string_t twitter_client::build_authorization_uri() {
    auto auth_uri_task = oauth1_config_.build_authorization_uri();
    try {
        string_t auth_uri = auth_uri_task.get();

        return auth_uri;
    } catch (const web::http::oauth1::experimental::oauth1_exception &e) {
        std::cout << "Error: " << e.what() << std::endl;

        return string_t();
    }
}

bool twitter_client::token_from_pin(const string_t &pin) {
    auto token_task = oauth1_config_.token_from_verifier(pin);
    try {
        token_task.get();
    } catch (const web::http::oauth1::experimental::oauth1_exception &e) {
        std::cout << "Error: " << e.what() << std::endl;

        return false;
    }

    http_client_config_.set_oauth1(oauth1_config_);

    return true;
}

std::experimental::optional<account_settings>
twitter_client::get_account_settings() const {
    web::http::client::http_client api(u("https://api.twitter.com/1.1/"),
                                       http_client_config_);

    web::json::object root =
        api.request(web::http::methods::GET, u("account/settings.json"))
            .get()
            .extract_json()
            .get()
            .as_object();

    account_settings settings;

    try {
        web::json::object time_zone = root.at(u("time_zone")).as_object();
        string_t name = time_zone[u("name")].as_string();
        int utc_offset = time_zone[u("utc_offset")].as_integer();
        string_t tzinfo_name = time_zone[u("tzinfo_name")].as_string();
        settings.set_time_zone(
            twitter::time_zone(name, utc_offset, tzinfo_name));

        bool is_protected = root.at(u("protected")).as_bool();
        settings.set_protected(is_protected);

        string_t screen_name = root.at(u("screen_name")).as_string();
        settings.set_screen_name(screen_name);

        bool is_always_use_https = root.at(u("always_use_https")).as_bool();
        settings.set_always_use_https(is_always_use_https);

        bool is_use_cookie_personalization =
            root.at(u("use_cookie_personalization")).as_bool();
        settings.set_use_cookie_personalization(is_use_cookie_personalization);

        web::json::object sleep_time = root.at(u("sleep_time")).as_object();
        if (sleep_time[u("enabled")].as_bool()) {
            hour start_time =
                static_cast<hour>(sleep_time[u("start_time")].as_integer());
            hour end_time =
                static_cast<hour>(sleep_time[u("end_time")].as_integer());
            settings.set_sleep_time(twitter::sleep_time(start_time, end_time));
        }

        bool is_geo_enabled = root.at(u("geo_enabled")).as_bool();
        settings.set_geo_enabled(is_geo_enabled);

        string_t language = root.at(u("language")).as_string();
        settings.set_language(language);

        bool is_discoverable_by_email =
            root.at(u("discoverable_by_email")).as_bool();
        settings.set_discoverable_by_email(is_discoverable_by_email);

        bool is_discoverable_by_mobile_phone =
            root.at(u("discoverable_by_mobile_phone")).as_bool();
        settings.set_discoverable_by_mobile_phone(
            is_discoverable_by_mobile_phone);

        bool is_display_sensitive_media =
            root.at(u("display_sensitive_media")).as_bool();
        settings.set_display_sensitive_media(is_display_sensitive_media);

        bool is_smart_mute = root.at(u("smart_mute")).as_bool();
        settings.set_smart_mute(is_smart_mute);

        auto string_to_allowed = [](const string_t &str) {
            if (str == u("all"))
                return allowed::all;
            else if (str == u("following"))
                return allowed::following;
            else
                return allowed::none;
        };

        allowed allow_contributor_request = string_to_allowed(
            root.at(u("allow_contributor_request")).as_string());
        settings.set_allow_contributor_request(allow_contributor_request);

        allowed allow_dms_from =
            string_to_allowed(root.at(u("allow_dms_from")).as_string());
        settings.set_allow_dms_from(allow_dms_from);

        allowed allow_dm_groups_from =
            string_to_allowed(root.at(u("allow_dm_groups_from")).as_string());
        settings.set_allow_dm_groups_from(allow_dm_groups_from);
    } catch (const web::json::json_exception &e) {
        std::cout << "Error: " << e.what() << std::endl;

        return std::experimental::optional<account_settings>();
    }

    return std::experimental::make_optional(settings);
}

std::vector<language> twitter_client::get_help_languages() const {
    std::vector<language> languages;

    web::http::client::http_client api(u("https://api.twitter.com/1.1/"),
                                       http_client_config_);

    web::json::array root =
        api.request(web::http::methods::GET, u("help/languages.json"))
            .get()
            .extract_json()
            .get()
            .as_array();

    string_t code;
    string_t name;
    string_t status;

    for (auto &e : root) {
        web::json::object object = e.as_object();
        code = object.at(u("code")).as_string();
        name = object.at(u("name")).as_string();
        status = object.at(u("status")).as_string();
        languages.emplace_back(code, name, status);
    }

    return languages;
}
}
