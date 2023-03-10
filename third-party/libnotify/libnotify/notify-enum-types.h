#pragma once

            #include <glib-object.h>


            G_BEGIN_DECLS

/* enumerations from "notification.h" */


GType notify_urgency_get_type (void);
#define NOTIFY_TYPE_URGENCY (notify_urgency_get_type())


GType notify_closed_reason_get_type (void);
#define NOTIFY_TYPE_CLOSED_REASON (notify_closed_reason_get_type())

G_END_DECLS
