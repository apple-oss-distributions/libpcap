/*
 * Copyright (c) 1994, 1995, 1996
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef pcap_int_h
#define	pcap_int_h

#include <stddef.h>

#include <signal.h>

#include <pcap/pcap.h>

#ifdef __APPLE__
#include <stdbool.h>
#include <sys/time.h>
#include "pcap/pcap-ng.h"
#include "pcap-util.h"
#endif /* __APPLE__ */

#ifdef MSDOS
  #include <fcntl.h>
  #include <io.h>
#endif

#include "varattrs.h"
#include "fmtutils.h"

#include <stdarg.h>

#include "portability.h"

/*
 * Version string.
 * Uses PACKAGE_VERSION from config.h.
 */
#define PCAP_VERSION_STRING "libpcap version " PACKAGE_VERSION

#ifdef __cplusplus
extern "C" {
#endif

/*
 * If pcap_new_api is set, we disable pcap_lookupdev(), because:
 *
 *    it's not thread-safe, and is marked as deprecated, on all
 *    platforms;
 *
 *    on Windows, it may return UTF-16LE strings, which the program
 *    might then pass to pcap_create() (or to pcap_open_live(), which
 *    then passes them to pcap_create()), requiring pcap_create() to
 *    check for UTF-16LE strings using a hack, and that hack 1)
 *    *cannot* be 100% reliable and 2) runs the risk of going past the
 *    end of the string.
 *
 * We keep it around in legacy mode for compatibility.
 *
 * We also disable the aforementioned hack in pcap_create().
 */
extern int pcap_new_api;

/*
 * If pcap_utf_8_mode is set, on Windows we treat strings as UTF-8.
 *
 * On UN*Xes, we assume all strings are and should be in UTF-8, regardless
 * of the setting of this flag.
 */
extern int pcap_utf_8_mode;

/*
 * Swap byte ordering of unsigned long long timestamp on a big endian
 * machine.
 */
#define SWAPLL(ull)  ((ull & 0xff00000000000000ULL) >> 56) | \
                      ((ull & 0x00ff000000000000ULL) >> 40) | \
                      ((ull & 0x0000ff0000000000ULL) >> 24) | \
                      ((ull & 0x000000ff00000000ULL) >> 8)  | \
                      ((ull & 0x00000000ff000000ULL) << 8)  | \
                      ((ull & 0x0000000000ff0000ULL) << 24) | \
                      ((ull & 0x000000000000ff00ULL) << 40) | \
                      ((ull & 0x00000000000000ffULL) << 56)

/*
 * Maximum snapshot length.
 *
 * Somewhat arbitrary, but chosen to be:
 *
 *    1) big enough for maximum-size Linux loopback packets (65549)
 *       and some USB packets captured with USBPcap:
 *
 *           https://desowin.org/usbpcap/
 *
 *       (> 131072, < 262144)
 *
 * and
 *
 *    2) small enough not to cause attempts to allocate huge amounts of
 *       memory; some applications might use the snapshot length in a
 *       savefile header to control the size of the buffer they allocate,
 *       so a size of, say, 2^31-1 might not work well.  (libpcap uses it
 *       as a hint, but doesn't start out allocating a buffer bigger than
 *       2 KiB, and grows the buffer as necessary, but not beyond the
 *       per-linktype maximum snapshot length.  Other code might naively
 *       use it; we want to avoid writing a too-large snapshot length,
 *       in order not to cause that code problems.)
 *
 * We don't enforce this in pcap_set_snaplen(), but we use it internally.
 */

#ifdef __APPLE__
/*
 * Note: Keep in sync with BPF_MAXBUFSIZE
 */
#define MAXIMUM_SNAPLEN		0x80000
#else
#define MAXIMUM_SNAPLEN		262144
#endif

/*
 * Locale-independent macros for testing character types.
 * These can be passed any integral value, without worrying about, for
 * example, sign-extending char values, unlike the C macros.
 */
#define PCAP_ISDIGIT(c) \
	((c) >= '0' && (c) <= '9')
#define PCAP_ISXDIGIT(c) \
	(((c) >= '0' && (c) <= '9') || \
	 ((c) >= 'A' && (c) <= 'F') || \
	 ((c) >= 'a' && (c) <= 'f'))

struct pcap_opt {
	char	*device;
	int	timeout;	/* timeout for buffering */
	u_int	buffer_size;
	int	promisc;
	int	rfmon;		/* monitor mode */
	int	immediate;	/* immediate mode - deliver packets as soon as they arrive */
	int	nonblock;	/* non-blocking mode - don't wait for packets to be delivered, return "no packets available" */
	int	tstamp_type;
	int	tstamp_precision;

	/*
	 * Platform-dependent options.
	 */
#ifdef __linux__
	int	protocol;	/* protocol to use when creating PF_PACKET socket */
#endif
#ifdef _WIN32
	int	nocapture_local;/* disable NPF loopback */
#endif
};

typedef int	(*activate_op_t)(pcap_t *);
typedef int	(*can_set_rfmon_op_t)(pcap_t *);
typedef int	(*read_op_t)(pcap_t *, int cnt, pcap_handler, u_char *);
typedef int	(*next_packet_op_t)(pcap_t *, struct pcap_pkthdr *, u_char **);
typedef int	(*inject_op_t)(pcap_t *, const void *, int);
typedef void	(*save_current_filter_op_t)(pcap_t *, const char *);
typedef int	(*setfilter_op_t)(pcap_t *, struct bpf_program *);
typedef int	(*setdirection_op_t)(pcap_t *, pcap_direction_t);
typedef int	(*set_datalink_op_t)(pcap_t *, int);
typedef int	(*getnonblock_op_t)(pcap_t *);
typedef int	(*setnonblock_op_t)(pcap_t *, int);
typedef int	(*stats_op_t)(pcap_t *, struct pcap_stat *);
typedef void	(*breakloop_op_t)(pcap_t *);
#ifdef _WIN32
typedef struct pcap_stat *(*stats_ex_op_t)(pcap_t *, int *);
typedef int	(*setbuff_op_t)(pcap_t *, int);
typedef int	(*setmode_op_t)(pcap_t *, int);
typedef int	(*setmintocopy_op_t)(pcap_t *, int);
typedef HANDLE	(*getevent_op_t)(pcap_t *);
typedef int	(*oid_get_request_op_t)(pcap_t *, bpf_u_int32, void *, size_t *);
typedef int	(*oid_set_request_op_t)(pcap_t *, bpf_u_int32, const void *, size_t *);
typedef u_int	(*sendqueue_transmit_op_t)(pcap_t *, pcap_send_queue *, int);
typedef int	(*setuserbuffer_op_t)(pcap_t *, int);
typedef int	(*live_dump_op_t)(pcap_t *, char *, int, int);
typedef int	(*live_dump_ended_op_t)(pcap_t *, int);
typedef PAirpcapHandle	(*get_airpcap_handle_op_t)(pcap_t *);
#endif
typedef void	(*cleanup_op_t)(pcap_t *);
#ifdef __APPLE__
typedef int	(*cleanup_interface_op_t)(const char *, char *);
typedef int	(*send_multiple_op_t)(const char *, const struct pcap_pkthdr **, int);
#endif /* __APPLE__ */
    
/*
 * We put all the stuff used in the read code path at the beginning,
 * to try to keep it together in the same cache line or lines.
 */
struct pcap {
	/*
	 * Method to call to read packets on a live capture.
	 */
	read_op_t read_op;

	/*
	 * Method to call to read the next packet from a savefile.
	 */
	next_packet_op_t next_packet_op;

#ifdef _WIN32
	HANDLE handle;
#else
	int fd;
#endif /* _WIN32 */

	/*
	 * Read buffer.
	 */
	u_int bufsize;
    void *buffer;

	u_char *bp;
	int cc;

	sig_atomic_t break_loop; /* flag set to force break from packet-reading loop */

	void *priv;		/* private data for methods */

#ifdef ENABLE_REMOTE
	struct pcap_samp rmt_samp;	/* parameters related to the sampling process. */
#endif

	int swapped;
	FILE *rfile;		/* null if live capture, non-null if savefile */
	u_int fddipad;
	struct pcap *next;	/* list of open pcaps that need stuff cleared on close */

	/*
	 * File version number; meaningful only for a savefile, but we
	 * keep it here so that apps that (mistakenly) ask for the
	 * version numbers will get the same zero values that they
	 * always did.
	 */
	int version_major;
	int version_minor;

	int snapshot;
	int linktype;		/* Network linktype */
	int linktype_ext;       /* Extended information stored in the linktype field of a file */
	int offset;		/* offset for proper alignment */
	int activated;		/* true if the capture is really started */
	int oldstyle;		/* if we're opening with pcap_open_live() */

	struct pcap_opt opt;

	/*
	 * Place holder for pcap_next().
	 */
	u_char *pkt;

#ifdef _WIN32
	struct pcap_stat stat;		/* used for pcap_stats_ex() */
#endif

	/* We're accepting only packets in this direction/these directions. */
	pcap_direction_t direction;

	/*
	 * Flags to affect BPF code generation.
	 */
	int bpf_codegen_flags;

#if !defined(_WIN32) && !defined(MSDOS)
	int selectable_fd;	/* FD on which select()/poll()/epoll_wait()/kevent()/etc. can be done */

	/*
	 * In case there either is no selectable FD, or there is but
	 * it doesn't necessarily work (e.g., if it doesn't get notified
	 * if the packet capture timeout expires before the buffer
	 * fills up), this points to a timeout that should be used
	 * in select()/poll()/epoll_wait()/kevent() call.  The pcap_t should
	 * be put into non-blocking mode, and, if the timeout expires on
	 * the call, an attempt should be made to read packets from all
	 * pcap_t's with a required timeout, and the code must be
	 * prepared not to see any packets from the attempt.
	 */
	const struct timeval *required_select_timeout;
#endif

	/*
	 * Placeholder for filter code if bpf not in kernel.
	 */
	struct bpf_program fcode;

	char errbuf[PCAP_ERRBUF_SIZE + 1];
#ifdef _WIN32
	char acp_errbuf[PCAP_ERRBUF_SIZE + 1];	/* buffer for local code page error strings */
#endif
	int dlt_count;
	u_int *dlt_list;
	int tstamp_type_count;
	u_int *tstamp_type_list;
	int tstamp_precision_count;
	u_int *tstamp_precision_list;

	struct pcap_pkthdr pcap_header;	/* This is needed for the pcap_next_ex() to work */

	/*
	 * More methods.
	 */
	activate_op_t activate_op;
	can_set_rfmon_op_t can_set_rfmon_op;
	inject_op_t inject_op;
	save_current_filter_op_t save_current_filter_op;
	setfilter_op_t setfilter_op;
	setdirection_op_t setdirection_op;
	set_datalink_op_t set_datalink_op;
	getnonblock_op_t getnonblock_op;
	setnonblock_op_t setnonblock_op;
	stats_op_t stats_op;
	breakloop_op_t breakloop_op;

	/*
	 * Routine to use as callback for pcap_next()/pcap_next_ex().
	 */
	pcap_handler oneshot_callback;

#ifdef _WIN32
	/*
	 * These are, at least currently, specific to the Win32 NPF
	 * driver.
	 */
	stats_ex_op_t stats_ex_op;
	setbuff_op_t setbuff_op;
	setmode_op_t setmode_op;
	setmintocopy_op_t setmintocopy_op;
	getevent_op_t getevent_op;
	oid_get_request_op_t oid_get_request_op;
	oid_set_request_op_t oid_set_request_op;
	sendqueue_transmit_op_t sendqueue_transmit_op;
	setuserbuffer_op_t setuserbuffer_op;
	live_dump_op_t live_dump_op;
	live_dump_ended_op_t live_dump_ended_op;
	get_airpcap_handle_op_t get_airpcap_handle_op;
#endif
	cleanup_op_t cleanup_op;

#ifdef __APPLE__
	/*
	 * Apple additions below
	 */
	int *selectable_fd_list;
	int selectable_fd_count;

	unsigned long packet_read_count; /* only packet type, not other block types */

	/*
	 * The following needs to be 'int' as required by the corresponding BPF ioctls
	 */
	int extendedhdr;
	int wantpktap;
	int truncation;
	int pktaphdrv2;
	int head_drop;

	int compression_mode;
	int compression_enabled;
	u_int compress_head_space;
	void *saved_data_buffer;
	void *prev_datap;
	u_int prev_caplen;
	uint64_t total_read;
	uint64_t total_size;
	uint64_t total_hdr_size;
	uint64_t count_no_common_prefix;
	uint64_t count_common_prefix;
	uint64_t total_common_prefix_size;
	uint8_t max_common_prefix_size;

	cleanup_interface_op_t cleanup_interface_op;
	char *pktap_ifname;
	activate_op_t pktap_activate_op;
	cleanup_op_t pktap_cleanup_op;

	send_multiple_op_t send_multiple_op;
	int send_multiple;
	uint32_t send_bpfhdr_count;
	struct bpf_hdr *send_bpfhdr_array;
	uint32_t send_iovec_count;
	struct iovec *send_iovec_array;

	char *filter_str;
	int shb_added;

	struct pcap_if_info_set if_info_set;

	struct pcap_proc_info_set proc_info_set;

	cleanup_op_t cleanup_extra_op;
#endif /* __APPLE__ */
};

/*
 * BPF code generation flags.
 */
#define BPF_SPECIAL_VLAN_HANDLING	0x00000001	/* special VLAN handling for Linux */

/*
 * This is a timeval as stored in a savefile.
 * It has to use the same types everywhere, independent of the actual
 * `struct timeval'; `struct timeval' has 32-bit tv_sec values on some
 * platforms and 64-bit tv_sec values on other platforms, and writing
 * out native `struct timeval' values would mean files could only be
 * read on systems with the same tv_sec size as the system on which
 * the file was written.
 */

struct pcap_timeval {
    bpf_int32 tv_sec;		/* seconds */
    bpf_int32 tv_usec;		/* microseconds */
};

/*
 * This is a `pcap_pkthdr' as actually stored in a savefile.
 *
 * Do not change the format of this structure, in any way (this includes
 * changes that only affect the length of fields in this structure),
 * and do not make the time stamp anything other than seconds and
 * microseconds (e.g., seconds and nanoseconds).  Instead:
 *
 *	introduce a new structure for the new format;
 *
 *	send mail to "tcpdump-workers@lists.tcpdump.org", requesting
 *	a new magic number for your new capture file format, and, when
 *	you get the new magic number, put it in "savefile.c";
 *
 *	use that magic number for save files with the changed record
 *	header;
 *
 *	make the code in "savefile.c" capable of reading files with
 *	the old record header as well as files with the new record header
 *	(using the magic number to determine the header format).
 *
 * Then supply the changes by forking the branch at
 *
 *	https://github.com/the-tcpdump-group/libpcap/tree/master
 *
 * and issuing a pull request, so that future versions of libpcap and
 * programs that use it (such as tcpdump) will be able to read your new
 * capture file format.
 */

struct pcap_sf_pkthdr {
    struct pcap_timeval ts;	/* time stamp */
    bpf_u_int32 caplen;		/* length of portion present */
    bpf_u_int32 len;		/* length of this packet (off wire) */
};

/*
 * How a `pcap_pkthdr' is actually stored in savefiles written
 * by some patched versions of libpcap (e.g. the ones in Red
 * Hat Linux 6.1 and 6.2).
 *
 * Do not change the format of this structure, in any way (this includes
 * changes that only affect the length of fields in this structure).
 * Instead, introduce a new structure, as per the above.
 */

struct pcap_sf_patched_pkthdr {
    struct pcap_timeval ts;	/* time stamp */
    bpf_u_int32 caplen;		/* length of portion present */
    bpf_u_int32 len;		/* length of this packet (off wire) */
    int		index;
    unsigned short protocol;
    unsigned char pkt_type;
};

/*
 * User data structure for the one-shot callback used for pcap_next()
 * and pcap_next_ex().
 */
struct oneshot_userdata {
	struct pcap_pkthdr *hdr;
	const u_char **pkt;
	pcap_t *pd;
};

#ifndef min
#define min(a, b) ((a) > (b) ? (b) : (a))
#endif

int	pcap_offline_read(pcap_t *, int, pcap_handler, u_char *);

#ifdef __APPLE__
int pcap_ng_offline_read(pcap_t *, int , pcap_handler , u_char *);
void pcap_ng_cleanup(pcap_t *);
    
struct pcap_dumper {
	FILE *f;

	int shb_added;
	pcapng_block_t dump_block;

	struct pcap_if_info_set dump_if_info_set;

	struct pcap_proc_info_set dump_proc_info_set;
};

pcap_dumper_t *pcap_alloc_dumper(pcap_t *, FILE *);

void pcap_darwin_cleanup(pcap_t *);
#endif /* __APPLE__ */

/*
 * Does the packet count argument to a module's read routine say
 * "supply packets until you run out of packets"?
 */
#define PACKET_COUNT_IS_UNLIMITED(count)	((count) <= 0)

/*
 * Routines that most pcap implementations can use for non-blocking mode.
 */
#if !defined(_WIN32) && !defined(MSDOS)
int	pcap_getnonblock_fd(pcap_t *);
int	pcap_setnonblock_fd(pcap_t *p, int);
#endif

/*
 * Internal interfaces for "pcap_create()".
 *
 * "pcap_create_interface()" is the routine to do a pcap_create on
 * a regular network interface.  There are multiple implementations
 * of this, one for each platform type (Linux, BPF, DLPI, etc.),
 * with the one used chosen by the configure script.
 *
 * "pcap_create_common()" allocates and fills in a pcap_t, for use
 * by pcap_create routines.
 */
pcap_t	*pcap_create_interface(const char *, char *);

/*
 * This wrapper takes an error buffer pointer and a type to use for the
 * private data, and calls pcap_create_common(), passing it the error
 * buffer pointer, the size fo the private data type, in bytes, and the
 * offset of the private data from the beginning of the structure, in
 * bytes.
 */
#define PCAP_CREATE_COMMON(ebuf, type) \
	pcap_create_common(ebuf, \
	    sizeof (struct { pcap_t __common; type __private; }), \
	    offsetof (struct { pcap_t __common; type __private; }, __private))
pcap_t	*pcap_create_common(char *, size_t, size_t);
int	pcap_do_addexit(pcap_t *);
void	pcap_add_to_pcaps_to_close(pcap_t *);
void	pcap_remove_from_pcaps_to_close(pcap_t *);
void	pcap_cleanup_live_common(pcap_t *);
int	pcap_check_activated(pcap_t *);
void	pcap_breakloop_common(pcap_t *);

/*
 * Internal interfaces for "pcap_findalldevs()".
 *
 * A pcap_if_list_t * is a reference to a list of devices.
 *
 * A get_if_flags_func is a platform-dependent function called to get
 * additional interface flags.
 *
 * "pcap_platform_finddevs()" is the platform-dependent routine to
 * find local network interfaces.
 *
 * "pcap_findalldevs_interfaces()" is a helper to find those interfaces
 * using the "standard" mechanisms (SIOCGIFCONF, "getifaddrs()", etc.).
 *
 * "add_dev()" adds an entry to a pcap_if_list_t.
 *
 * "find_dev()" tries to find a device, by name, in a pcap_if_list_t.
 *
 * "find_or_add_dev()" checks whether a device is already in a pcap_if_list_t
 * and, if not, adds an entry for it.
 */
struct pcap_if_list;
typedef struct pcap_if_list pcap_if_list_t;
typedef int (*get_if_flags_func)(const char *, bpf_u_int32 *, char *);
int	pcap_platform_finddevs(pcap_if_list_t *, char *);
#if !defined(_WIN32) && !defined(MSDOS)
int	pcap_findalldevs_interfaces(pcap_if_list_t *, char *,
	    int (*)(const char *), get_if_flags_func);
#endif
pcap_if_t *find_or_add_dev(pcap_if_list_t *, const char *, bpf_u_int32,
	    get_if_flags_func, const char *, char *);
pcap_if_t *find_dev(pcap_if_list_t *, const char *);
pcap_if_t *add_dev(pcap_if_list_t *, const char *, bpf_u_int32, const char *,
	    char *);
int	add_addr_to_dev(pcap_if_t *, struct sockaddr *, size_t,
	    struct sockaddr *, size_t, struct sockaddr *, size_t,
	    struct sockaddr *dstaddr, size_t, char *errbuf);
#ifndef _WIN32
pcap_if_t *find_or_add_if(pcap_if_list_t *, const char *, bpf_u_int32,
	    get_if_flags_func, char *);
int	add_addr_to_if(pcap_if_list_t *, const char *, bpf_u_int32,
	    get_if_flags_func,
	    struct sockaddr *, size_t, struct sockaddr *, size_t,
	    struct sockaddr *, size_t, struct sockaddr *, size_t, char *);
#endif

/*
 * Internal interfaces for "pcap_open_offline()" and other savefile
 * I/O routines.
 *
 * "pcap_open_offline_common()" allocates and fills in a pcap_t, for use
 * by pcap_open_offline routines.
 *
 * "pcap_adjust_snapshot()" adjusts the snapshot to be non-zero and
 * fit within an int.
 *
 * "sf_cleanup()" closes the file handle associated with a pcap_t, if
 * appropriate, and frees all data common to all modules for handling
 * savefile types.
 *
 * "charset_fopen()", in UTF-8 mode on Windows, does an fopen() that
 * treats the pathname as being in UTF-8, rather than the local
 * code page, on Windows.
 */

/*
 * This wrapper takes an error buffer pointer and a type to use for the
 * private data, and calls pcap_create_common(), passing it the error
 * buffer pointer, the size fo the private data type, in bytes, and the
 * offset of the private data from the beginning of the structure, in
 * bytes.
 */
#define PCAP_OPEN_OFFLINE_COMMON(ebuf, type) \
	pcap_open_offline_common(ebuf, \
	    sizeof (struct { pcap_t __common; type __private; }), \
	    offsetof (struct { pcap_t __common; type __private; }, __private))
pcap_t	*pcap_open_offline_common(char *ebuf, size_t total_size,
    size_t private_data);
bpf_u_int32 pcap_adjust_snapshot(bpf_u_int32 linktype, bpf_u_int32 snaplen);
void	sf_cleanup(pcap_t *p);
#ifdef _WIN32
FILE	*charset_fopen(const char *path, const char *mode);
#else
/*
 * On other OSes, just use Boring Old fopen().
 */
#define charset_fopen(path, mode)	fopen((path), (mode))
#endif

/*
 * Internal interfaces for loading code at run time.
 */
#ifdef _WIN32
#define pcap_code_handle_t	HMODULE
#define pcap_funcptr_t		FARPROC

pcap_code_handle_t	pcap_load_code(const char *);
pcap_funcptr_t		pcap_find_function(pcap_code_handle_t, const char *);
#endif

/*
 * Internal interfaces for doing user-mode filtering of packets and
 * validating filter programs.
 */
/*
 * Auxiliary data, for use when interpreting a filter intended for the
 * Linux kernel when the kernel rejects the filter (requiring us to
 * run it in userland).  It contains VLAN tag information.
 */
struct pcap_bpf_aux_data {
	u_short vlan_tag_present;
	u_short vlan_tag;
};

/*
 * Filtering routine that takes the auxiliary data as an additional
 * argument.
 */
u_int	pcap_filter_with_aux_data(const struct bpf_insn *,
    const u_char *, u_int, u_int, const struct pcap_bpf_aux_data *);

/*
 * Filtering routine that doesn't.
 */
u_int	pcap_filter(const struct bpf_insn *, const u_char *, u_int, u_int);

/*
 * Routine to validate a BPF program.
 */
int	pcap_validate_filter(const struct bpf_insn *, int);

/*
 * Internal interfaces for both "pcap_create()" and routines that
 * open savefiles.
 *
 * "pcap_oneshot()" is the standard one-shot callback for "pcap_next()"
 * and "pcap_next_ex()".
 */
void	pcap_oneshot(u_char *, const struct pcap_pkthdr *, const u_char *);

int	install_bpf_program(pcap_t *, struct bpf_program *);

int	pcap_strcasecmp(const char *, const char *);

/*
 * Internal interfaces for pcap_createsrcstr and pcap_parsesrcstr with
 * the additional bit of information regarding SSL support (rpcap:// vs.
 * rpcaps://).
 */
int	pcap_createsrcstr_ex(char *, int, const char *, const char *,
    const char *, unsigned char, char *);
int	pcap_parsesrcstr_ex(const char *, int *, char *, char *,
    char *, unsigned char *, char *);

#ifdef YYDEBUG
extern int pcap_debug;
#endif

#ifdef __cplusplus
}
#endif

#endif
