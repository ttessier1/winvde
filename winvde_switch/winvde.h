#pragma once

#define VDE_SOCK_DIR LOCALSTATEDIR"/run"
#define VDE_RC_DIR SYSCONFDIR"/vde2"

#ifndef VDESTDSOCK
#define VDESTDSOCK	"\\windows\\ServiceProfiles\\LocalService\\vde.ctl"
#define VDETMPSOCK	"\\windowws\\temp\\vde.ctl"
#endif

#define DO_SYSLOG
#define VDE_IP_LOG

/*
 * Enable the new packet queueing. Experimental but recommended
 * (expecially with Darwin and other BSDs)
 */
#define VDE_PQ