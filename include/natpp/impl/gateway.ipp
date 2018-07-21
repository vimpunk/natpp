#ifndef NATPP_GATEWAY_IMPL
#define NATPP_GATEWAY_IMPL

#if defined(__linux__)
# define NATPP_USE_PROC_NET
#elif defined(BSD) || defined(__FreeBSD_kernel__) || (defined(sun) && defined(__SVR4))
# define NATPP_USE_SOCKET
#elif defined(__APPLE_)
# define NATPP_USE_SYCTL_NET
#elif defined(__HAIKU__)
# define NATPP_USE_HAIKU
//# include <cstdlib>
//# include <cunistd>
# include <net/if.h>
# include <sys/sockio.h>
#elif WIN32
# define NATPP_USE_WIN
# include <windows.h>
# include <iphlpapi.h>
# include <winsock2.h>
#endif

#include <fstream>
#include <sstream>

#include <endian/endian.hpp>
#include <asio/error.hpp>
#include <asio/ip/address.hpp>

namespace nat {

inline asio::ip::address default_gateway_address(error_code& error)
{
#ifdef NATPP_USE_PROC_NET
/* Example route file:
Iface	Destination	Gateway 	Flags	RefCnt	Use	Metric	Mask		MTU	Window	IRTT                                                       
wlp7s0	00000000	0100A8C0	0003	0	0	600	00000000	0	0	0                                                                           
wlp7s0	0000A8C0	00000000	0001	0	0	600	00FFFFFF	0	0	0                                                                           
*/
    std::ifstream file("/proc/net/route");
    if(!file)
    {
        error = std::make_error_code(std::errc::bad_file_descriptor);
        return asio::ip::address_v4(0);
    }
    std::string line;
    // Ignore first line. TODO optimize
    std::getline(file, line);
    while(std::getline(file, line))
    {
        std::stringstream ss;
        uint32_t dest;
        uint32_t gateway;
        // Ignore the interface identifier by trimming up to the first whitespace char.
        line.erase(line.cbegin(), std::find_if(line.cbegin(), line.cend(),
            [](const char c) { return std::isspace(c); }));
        ss << std::hex;
        ss << line;
        ss >> dest;
        ss >> gateway;
        if((dest == 0) && (gateway != 0))
        {
            // /proc/net/route stores numbers in the architecture's byte order,
            // but since stringstream assumes big endian order, we have to
            // manually convert it if the system is not big endian.
            if(endian::order::host == endian::order::little)
                return asio::ip::address_v4(endian::reverse(gateway));
            else
                return asio::ip::address_v4(gateway);
        }
    }
    return asio::ip::address_v4(0);
}

#elif NATPP_USE_SOCKET
/* Thanks to Darren Kenny for this code */
#define NEXTADDR(w, u) \
        if (rtm_addrs & (w)) {\
            l = sizeof(struct sockaddr); memmove(cp, &(u), l); cp += l;\
        }

#define rtm m_rtmsg.m_rtm

struct {
  struct rt_msghdr m_rtm;
  char       m_space[512];
} m_rtmsg;

int getdefaultgateway(in_addr_t *addr)
{
  int s, seq, l, rtm_addrs, i;
  pid_t pid;
  struct sockaddr so_dst, so_mask;
  char *cp = m_rtmsg.m_space; 
  struct sockaddr *gate = NULL, *sa;
  struct rt_msghdr *msg_hdr;

  pid = getpid();
  seq = 0;
  rtm_addrs = RTA_DST | RTA_NETMASK;

  memset(&so_dst, 0, sizeof(so_dst));
  memset(&so_mask, 0, sizeof(so_mask));
  memset(&rtm, 0, sizeof(struct rt_msghdr));

  rtm.rtm_type = RTM_GET;
  rtm.rtm_flags = RTF_UP | RTF_GATEWAY;
  rtm.rtm_version = RTM_VERSION;
  rtm.rtm_seq = ++seq;
  rtm.rtm_addrs = rtm_addrs; 

  so_dst.sa_family = AF_INET;
  so_mask.sa_family = AF_INET;

  NEXTADDR(RTA_DST, so_dst);
  NEXTADDR(RTA_NETMASK, so_mask);

  rtm.rtm_msglen = l = cp - (char *)&m_rtmsg;

  s = socket(PF_ROUTE, SOCK_RAW, 0);

  if (write(s, (char *)&m_rtmsg, l) < 0) {
      close(s);
      return FAILED;
  }

  do {
    l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
  } while (l > 0 && (rtm.rtm_seq != seq || rtm.rtm_pid != pid));
                        
  close(s);

  msg_hdr = &rtm;

  cp = ((char *)(msg_hdr + 1));
  if (msg_hdr->rtm_addrs) {
    for (i = 1; i; i <<= 1)
      if (i & msg_hdr->rtm_addrs) {
        sa = (struct sockaddr *)cp;
        if (i == RTA_GATEWAY )
          gate = sa;

        cp += sizeof(struct sockaddr);
      }
  } else {
      return FAILED;
  }


  if (gate != NULL ) {
      *addr = ((struct sockaddr_in *)gate)->sin_addr.s_addr;
      return SUCCESS;
  } else {
      return FAILED;
  }
}
#elif NATPP_USE_SYCTL_NET
#elif USE_WIN_CODE
#endif

} // nat

#endif // NATPP_GATEWAY_IMPL


#if 0
/* $Id: getgateway.c,v 1.22 2011/08/08 21:20:51 nanard Exp $ */
/* libnatpmp

Copyright (c) 2007-2011, Thomas BERNARD 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * The name of the author may not be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <ctype.h>
#ifndef WIN32
#include <netinet/in.h>
#endif
#if !defined(_MSC_VER)
#include <sys/param.h>
#endif

#ifdef WIN32
# include <stdint.h>
# define in_addr_t uint32_t
#else
# define in_addr_t in_addr
#endif

/* There is no portable method to get the default route gateway.
 * So below are four (or five ?) differents functions implementing this.
 * Parsing /proc/net/route is for linux.
 * sysctl is the way to access such informations on BSD systems.
 * Many systems should provide route information through raw PF_ROUTE
 * sockets.
 * In MS Windows, default gateway is found by looking into the registry
 * or by using GetBestRoute(). */
#ifdef __linux__
#define USE_PROC_NET_ROUTE
#undef USE_SOCKET_ROUTE
#undef USE_SYSCTL_NET_ROUTE
#endif

#if defined(BSD) || defined(__FreeBSD_kernel__)
#undef USE_PROC_NET_ROUTE
#define USE_SOCKET_ROUTE
#undef USE_SYSCTL_NET_ROUTE
#endif

#if (defined(sun) && defined(__SVR4))
#undef USE_PROC_NET_ROUTE
#define USE_SOCKET_ROUTE
#undef USE_SYSCTL_NET_ROUTE
#endif

#ifdef __APPLE__
#undef USE_PROC_NET_ROUTE
#undef USE_SOCKET_ROUTE
#define USE_SYSCTL_NET_ROUTE
#endif

#ifdef WIN32
#undef USE_PROC_NET_ROUTE
#undef USE_SOCKET_ROUTE
#undef USE_SYSCTL_NET_ROUTE
//#define USE_WIN32_CODE
#define USE_WIN32_CODE_2
#endif

#ifdef __CYGWIN__
#undef USE_PROC_NET_ROUTE
#undef USE_SOCKET_ROUTE
#undef USE_SYSCTL_NET_ROUTE
#define USE_WIN32_CODE
#include <stdarg.h>
#include <w32api/windef.h>
#include <w32api/winbase.h>
#include <w32api/winreg.h>
#endif 

#ifdef __HAIKU__
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/sockio.h>
#define USE_HAIKU_CODE
#endif 

#ifdef USE_SYSCTL_NET_ROUTE
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <net/route.h>
#endif
#ifdef USE_SOCKET_ROUTE
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#endif

#ifdef USE_WIN32_CODE
#include <unknwn.h>
#include <winreg.h>
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_LENGTH 16383
#endif

#ifdef USE_WIN32_CODE_2
#include <windows.h>
#include <iphlpapi.h>
#include <winsock2.h>
#endif

#ifndef WIN32
#define SUCCESS (0)
#define FAILED  (-1)
#endif

#ifdef USE_PROC_NET_ROUTE
/*
 parse /proc/net/route which is as follow :

Iface   Destination     Gateway         Flags   RefCnt  Use     Metric  Mask            MTU     Window  IRTT           
wlan0   0001A8C0        00000000        0001    0       0       0       00FFFFFF        0       0       0              
eth0    0000FEA9        00000000        0001    0       0       0       0000FFFF        0       0       0              
wlan0   00000000        0101A8C0        0003    0       0       0       00000000        0       0       0              
eth0    00000000        00000000        0001    0       0       1000    00000000        0       0       0              

 One header line, and then one line by route by route table entry.
*/
int getdefaultgateway(in_addr_t * addr)
{
	unsigned long d, g;
	char buf[256];
	int line = 0;
	FILE * f;
	char * p;
	f = fopen("/proc/net/route", "r");
	if(!f)
		return FAILED;
	while(fgets(buf, sizeof(buf), f)) {
		if(line > 0) {	/* skip the first line */
			p = buf;
			/* skip the interface name */
			while(*p && !isspace(*p))
				p++;
			while(*p && isspace(*p))
				p++;
			if(sscanf(p, "%lx%lx", &d, &g)==2) {
				if(d == 0 && g != 0) { /* default */
					*addr = g;
					fclose(f);
					return SUCCESS;
				}
			}
		}
		line++;
	}
	/* default route not found ! */
	if(f)
		fclose(f);
	return FAILED;
}
#endif /* #ifdef USE_PROC_NET_ROUTE */


#ifdef USE_SYSCTL_NET_ROUTE

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

int getdefaultgateway(in_addr_t * addr)
{
#if 0
	/* net.route.0.inet.dump.0.0 ? */
	int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET,
	             NET_RT_DUMP, 0, 0/*tableid*/};
#endif
	/* net.route.0.inet.flags.gateway */
	int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET,
	             NET_RT_FLAGS, RTF_GATEWAY};
	size_t l;
	char * buf, * p;
	struct rt_msghdr * rt;
	struct sockaddr * sa;
	struct sockaddr * sa_tab[RTAX_MAX];
	int i;
	int r = FAILED;
	if(sysctl(mib, sizeof(mib)/sizeof(int), 0, &l, 0, 0) < 0) {
		return FAILED;
	}
	if(l>0) {
		buf = malloc(l);
		if(sysctl(mib, sizeof(mib)/sizeof(int), buf, &l, 0, 0) < 0) {
			free(buf);
			return FAILED;
		}
		for(p=buf; p<buf+l; p+=rt->rtm_msglen) {
			rt = (struct rt_msghdr *)p;
			sa = (struct sockaddr *)(rt + 1);
			for(i=0; i<RTAX_MAX; i++) {
				if(rt->rtm_addrs & (1 << i)) {
					sa_tab[i] = sa;
					sa = (struct sockaddr *)((char *)sa + ROUNDUP(sa->sa_len));
				} else {
					sa_tab[i] = NULL;
				}
			}
			if( ((rt->rtm_addrs & (RTA_DST|RTA_GATEWAY)) == (RTA_DST|RTA_GATEWAY))
              && sa_tab[RTAX_DST]->sa_family == AF_INET
              && sa_tab[RTAX_GATEWAY]->sa_family == AF_INET) {
				if(((struct sockaddr_in *)sa_tab[RTAX_DST])->sin_addr.s_addr == 0) {
					*addr = ((struct sockaddr_in *)(sa_tab[RTAX_GATEWAY]))->sin_addr.s_addr;
					r = SUCCESS;
				}
			}
		}
		free(buf);
	}
	return r;
}
#endif /* #ifdef USE_SYSCTL_NET_ROUTE */


#ifdef USE_SOCKET_ROUTE
#endif /* #ifdef USE_SOCKET_ROUTE */

#ifdef USE_WIN32_CODE
int getdefaultgateway(in_addr_t *addr)
{
	MIB_IPFORWARDROW ip_forward;
	memset(&ip_forward, 0, sizeof(ip_forward));
	if(GetBestRoute(inet_addr("0.0.0.0"), 0, &ip_forward) != NO_ERROR)
		return -1;
	*addr = ip_forward.dwForwardNextHop;
	return 0;
}
#endif /* #ifdef USE_WIN32_CODE_2 */

#ifdef NATPP_USE_HAIKU
int getdefaultgateway(in_addr_t *addr)
{
    int fd, ret = -1;
    struct ifconf config;
    void *buffer = NULL;
    struct ifreq *interface;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }
    if (ioctl(fd, SIOCGRTSIZE, &config, sizeof(config)) != 0) {
        goto fail;
    }
    if (config.ifc_value < 1) {
        goto fail; /* No routes */
    }
    if ((buffer = malloc(config.ifc_value)) == NULL) {
        goto fail;
    }
    config.ifc_len = config.ifc_value;
    config.ifc_buf = buffer;
    if (ioctl(fd, SIOCGRTTABLE, &config, sizeof(config)) != 0) {
        goto fail;
    }
    for (interface = buffer;
      (uint8_t *)interface < (uint8_t *)buffer + config.ifc_len; ) {
        struct route_entry route = interface->ifr_route;
        int intfSize;
        if (route.flags & (RTF_GATEWAY | RTF_DEFAULT)) {
            *addr = ((struct sockaddr_in *)route.gateway)->sin_addr.s_addr;
            ret = 0;
            break;
        }
        intfSize = sizeof(route) + IF_NAMESIZE;
        if (route.destination != NULL) {
            intfSize += route.destination->sa_len;
        }
        if (route.mask != NULL) {
            intfSize += route.mask->sa_len;
        }
        if (route.gateway != NULL) {
            intfSize += route.gateway->sa_len;
        }
        interface = (struct ifreq *)((uint8_t *)interface + intfSize);
    }
fail:
    free(buffer);
    close(fd);
    return ret;
}
#endif /* #ifdef USE_HAIKU_CODE */

#endif
