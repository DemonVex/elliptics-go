/*
* 2013+ Copyright (c) Anton Tyurin <noxiouz@yandex.ru>
* All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*/

#ifndef SESSION_H
#define SESSION_H

#include "node.h"
#include "key.h"

#ifdef __cplusplus

#include <iostream>
#include <functional>
#include <elliptics/session.hpp>
typedef ioremap::elliptics::session ell_session;
extern "C" {

#else
typedef void ell_session;
#endif

typedef void(*Callback)(void*, void*);

ell_session*
new_elliptics_session(ell_node* node);

void
session_read_data(ell_session *session, ell_key *key);

void
session_stat_log(ell_session *session, Callback clb, void *ch);


#ifdef __cplusplus 
}
#endif

#endif