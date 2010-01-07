#include <loudmouth/loudmouth.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "types.h"
#include "ui_roster.h"

/* functions */
static void free_entry(gpointer);
static void free_res(gpointer);
static gint match_by_jid(gconstpointer, gconstpointer);
static gint match_res_by_name(gconstpointer, gconstpointer);
void xmpp_roster_add_resource(Buddy *, Resource *);
void xmpp_roster_cleanup();
Buddy *xmpp_roster_find_by_jid(const char *);
Resource *xmpp_roster_find_res_by_name(Buddy *, const char *);
const char *xmpp_roster_get_best_resname(const char *);
void xmpp_roster_parse_query(LmConnection *, LmMessageNode *);
void xmpp_roster_request(LmConnection *);
/*************/

/* global vars */
GSList *roster;
/***************/

static void
free_entry(gpointer elem)
{
	Buddy *sb = (Buddy *)elem;
	g_free(sb->name);
	g_free(sb->jid);
	g_free(sb->group);
	g_slist_foreach(sb->resources, (GFunc)free_res, NULL);
	if(sb->resources) g_slist_free(sb->resources);
	g_free(sb);
} /* free_entry */

static void
free_res(gpointer elem)
{
	Resource *foo = (Resource *)elem;
	g_free(foo->name);
	g_free(foo->status_msg);
} /* free_res */

static gint
match_by_jid(gconstpointer entry, gconstpointer jid)
{
	Buddy *foo = (Buddy *)entry;
	return strcmp(foo->jid, jid);
} /* match_by_jid */

static gint
match_res_by_name(gconstpointer res, gconstpointer name)
{
	Resource *foo = (Resource *)res;
	return strcmp(foo->name, name);
} /* match_res_by_name */

void
xmpp_roster_add_resource(Buddy *b, Resource *r)
{
	if(!xmpp_roster_find_res_by_name(b, r->name))
		b->resources = g_slist_prepend(b->resources, r);
} /* xmpp_roster_add_resource */

void
xmpp_roster_cleanup()
{
	g_slist_foreach(roster, (GFunc)free_entry, NULL);
	if(roster) g_slist_free(roster);
} /* xmpp_roster_cleanup */

Resource *
xmpp_roster_find_res_by_name(Buddy *sb, const char *name)
{
	GSList *elem;
	elem = g_slist_find_custom(sb->resources, name, match_res_by_name);
	if(elem)
		return (Resource *)(elem->data);
	else
		return NULL;
} /* xmpp_roster_find_res_by_name */

Buddy *
xmpp_roster_find_by_jid(const char *jid)
{
	GSList *elem;
	elem = g_slist_find_custom(roster, jid, match_by_jid);
	if(elem)
		return (Buddy *)(elem->data);
	else
		return NULL;
} /* xmpp_roster_find_by_jid */

const char *
xmpp_roster_get_best_resname(const char *jid)
{
	Buddy *sb;
	GSList *elem;
	Resource *best = NULL;
	sb = xmpp_roster_find_by_jid(jid);
	if(!sb) {
		g_printerr("xmpp_roster_get_best_resname: Not found? "
		           "Are you shitting me? What the fuck man?");
		return NULL;
	}
	for(elem = sb->resources; elem; elem = elem->next) {
		Resource *ed = (Resource *)elem->data;
		if(!best || best->priority < ed->priority)
			best = elem->data;
	}
	return best->name;
} /* xmpp_roster_get_best_resname */

void
xmpp_roster_parse_query(LmConnection *c, LmMessageNode *q)
{
	LmMessageNode *item;
	for(item = lm_message_node_get_child(q, "item"); item;
	    item = item->next) {
		Buddy *entry;	
		const char *attr;
		LmMessageNode *node;
		/* first, let's check if it isn't just a delete request */
		attr = lm_message_node_get_attribute(item, "subscription");
		if(strcmp(attr, "remove") == 0) {
			/*TODO: Delete this guy from our roster*/
			g_printerr("Removing somebody from roster\n");
			continue;
		}
		attr = lm_message_node_get_attribute(item, "jid");
		/* Checking if we don't have this guy on our roster */
		if(xmpp_roster_find_by_jid(attr))
			continue;
		entry = (Buddy *)malloc(sizeof(Buddy));
		entry->jid = strdup(attr ? attr : "");
		attr = lm_message_node_get_attribute(item, "name");
		if(attr) {
			entry->name = strdup(attr);
		} else {
			entry->name = strndup(entry->jid,
			                      strchr(entry->jid, '@') - entry->jid);
		}
		attr = lm_message_node_get_attribute(item, "subscription");
		if(strcmp(attr, "both") == 0)
			entry->subscription = BOTH;
		else if(strcmp(attr, "to") == 0)
			entry->subscription = TO;
		else if(strcmp(attr, "from") == 0)
			entry->subscription = FROM;
		else if(strcmp(attr, "none") == 0)
			entry->subscription = NONE;
		/* "remove" has alredy been covered */
		node = lm_message_node_get_child(item, "group");
		if(node)
			entry->group = strdup(lm_message_node_get_value(node));
		else
			entry->group = NULL;
		/* creating resources list */
		entry->resources = NULL;
		roster = g_slist_prepend(roster, entry);
		ui_roster_add(entry->jid, entry->name);
	}
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
	/*TODO: Make sure if I don't need to free this*/
	lm_message_node_set_attributes(query, "xmlns", "jabber:iq:roster", NULL);
	if(!lm_connection_send(conn, req, &err)) {
		ui_status_print("Error sending roster request: %s\n", err->message);
		g_error_free(err);
	}
	lm_message_unref(req);
} /* xmpp_roster_request */
