/*
 * 2013+ Copyright (c) Anton Tyurin <noxiouz@yandex.ru>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "session.h"
#include <errno.h>

using namespace ioremap;

extern "C" {

//This header is generated by cgo in compile time
#include "_cgo_export.h"

struct go_data_pointer new_data_pointer(char *data, int size) {
	return go_data_pointer {
		data, size
	};
};

void on_finish(context_t context, const elliptics::error_info &error)
{
	go_error err;
	err.code = error.code();
	err.flags = 0;
	err.message = error.message().c_str();

	go_final_callback(&err, context);
}

ell_session *new_elliptics_session(ell_node *node)
{
	ell_session *session = new elliptics::session(*node);
	session->set_exceptions_policy(elliptics::session::no_exceptions);
	// do not set negative/all filters without checking all callbacks,
	// they expect only valid values
	//session->set_filter(elliptics::filters::all);
	return session;
}

void delete_session(ell_session *session)
{
	delete session;
}

void session_set_filter_all(ell_session *session)
{
	session->set_filter(elliptics::filters::all);
}
void session_set_filter_positive(ell_session *session)
{
	session->set_filter(elliptics::filters::positive);
}

void session_set_groups(ell_session *session, uint32_t *groups, int count)
{
	std::vector<int>g(groups, groups + count);
	session->set_groups(g);
}

void session_set_namespace(ell_session *session, const char *name, int nsize)
{
	session->set_namespace(name, nsize);
}

void session_set_timeout(ell_session *session, int timeout)
{
	session->set_timeout(timeout);
}

void session_set_cflags(ell_session *session, cflags_t cflags)
{
	session->set_cflags(cflags);
}

void session_set_ioflags(ell_session *session, uint32_t ioflags)
{
	session->set_ioflags(ioflags);
}

void session_set_trace_id(ell_session *session, trace_id_t trace_id)
{
	session->set_trace_id(trace_id);
}

trace_id_t session_get_trace_id(ell_session *session)
{
	return session->get_trace_id();
}

long session_get_timeout(ell_session *session)
{
	return session->get_timeout();
}

cflags_t session_get_cflags(ell_session *session)
{
	return session->get_cflags();
}

ioflags_t session_get_ioflags(ell_session *session)
{
	return session->get_ioflags();
}

const char *session_transform(ell_session *session, const char *key)
{
	dnet_raw_id id;
	session->transform(key, id);
	return dnet_dump_id_str_full(id.id);
}

/*
 * Read
 */
static void on_read(context_t context, const elliptics::read_result_entry &result)
{
	if (result.error()) {
		go_error err {
			result.error().code(),
			result.command()->flags,
			result.error().message().c_str()
		};

		go_read_error(result.command(), result.address(), &err, context);
	} else {
		elliptics::data_pointer data(result.file());
		go_read_result to_go {
			result.command(), result.address(),
			result.io_attribute(), (const char *)data.data(), data.size()
		};

		go_read_callback(&to_go, context);
	}
}

void session_read_data(ell_session *session, context_t on_chunk_context,
		       context_t final_context, ell_key *key, uint64_t offset, uint64_t size)
{
	using namespace std::placeholders;
	session->read_data(*key, offset, size).connect(std::bind(&on_read, on_chunk_context, _1),
				      std::bind(&on_finish, final_context, _1));
}

/*
 * Write and Lookup
 */
static void on_lookup(context_t context, const elliptics::lookup_result_entry & result)
{
	if (result.error()) {
		go_error err {
			result.error().code(),
			result.command()->flags,
			result.error().message().c_str()
		};

		go_lookup_error(result.command(), result.address(), &err, context);
	} else {
		go_lookup_result to_go {
			result.command(), result.address(),
			result.file_info(), result.storage_address(), result.file_path()
		};

		go_lookup_callback(&to_go, context);
	}
}

void session_write_data(ell_session *session, context_t on_chunk_context,
			context_t final_context, ell_key *key, uint64_t offset,
			char *data, uint64_t size)
{
	using namespace std::placeholders;

	elliptics::data_pointer tmp = elliptics::data_pointer::from_raw(data, size);
	session->write_data(*key, tmp, offset).connect(std::bind(&on_lookup, on_chunk_context, _1),
				       std::bind(&on_finish, final_context, _1));
}

void session_write_prepare(ell_session *session, context_t on_chunk_context,
			context_t final_context, ell_key *key,
			uint64_t offset, uint64_t total_size,
			char *data, uint64_t size)
{
	using namespace std::placeholders;

	elliptics::data_pointer tmp = elliptics::data_pointer::from_raw(data, size);
	session->write_prepare(*key, tmp, offset, total_size).connect(std::bind(&on_lookup, on_chunk_context, _1),
				       std::bind(&on_finish, final_context, _1));
}

void session_write_plain(ell_session *session, context_t on_chunk_context,
			context_t final_context, ell_key *key,
			uint64_t offset,
			char *data, uint64_t size)
{
	using namespace std::placeholders;

	elliptics::data_pointer tmp = elliptics::data_pointer::from_raw(data, size);
	session->write_plain(*key, tmp, offset).connect(std::bind(&on_lookup, on_chunk_context, _1),
				       std::bind(&on_finish, final_context, _1));
}

void session_write_commit(ell_session *session, context_t on_chunk_context,
			context_t final_context, ell_key *key,
			uint64_t offset,
			uint64_t commit_size,
			char *data, uint64_t size)
{
	using namespace std::placeholders;

	elliptics::data_pointer tmp = elliptics::data_pointer::from_raw(data, size);
	session->write_commit(*key, tmp, offset, commit_size).connect(std::bind(&on_lookup, on_chunk_context, _1),
				       std::bind(&on_finish, final_context, _1));
}

void session_lookup(ell_session *session, context_t on_chunk_context,
		    context_t final_context, ell_key *key)
{
	using namespace std::placeholders;
	session->lookup(*key).connect(std::bind(&on_lookup, on_chunk_context, _1),
				      std::bind(&on_finish, final_context, _1));
}

void session_parallel_lookup(ell_session *session, context_t on_chunk_context,
		    context_t final_context, ell_key *key)
{
	using namespace std::placeholders;
	session->parallel_lookup(*key).connect(std::bind(&on_lookup, on_chunk_context, _1),
				      std::bind(&on_finish, final_context, _1));
}

/*
 * Remove
 * @on_remove() callback converts returned command into golang DnetCmd
 */
static void on_remove(context_t context, const elliptics::remove_result_entry &result)
{
	go_remove_result res {
		result.command()
	};

	go_remove_callback(&res, context);
}

void session_remove(ell_session *session, context_t on_chunk_context,
		    context_t final_context, ell_key *key)
{
	using namespace std::placeholders;
	session->remove(*key).connect(std::bind(&on_remove, on_chunk_context, _1),
				      std::bind(&on_finish, final_context, _1));
}

void session_bulk_remove(ell_session *session, context_t on_chunk_context, context_t final_context, void *ekeys)
{
	using namespace std::placeholders;

	ell_keys *keys = (ell_keys *)ekeys;

	for (auto it = keys->kk.begin(); it != keys->kk.end(); ++it) {
		it->transform(*session);
	}

	session->set_filter(elliptics::filters::all_with_ack);
	session->bulk_remove(keys->kk).connect(std::bind(&on_remove, on_chunk_context, _1),
				      std::bind(&on_finish, final_context, _1));
}

/*
 * Find
 */
static void on_find(context_t context, const elliptics::find_indexes_result_entry &result)
{
	std::vector <c_index_entry> c_index_entries;

	for (uint64_t i = 0; i < result.indexes.size(); i++) {
		c_index_entries.push_back(c_index_entry {
						(const char *)
						result.indexes[i].data.data(),
						result.indexes[i].data.size()
					  });
	}

	go_find_result to_go {
		&result.id, c_index_entries.size(), c_index_entries.data()
	};

	go_find_callback(&to_go, context);
}

void session_find_all_indexes(ell_session *session,
			      context_t on_chunk_context,
			      context_t final_context, char *indexes[], uint64_t nsize)
{
	using namespace std::placeholders;
	std::vector <std::string> index_names(indexes, indexes + nsize);
	session->find_all_indexes(index_names).connect(std::bind(&on_find, on_chunk_context, _1),
						  std::bind(&on_finish, final_context, _1));
}

void session_find_any_indexes(ell_session *session,
			      context_t on_chunk_context,
			      context_t final_context, char *indexes[], uint64_t nsize)
{
	using namespace std::placeholders;
	std::vector <std::string> index_names(indexes, indexes + nsize);
	session->find_any_indexes(index_names).connect(std::bind(&on_find, on_chunk_context, _1),
						  std::bind(&on_finish, final_context, _1));
}


/*
 * Indexes
 * Not implemented. Don't know about anything usefull informaitopn from result.
 */
static void on_set_indexes(context_t context, const elliptics::callback_result_entry &result)
{
	(void)context;
	(void)result;
}

static void on_list_indexes(context_t context, const elliptics::index_entry &result)
{
	c_index_entry to_go {
		(const char *)result.data.data(), result.data.size()
	};

	go_index_entry_callback(&to_go, context);
}

void session_list_indexes(ell_session *session,
			  context_t on_chunk_context, context_t final_context, ell_key *key)
{
	using namespace std::placeholders;
	session->list_indexes(*key).connect(std::bind(&on_list_indexes, on_chunk_context, _1),
					    std::bind(&on_finish, final_context, _1));
}

void session_set_indexes(ell_session *session, context_t on_chunk_context,
			 context_t final_context, ell_key *key,
			 char *indexes[], struct go_data_pointer *data, uint64_t count)
{
	/*Move to util function */
	using namespace std::placeholders;
	std::vector<std::string> index_names(indexes, indexes + count);
	std::vector<elliptics::data_pointer> index_datas;

	index_datas.reserve(count);
	for (uint64_t i = 0; i < count; i++) {
		elliptics::data_pointer dp = elliptics::data_pointer::from_raw(data[i].data, data[i].size);
		index_datas.push_back(dp);
	}

	session->set_indexes(*key, index_names, index_datas)
		.connect(std::bind(&on_set_indexes, on_chunk_context, _1),
			 std::bind(&on_finish, final_context, _1));
}

void session_update_indexes(ell_session *session,
			    context_t on_chunk_context,
			    context_t final_context, ell_key *key,
			    char *indexes[], struct go_data_pointer *data, uint64_t count)
{
	/*Move to util function */
	using namespace std::placeholders;
	std::vector<std::string> index_names(indexes, indexes + count);
	std::vector<elliptics::data_pointer> index_datas;

	index_datas.reserve(count);
	for (uint64_t i = 0; i < count; i++) {
		elliptics::data_pointer dp = elliptics::data_pointer::from_raw(data[i].data, data[i].size);
		index_datas.push_back(dp);
	}

	session->update_indexes(*key, index_names, index_datas)
		.connect(std::bind(&on_set_indexes, on_chunk_context, _1),
			 std::bind(&on_finish, final_context, _1));
}

void session_remove_indexes(ell_session *session,
			    context_t on_chunk_context,
			    context_t final_context, ell_key *key,
			    char *indexes[], uint64_t nsize)
{
	using namespace std::placeholders;
	std::vector<std::string> index_names(indexes, indexes + nsize);

	session->remove_indexes(*key, index_names)
		.connect(std::bind(&on_set_indexes, on_chunk_context, _1),
			 std::bind(&on_finish, final_context, _1));
}

static void on_backend_status(context_t context, const std::vector<elliptics::backend_status_result_entry> &result,
		const elliptics::error_info &error)
{
	if (error) {
		go_error err {
			error.code(),
			0,
			error.message().c_str()
		};

		go_backend_status_error(context, &err);
		return;
	}

	struct dnet_backend_status_list *elements = result[0].list();
	go_backend_status_callback(context, elements);
}

void session_backends_status(ell_session *session, const struct dnet_addr *addr, context_t context)
{
	session->request_backends_status((*addr)).connect(std::bind(&on_backend_status, context,
				std::placeholders::_1, std::placeholders::_2));
}
void session_backend_start_defrag(ell_session *session, const struct dnet_addr *addr, uint32_t backend_id, context_t context)
{
	session->start_defrag((*addr), backend_id).connect(std::bind(&on_backend_status, context,
				std::placeholders::_1, std::placeholders::_2));
}
void session_backend_enable(ell_session *session, const struct dnet_addr *addr, uint32_t backend_id, context_t context)
{
	session->enable_backend((*addr), backend_id).connect(std::bind(&on_backend_status, context,
				std::placeholders::_1, std::placeholders::_2));
}
void session_backend_disable(ell_session *session, const struct dnet_addr *addr, uint32_t backend_id, context_t context)
{
	session->disable_backend((*addr), backend_id).connect(std::bind(&on_backend_status, context,
				std::placeholders::_1, std::placeholders::_2));
}
void session_backend_make_writable(ell_session *session, const struct dnet_addr *addr, uint32_t backend_id, context_t context)
{
	session->make_writable((*addr), backend_id).connect(std::bind(&on_backend_status, context,
				std::placeholders::_1, std::placeholders::_2));
}
void session_backend_make_readonly(ell_session *session, const struct dnet_addr *addr, uint32_t backend_id, context_t context)
{
	session->make_readonly((*addr), backend_id).connect(std::bind(&on_backend_status, context,
				std::placeholders::_1, std::placeholders::_2));
}
void session_backend_set_delay(ell_session *session, const struct dnet_addr *addr, uint32_t backend_id, uint32_t delay, context_t context)
{
	session->set_delay((*addr), backend_id, delay).connect(std::bind(&on_backend_status, context,
				std::placeholders::_1, std::placeholders::_2));
}

int session_lookup_addr(ell_session *session, const char *key, int len, int group_id, struct dnet_addr *addr, int *backend_id)
{
	return dnet_lookup_addr(session->get_native(), key, len, NULL, group_id, addr, backend_id);
}

} // extern "C"
