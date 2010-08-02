#ifndef XMPP_IQ_H
#define XMPP_IQ_H
#include "types.h"
#include <loudmouth/loudmouth.h>

LmHandlerResult xmpp_iq_handler(LmMessageHandler *, LmConnection *,
				  LmMessage *, gpointer);

#endif
