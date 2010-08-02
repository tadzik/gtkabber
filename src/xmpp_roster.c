#include "xmpp_roster.h"

#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "ui_roster.h"
#include "types.h"
#include "xmpp_conn.h"

/* global vars */
GSList *roster;
/***************/

void
xmpp_roster_add_resource(Buddy *b, Resource *r)
{
	if(!xmpp_roster_find_res_by_name(b, r->name))
		b->resources = g_slist_prepend(b->resources, r);
} /* xmpp_roster_add_resource */

void
xmpp_roster_cleanup()
{
	GSList *elem, *res;
	for(elem = roster; elem; elem = elem->next) {
		Buddy *b = (Buddy *)elem->data;
		g_free(b->name);
		g_free(b->jid);
		g_free(b->group);
		for(res = b->resources; res; res = res->next) {
			Resource *r = (Resource *)res->data;
			g_free(r->name);
			g_free(r->status_msg);
			g_free(r);
		}
		if(b->resources) g_slist_free(b->resources);
		g_free(b);
	}
	if(roster) g_slist_free(roster);
} /* xmpp_roster_cleanup */

Resource *
xmpp_roster_find_res_by_name(Buddy *sb, const char *name)
{
	GSList *elem;
	Resource *res;
	for(elem = sb->resources; elem; elem = elem->next) {
		res = (Resource *)elem->data;
		if(g_strcmp0(name, res->name) == 0) return res;
	}
	return NULL;
} /* xmpp_roster_find_res_by_name */

Buddy *
xmpp_roster_find_by_jid(const char *jid)
{
	Buddy *bud;
	GSList *elem;
	for(elem = roster; elem; elem = elem->next) {
		bud = (Buddy *)elem->data;
		if(g_strcmp0(bud->jid, jid) == 0) return bud;
	}
	return NULL;
} /* xmpp_roster_find_by_jid */

Resource *
xmpp_roster_get_best_resource(const char *jid)
{
	Buddy *sb;
	GSList *elem;
	Resource *best = NULL;
	sb = xmpp_roster_find_by_jid(jid);
	if(!sb) {
		g_printerr("xmpp_roster_get_best_resource: buddy %s not found",
		           jid);
		return NULL;
	}
	if(sb->resources == NULL) return NULL;
	for(elem = sb->resources; elem; elem = elem->next) {
		Resource *ed = (Resource *)elem->data;
		if(!best || best->priority < ed->priority)
			best = elem->data;
	}
	return best;
} /* xmpp_roster_get_best_resource */

void
xmpp_roster_parse_query(LmMessageNode *q)
{
	LmMessageNode *item;
	for(item = lm_message_node_get_child(q, "item"); item;
	    item = item->next) {
		Buddy *entry;	
		const char *attr;
		LmMessageNode *node;
		/* first, let's check if it isn't just a delete request */
		attr = lm_message_node_get_attribute(item, "subscription");
		if(g_strcmp0(attr, "remove") == 0) {
			/*TODO: Delete this guy from our roster*/
			g_printerr("Removing somebody from roster\n");
			continue;
		}
		attr = lm_message_node_get_attribute(item, "jid");
		/* Checking if we don't have this guy on our roster */
		if(xmpp_roster_find_by_jid(attr))
			continue;
		entry = (Buddy *)g_malloc(sizeof(Buddy));
		entry->jid = g_strdup(attr ? attr : "");
		attr = lm_message_node_get_attribute(item, "name");
		if(attr) {
			entry->name = g_strdup(attr);
		} else {
			entry->name = g_strndup(entry->jid,
			                      strchr(entry->jid, '@') - entry->jid);
		}
		attr = lm_message_node_get_attribute(item, "subscription");
		if(attr[0] == 'b')
			entry->subscription = BOTH;
		else if(attr[0] == 't')
			entry->subscription = TO;
		else if(attr[0] == 'f')
			entry->subscription = FROM;
		else if(attr[0] == 'n')
			entry->subscription = NONE;
		/* "remove" has alredy been covered */
		node = lm_message_node_get_child(item, "group");
		if(node)
			entry->group = g_strdup(lm_message_node_get_value(node));
		else
			entry->group = NULL;
		/* creating resources list */
		entry->resources = NULL;
		roster = g_slist_prepend(roster, entry);
		ui_roster_add(entry->jid, entry->name, entry->group);
	}
	xmpp_roster_parsed_cb();
} /* xmpp_roster_parse_query */

void
xmpp_roster_request(LmConnection *conn)
{
	LmMessage *req;
	LmMessageNode *query;
	GError *err = NULL;
	req = lm_message_new_with_sub_type(NULL, LM_MESSAGE_TYPE_IQ,
	                                   LM_MESSAGE_SUB_TYPE_GET);
	query = lm_message_node_add_child(req->node, "query", NULL);
	lm_message_node_set_attributes(query, "xmlns", "jabber:iq:roster", NULL);
	if(!lm_connection_send(conn, req, &err)) {
		ui_print("Error sending roster request: %s\n", err->message);
		g_error_free(err);
	}
	lm_message_unref(req);
} /* xmpp_roster_request */
