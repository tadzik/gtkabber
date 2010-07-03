#include <gtk/gtk.h>
#include <string.h>

/* Stolen from gnome bugzilla (small hacks for my needs).
 * Fsck you gnome guys for not adding this.
 * The patch is there since 2004 goddamnit */

/**
 * tbim: (text_buffer_insert_markup)
 * @buffer: a #GtkTextBuffer
 * @iter: a position in the buffer
 * @markup: UTF-8 format text with pango markup to insert
 * @len: length of text in bytes, or -1
 *
 * Inserts @len bytes of @markup at position @iter.  If @len is -1,
 * @text must be nul-terminated and will be inserted in its
 * entirety. Emits the "insert_text" signal, possibly multiple
 * times; insertion actually occurs in the default handler for
 * the signal. @iter will point to the end of the inserted text
 * on return
 *
 **/

void
gtk_tbim (GtkTextBuffer *buffer,
	  GtkTextIter   *textiter,
	  const gchar   *markup,
	  gint           len)
{
	PangoAttrIterator  *paiter;
	PangoAttrList      *attrlist;
	GtkTextMark        *mark;
	GError             *error = NULL;
	gchar              *text;

  g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (textiter != NULL);
  g_return_if_fail (markup != NULL);
  g_return_if_fail (gtk_text_iter_get_buffer (textiter) == buffer);

	if (len == 0)
		return;

	if (!pango_parse_markup(markup, len, 0, &attrlist, &text, NULL, &error))
	{
		g_warning("Invalid markup string: %s", error->message);
		g_error_free(error);
		return;
	}

	len = strlen(text); /* TODO: is this needed? */

	if (attrlist == NULL)
	{
		gtk_text_buffer_insert(buffer, textiter, text, len);
		g_free(text);
		return;
	}

	/* create mark with right gravity */
	mark = gtk_text_buffer_create_mark(buffer, NULL, textiter, FALSE);

	paiter = pango_attr_list_get_iterator(attrlist);

	do
	{
		PangoAttribute *attr;
		GtkTextTag     *tag;
		gint            start, end;

		pango_attr_iterator_range(paiter, &start, &end);

		if (end == G_MAXINT)  /* last chunk */
			end = strlen(text);

		tag = gtk_text_tag_new(NULL);

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_LANGUAGE)))
			g_object_set(tag, "language", pango_language_to_string(((PangoAttrLanguage*)attr)->value), NULL);

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_FAMILY)))
			g_object_set(tag, "family", ((PangoAttrString*)attr)->value, NULL);

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_STYLE)))
			g_object_set(tag, "style", ((PangoAttrInt*)attr)->value, NULL);

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_WEIGHT)))
			g_object_set(tag, "weight", ((PangoAttrInt*)attr)->value, NULL);

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_VARIANT)))
			g_object_set(tag, "variant", ((PangoAttrInt*)attr)->value, NULL);

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_STRETCH)))
			g_object_set(tag, "stretch", ((PangoAttrInt*)attr)->value, NULL);

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_SIZE)))
			g_object_set(tag, "size", ((PangoAttrInt*)attr)->value, NULL);

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_FONT_DESC)))
			g_object_set(tag, "font-desc", ((PangoAttrFontDesc*)attr)->desc, NULL);

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_FOREGROUND)))
		{
			GdkColor col = { 0,
			                 ((PangoAttrColor*)attr)->color.red,
			                 ((PangoAttrColor*)attr)->color.green,
			                 ((PangoAttrColor*)attr)->color.blue
			               };

			g_object_set(tag, "foreground-gdk", &col, NULL);
		}

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_BACKGROUND)))
		{
			GdkColor col = { 0,
			                 ((PangoAttrColor*)attr)->color.red,
			                 ((PangoAttrColor*)attr)->color.green,
			                 ((PangoAttrColor*)attr)->color.blue
			               };

			g_object_set(tag, "background-gdk", &col, NULL);
		}

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_UNDERLINE)))
			g_object_set(tag, "underline", ((PangoAttrInt*)attr)->value, NULL);

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_STRIKETHROUGH)))
			g_object_set(tag, "strikethrough", (gboolean)(((PangoAttrInt*)attr)->value != 0), NULL);

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_RISE)))
			g_object_set(tag, "rise", ((PangoAttrInt*)attr)->value, NULL);

		/* PANGO_ATTR_SHAPE cannot be defined via markup text */

		if ((attr = pango_attr_iterator_get(paiter, PANGO_ATTR_SCALE)))
			g_object_set(tag, "scale", ((PangoAttrFloat*)attr)->value, NULL);

		gtk_text_tag_table_add(gtk_text_buffer_get_tag_table(buffer), tag);

		gtk_text_buffer_insert_with_tags(buffer, textiter, text+start, end - start, tag, NULL);

		/* mark had right gravity, so it should be
		 *  at the end of the inserted text now */
		gtk_text_buffer_get_iter_at_mark(buffer, textiter, mark);
	}
	while (pango_attr_iterator_next(paiter));

	gtk_text_buffer_delete_mark(buffer, mark);
	pango_attr_iterator_destroy(paiter);
	pango_attr_list_unref(attrlist);
	g_free(text);
}



