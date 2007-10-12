/*	$OpenBSD: hce.c,v 1.31 2007/10/12 12:50:59 blambert Exp $	*/

/*
 * Copyright (c) 2006 Pierre-Yves Ritschard <pyr@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/queue.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <errno.h>
#include <event.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <pwd.h>

#include <openssl/ssl.h>

#include "hoststated.h"

__dead void hce_shutdown(void);
void	hce_sig_handler(int sig, short, void *);
void	hce_dispatch_imsg(int, short, void *);
void	hce_dispatch_parent(int, short, void *);
void	hce_launch_checks(int, short, void *);
void	hce_setup_events(void);
void	hce_disable_events(void);

static struct hoststated *env = NULL;
struct imsgbuf		*ibuf_pfe;
struct imsgbuf		*ibuf_main;
int			 pipe_pfe;
int			 pipe_parent;
int			 running = 0;

void
hce_sig_handler(int sig, short event, void *arg)
{
	switch (sig) {
	case SIGINT:
	case SIGTERM:
		hce_shutdown();
		break;
	default:
		fatalx("hce_sig_handler: unexpected signal");
	}
}

pid_t
hce(struct hoststated *x_env, int pipe_parent2pfe[2], int pipe_parent2hce[2],
    int pipe_parent2relay[RELAY_MAXPROC][2], int pipe_pfe2hce[2],
    int pipe_pfe2relay[RELAY_MAXPROC][2])
{
	pid_t		 pid;
	struct passwd	*pw;
	int		 i;
	struct event	 ev_sigint;
	struct event	 ev_sigterm;

	switch (pid = fork()) {
	case -1:
		fatal("hce: cannot fork");
	case 0:
		break;
	default:
		return (pid);
	}

	env = x_env;
	purge_config(env, PURGE_SERVICES|PURGE_RELAYS|PURGE_PROTOS);

	if ((pw = getpwnam(HOSTSTATED_USER)) == NULL)
		fatal("hce: getpwnam");

#ifndef DEBUG
	if (chroot(pw->pw_dir) == -1)
		fatal("hce: chroot");
	if (chdir("/") == -1)
		fatal("hce: chdir(\"/\")");
#else
#warning disabling privilege revocation and chroot in DEBUG mode
#endif

	setproctitle("host check engine");
	hoststated_process = PROC_HCE;

	/* this is needed for icmp tests */
	icmp_init(env);

#ifndef DEBUG
	if (setgroups(1, &pw->pw_gid) ||
	    setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) ||
	    setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid))
		fatal("hce: can't drop privileges");
#endif

	event_init();

	if ((ibuf_pfe = calloc(1, sizeof(struct imsgbuf))) == NULL ||
	    (ibuf_main = calloc(1, sizeof(struct imsgbuf))) == NULL)
		fatal("hce");
	imsg_init(ibuf_pfe, pipe_pfe2hce[0], hce_dispatch_imsg);
	imsg_init(ibuf_main, pipe_parent2hce[1], hce_dispatch_parent);

	ibuf_pfe->events = EV_READ;
	event_set(&ibuf_pfe->ev, ibuf_pfe->fd, ibuf_pfe->events,
	    ibuf_pfe->handler, ibuf_pfe);
	event_add(&ibuf_pfe->ev, NULL);

	ibuf_main->events = EV_READ;
	event_set(&ibuf_main->ev, ibuf_main->fd, ibuf_main->events,
	    ibuf_main->handler, ibuf_main);
	event_add(&ibuf_main->ev, NULL);

	signal_set(&ev_sigint, SIGINT, hce_sig_handler, NULL);
	signal_set(&ev_sigterm, SIGTERM, hce_sig_handler, NULL);
	signal_add(&ev_sigint, NULL);
	signal_add(&ev_sigterm, NULL);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	/* setup pipes */
	close(pipe_pfe2hce[1]);
	close(pipe_parent2hce[0]);
	close(pipe_parent2pfe[0]);
	close(pipe_parent2pfe[1]);
	for (i = 0; i < env->prefork_relay; i++) {
		close(pipe_parent2relay[i][0]);
		close(pipe_parent2relay[i][1]);
		close(pipe_pfe2relay[i][0]);
		close(pipe_pfe2relay[i][1]);
	}

	hce_setup_events();
	event_dispatch();
	hce_shutdown();

	return (0);
}

void
hce_setup_events(void)
{
	struct timeval	 tv;
	struct table	*table;

	if (!TAILQ_EMPTY(env->tables)) {
		evtimer_set(&env->ev, hce_launch_checks, env);
		bzero(&tv, sizeof(tv));
		evtimer_add(&env->ev, &tv);
	}

	if (env->flags & F_SSL) {
		ssl_init(env);
		TAILQ_FOREACH(table, env->tables, entry) {
			if (!(table->conf.flags & F_SSL))
				continue;
			table->ssl_ctx = ssl_ctx_create(env);
		}
	}
}

void
hce_disable_events(void)
{
	struct table	*table;
	struct host	*host;

	evtimer_del(&env->ev);
	TAILQ_FOREACH(table, env->tables, entry) {
		TAILQ_FOREACH(host, &table->hosts, entry) {
			event_del(&host->cte.ev);
			close(host->cte.s);
		}
	}
	if (env->has_icmp) {
		event_del(&env->icmp_send.ev);
		event_del(&env->icmp_recv.ev);
	}
	if (env->has_icmp6) {
		event_del(&env->icmp6_send.ev);
		event_del(&env->icmp6_recv.ev);
	}
}

void
hce_launch_checks(int fd, short event, void *arg)
{
	struct host		*host;
	struct table		*table;
	struct timeval		 tv;

	/*
	 * notify pfe checks are done and schedule next check
	 */
	imsg_compose(ibuf_pfe, IMSG_SYNC, 0, 0, -1, NULL, 0);
	TAILQ_FOREACH(table, env->tables, entry) {
		TAILQ_FOREACH(host, &table->hosts, entry) {
			host->flags &= ~(F_CHECK_SENT|F_CHECK_DONE);
			event_del(&host->cte.ev);
		}
	}

	if (gettimeofday(&tv, NULL))
		fatal("hce_launch_checks: gettimeofday");

	TAILQ_FOREACH(table, env->tables, entry) {
		if (table->conf.flags & F_DISABLE)
			continue;
		if (table->conf.check == CHECK_NOCHECK)
			fatalx("hce_launch_checks: unknown check type");

		TAILQ_FOREACH(host, &table->hosts, entry) {
			if (host->flags & F_DISABLE)
				continue;
			switch (table->conf.check) {
			case CHECK_ICMP:
				schedule_icmp(env, host);
				break;
			case CHECK_SCRIPT:
				check_script(host);
				break;
			default:
				/* Any other TCP-style checks */
				bzero(&host->cte, sizeof(host->cte));
				host->last_up = host->up;
				host->cte.host = host;
				host->cte.table = table;
				bcopy(&tv, &host->cte.tv_start,
				    sizeof(host->cte.tv_start));
				check_tcp(&host->cte);
				break;
			}
		}
	}
	check_icmp(env, &tv);

	bcopy(&env->interval, &tv, sizeof(tv));
	evtimer_add(&env->ev, &tv);
}

void
hce_notify_done(struct host *host, const char *msg)
{
	struct table		*table;
	struct ctl_status	 st;
	struct timeval		 tv_now, tv_dur;
	u_long			 duration;
	u_int			 logopt;

	if (host->up == HOST_DOWN && host->retry_cnt) {
		log_debug("hce_notify_done: host %s retry %d",
		    host->conf.name, host->retry_cnt);
		host->up = host->last_up;
		host->retry_cnt--;
	} else
		host->retry_cnt = host->conf.retry;
	if (host->up != HOST_UNKNOWN) {
		host->check_cnt++;
		if (host->up == HOST_UP)
			host->up_cnt++;
	}
	st.id = host->conf.id;
	st.up = host->up;
	st.check_cnt = host->check_cnt;
	st.retry_cnt = host->retry_cnt;
	host->flags |= (F_CHECK_SENT|F_CHECK_DONE);
	if (msg)
		log_debug("hce_notify_done: %s (%s)", host->conf.name, msg);

	imsg_compose(ibuf_pfe, IMSG_HOST_STATUS, 0, 0, -1, &st, sizeof(st));
	if (host->up != host->last_up)
		logopt = HOSTSTATED_OPT_LOGUPDATE;
	else
		logopt = HOSTSTATED_OPT_LOGNOTIFY;

	if (gettimeofday(&tv_now, NULL))
		fatal("hce_notify_done: gettimeofday");
	timersub(&tv_now, &host->cte.tv_start, &tv_dur);
	if (timercmp(&host->cte.tv_start, &tv_dur, >))
		duration = (tv_dur.tv_sec * 1000) + (tv_dur.tv_usec / 1000.0);
	else
		duration = 0;

	if ((table = table_find(env, host->conf.tableid)) == NULL)
		fatalx("hce_notify_done: invalid table id");

	if (env->opts & logopt) {
		log_info("host %s, check %s%s (%lums), state %s -> %s, "
		    "availability %s",
		    host->conf.name, table_check(table->conf.check),
		    (table->conf.flags & F_SSL) ? " use ssl" : "", duration,
		    host_status(host->last_up), host_status(host->up),
		    print_availability(host->check_cnt, host->up_cnt));
	}
	host->last_up = host->up;
}

void
hce_shutdown(void)
{
	log_info("host check engine exiting");
	_exit(0);
}

void
hce_dispatch_imsg(int fd, short event, void *ptr)
{
	struct imsgbuf		*ibuf;
	struct imsg		 imsg;
	ssize_t			 n;
	objid_t			 id;
	struct host		*host;
	struct table		*table;

	ibuf = ptr;
	switch (event) {
	case EV_READ:
		if ((n = imsg_read(ibuf)) == -1)
			fatal("hce_dispatch_imsg: imsg_read_error");
		if (n == 0) {
			/* this pipe is dead, so remove the event handler */
			event_del(&ibuf->ev);
			event_loopexit(NULL);
			return;
		}
		break;
	case EV_WRITE:
		if (msgbuf_write(&ibuf->w) == -1)
			fatal("hce_dispatch_imsg: msgbuf_write");
		imsg_event_add(ibuf);
		return;
	default:
		fatalx("hce_dispatch_imsg: unknown event");
	}

	for (;;) {
		if ((n = imsg_get(ibuf, &imsg)) == -1)
			fatal("hce_dispatch_imsg: imsg_read error");
		if (n == 0)
			break;

		switch (imsg.hdr.type) {
		case IMSG_HOST_DISABLE:
			memcpy(&id, imsg.data, sizeof(id));
			if ((host = host_find(env, id)) == NULL)
				fatalx("hce_dispatch_imsg: desynchronized");
			host->flags |= F_DISABLE;
			host->up = HOST_UNKNOWN;
			host->check_cnt = 0;
			host->up_cnt = 0;
			break;
		case IMSG_HOST_ENABLE:
			memcpy(&id, imsg.data, sizeof(id));
			if ((host = host_find(env, id)) == NULL)
				fatalx("hce_dispatch_imsg: desynchronized");
			host->flags &= ~(F_DISABLE);
			host->up = HOST_UNKNOWN;
			break;
		case IMSG_TABLE_DISABLE:
			memcpy(&id, imsg.data, sizeof(id));
			if ((table = table_find(env, id)) == NULL)
				fatalx("hce_dispatch_imsg: desynchronized");
			table->conf.flags |= F_DISABLE;
			TAILQ_FOREACH(host, &table->hosts, entry)
				host->up = HOST_UNKNOWN;
			break;
		case IMSG_TABLE_ENABLE:
			memcpy(&id, imsg.data, sizeof(id));
			if ((table = table_find(env, id)) == NULL)
				fatalx("hce_dispatch_imsg: desynchronized");
			table->conf.flags &= ~(F_DISABLE);
			TAILQ_FOREACH(host, &table->hosts, entry)
				host->up = HOST_UNKNOWN;
			break;
		default:
			log_debug("hce_dispatch_msg: unexpected imsg %d",
			    imsg.hdr.type);
			break;
		}
		imsg_free(&imsg);
	}
	imsg_event_add(ibuf);
}

void
hce_dispatch_parent(int fd, short event, void * ptr)
{
	struct imsgbuf		*ibuf;
	struct imsg		 imsg;
	struct ctl_script	 scr;
	ssize_t		 	 n;
	size_t			 len;

	static struct table	*table = NULL;
	struct host		*host;

	ibuf = ptr;
	switch (event) {
	case EV_READ:
		if ((n = imsg_read(ibuf)) == -1)
			fatal("hce_dispatch_parent: imsg_read error");
		if (n == 0) {
			/* this pipe is dead, so remove the event handler */
			event_del(&ibuf->ev);
			event_loopexit(NULL);
			return;
		}
		break;
	case EV_WRITE:
		if (msgbuf_write(&ibuf->w) == -1)
			fatal("hce_dispatch_parent: msgbuf_write");
		imsg_event_add(ibuf);
		return;
	default:
		fatalx("hce_dispatch_parent: unknown event");
	}

	for (;;) {
		if ((n = imsg_get(ibuf, &imsg)) == -1)
			fatal("hce_dispatch_parent: imsg_read error");
		if (n == 0)
			break;

		switch (imsg.hdr.type) {
		case IMSG_SCRIPT:
			if (imsg.hdr.len - IMSG_HEADER_SIZE !=
			    sizeof(scr))
				fatalx("hce_dispatch_parent: "
				    "invalid size of script request");
			bcopy(imsg.data, &scr, sizeof(scr));
			script_done(env, &scr);
			break;
		case IMSG_RECONF:
			log_debug("hce: reloading configuration");
			if (imsg.hdr.len !=
			    sizeof(struct hoststated) + IMSG_HEADER_SIZE)
				fatalx("corrupted reload data");
			hce_disable_events();
			purge_config(env, PURGE_TABLES);
			merge_config(env, (struct hoststated *)imsg.data);

			env->tables = calloc(1, sizeof(*env->tables));
			if (env->tables == NULL)
				fatal(NULL);

			TAILQ_INIT(env->tables);
			break;
		case IMSG_RECONF_TABLE:
			if ((table = calloc(1, sizeof(*table))) == NULL)
				fatal(NULL);
			memcpy(&table->conf, imsg.data, sizeof(table->conf));
			TAILQ_INIT(&table->hosts);
			TAILQ_INSERT_TAIL(env->tables, table, entry);
			break;
		case IMSG_RECONF_SENDBUF:
			len = imsg.hdr.len - IMSG_HEADER_SIZE;
			table->sendbuf = calloc(1, len);
			(void)strlcpy(table->sendbuf, (char *)imsg.data, len);
			break;
		case IMSG_RECONF_HOST:
			if ((host = calloc(1, sizeof(*host))) == NULL)
				fatal(NULL);
			memcpy(&host->conf, imsg.data, sizeof(host->conf));
			host->tablename = table->conf.name;
			TAILQ_INSERT_TAIL(&table->hosts, host, entry);
			break;
		case IMSG_RECONF_END:
			log_warnx("hce: configuration reloaded");
			hce_setup_events();
			break;
		default:
			log_debug("hce_dispatch_parent: unexpected imsg %d",
			    imsg.hdr.type);
			break;
		}
		imsg_free(&imsg);
	}
	imsg_event_add(ibuf);
}
