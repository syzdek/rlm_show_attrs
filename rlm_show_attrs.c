/*
 *  Attribute Debugging Module for FreeRADIUS  
 *  Copyright (C) 2026 David M. Syzdek <david@syzdek.net>.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *     1. Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 *     3. Neither the name of the copyright holder nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file rlm_show_attrs.c
 * @brief Prints values of attributes when troubleshooting FreeRADIUS.
 *
 * @author David M. Syzdek <david@syzdek.net>
 *
 * @copyright 2026 David M. Syzdek <david@syzdek.net>
 */

///////////////
//           //
//  Headers  //
//           //
///////////////
// MARK: - Headers

#include <freeradius-devel/radiusd.h>
#include <freeradius-devel/modules.h>
#include <freeradius-devel/dlist.h>
#include <freeradius-devel/rad_assert.h>

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <arpa/inet.h>


///////////////////
//               //
//  Definitions  //
//               //
///////////////////
// MARK: - Definitions

#define RLM_SHOW_ATTRS_REQUEST		1
#define RLM_SHOW_ATTRS_CONTROL		2
#define RLM_SHOW_ATTRS_REPLY		3


//////////////////
//              //
//  Data Types  //
//              //
//////////////////
// MARK: - Data Types

typedef struct rlm_show_attrs_t rlm_show_attrs_t;

// modules's structure for the configuration variables
struct rlm_show_attrs_t
{	char const *		name;		//!< name of this instance
	bool			control_attrs;	//!< print control attributes
	bool			reply_attrs;	//!< print reply attributes
	bool			request_attrs;	//!< print request attributes
	bool			use_gmtime;     //!< display date attributes as GM time instead of local time
};


//////////////////
//              //
//  Prototypes  //
//              //
//////////////////
// MARK: - Prototypes

static rlm_rcode_t
mod_accounting(
        void *                          instance,
        REQUEST *                       request );


static rlm_rcode_t
mod_authenticate(
        void *                          instance,
        REQUEST *                       request );


static rlm_rcode_t
mod_authorize(
        void *                          instance,
        REQUEST *                       request );


static int
mod_bootstrap(
	CONF_SECTION *			conf,
	void *				instance );


static int
mod_detach(
	UNUSED void *			instance );


static int
mod_instantiate(
	UNUSED CONF_SECTION *		conf,
	void *				instance );


static rlm_rcode_t
mod_post_auth(
	void *				instance,
	REQUEST *			request );


static rlm_rcode_t
mod_post_proxy(
	void *				instance,
	REQUEST *			request );


static rlm_rcode_t
mod_pre_proxy(
	void *				instance,
	REQUEST *			request );


static rlm_rcode_t
mod_preacct(
	void *				instance,
	REQUEST *			request );


static rlm_rcode_t
mod_recv_coa(
	void *				instance,
	REQUEST *			request );


static rlm_rcode_t
mod_send_coa(
	void *				instance,
	REQUEST *			request );


static rlm_rcode_t
mod_session(
	void *				instance,
	REQUEST *			request );


static rlm_rcode_t
rlm_show_attrs_debug(
	void *				instance,
	REQUEST *			request,
	const char *			prefix );


static rlm_rcode_t
rlm_show_attrs_debug_vps(
	void *				instance,
	REQUEST *			request,
	const char *			prefix,
	int				scope );


/////////////////
//             //
//  Variables  //
//             //
/////////////////
// MARK: - Variables

// Map configuration file names to internal variables
static const CONF_PARSER module_config[] =
{	{ "reply_attributes",	FR_CONF_OFFSET(PW_TYPE_BOOLEAN,	rlm_show_attrs_t, control_attrs),	"yes" },
	{ "reply_attributes",	FR_CONF_OFFSET(PW_TYPE_BOOLEAN,	rlm_show_attrs_t, reply_attrs),		"yes" },
	{ "reply_attributes",	FR_CONF_OFFSET(PW_TYPE_BOOLEAN,	rlm_show_attrs_t, request_attrs),	"yes" },
	{ "use_gmtime",		FR_CONF_OFFSET(PW_TYPE_BOOLEAN,	rlm_show_attrs_t, request_attrs),	"no" },
	CONF_PARSER_TERMINATOR
};


extern module_t rlm_show_attrs;
module_t rlm_show_attrs =
{ 	.magic				= RLM_MODULE_INIT,
	.name				= "totp_code",
	.type				= RLM_TYPE_THREAD_SAFE,
	.inst_size			= sizeof(rlm_show_attrs_t),
	.config				= module_config,
	.instantiate			= mod_instantiate,
	.bootstrap			= mod_bootstrap,
	.detach				= mod_detach,
	.methods =
	{	[MOD_ACCOUNTING]	= mod_accounting,
		[MOD_AUTHENTICATE]	= mod_authenticate,
		[MOD_AUTHORIZE]		= mod_authorize,
		[MOD_POST_AUTH]		= mod_post_auth,
		[MOD_POST_PROXY]	= mod_post_proxy,
		[MOD_PRE_PROXY]		= mod_pre_proxy,
		[MOD_PREACCT]		= mod_preacct,
		[MOD_RECV_COA]		= mod_recv_coa,
		[MOD_SEND_COA]		= mod_send_coa,
		[MOD_SESSION]		= mod_session
	},
};


static const char * base16_map = "0123456789abcdef";


/////////////////
//             //
//  Functions  //
//             //
/////////////////
// MARK: - Functions

rlm_rcode_t
mod_accounting(
        void *                          instance,
        REQUEST *                       request )
{
	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	return(rlm_show_attrs_debug(instance, request, "accounting:"));
}


rlm_rcode_t
mod_authenticate(
        void *                          instance,
        REQUEST *                       request )
{
	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	return(rlm_show_attrs_debug(instance, request, "authenticate:"));
}


rlm_rcode_t
mod_authorize(
        void *                          instance,
        REQUEST *                       request )
{
	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	return(rlm_show_attrs_debug(instance, request, "authorize:"));
}


int
mod_bootstrap(
	CONF_SECTION *			conf,
	void *				instance )
{
	rlm_show_attrs_t *	inst;

	rad_assert(instance != NULL);

	inst = instance;
	if ((inst->name = cf_section_name2(conf)) == NULL)
		inst->name = cf_section_name1(conf);

	return(0);
}


int
mod_detach(
	UNUSED void *			instance )
{
	rad_assert(instance != NULL);
	return(0);
}


int
mod_instantiate(
	UNUSED CONF_SECTION *		conf,
	void *				instance )
{
	rad_assert(conf != NULL);
	rad_assert(instance != NULL);
	return(0);
}


rlm_rcode_t
mod_post_auth(
	void *				instance,
	REQUEST *			request )
{
	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	return(rlm_show_attrs_debug(instance, request, "post-auth:"));
}


rlm_rcode_t
mod_post_proxy(
	void *				instance,
	REQUEST *			request )
{
	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	return(rlm_show_attrs_debug(instance, request, "post-proxy:"));
}


rlm_rcode_t
mod_pre_proxy(
	void *				instance,
	REQUEST *			request )
{
	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	return(rlm_show_attrs_debug(instance, request, "pre-proxy:"));
}


rlm_rcode_t
mod_preacct(
	void *				instance,
	REQUEST *			request )
{
	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	return(rlm_show_attrs_debug(instance, request, "preacct:"));
}


rlm_rcode_t
mod_recv_coa(
	void *				instance,
	REQUEST *			request )
{
	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	return(rlm_show_attrs_debug(instance, request, "recv_coa:"));
}


rlm_rcode_t
mod_send_coa(
	void *				instance,
	REQUEST *			request )
{
	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	return(rlm_show_attrs_debug(instance, request, "send_coa:"));
}


rlm_rcode_t
mod_session(
	void *				instance,
	REQUEST *			request )
{
	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	return(rlm_show_attrs_debug(instance, request, "session:"));
}


rlm_rcode_t
rlm_show_attrs_debug(
	void *				instance,
	REQUEST *			request,
	const char *			prefix )
{
	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	rad_assert(prefix != NULL);

	rlm_show_attrs_debug_vps(instance, request, prefix, RLM_SHOW_ATTRS_REQUEST);
	rlm_show_attrs_debug_vps(instance, request, prefix, RLM_SHOW_ATTRS_CONTROL);
	rlm_show_attrs_debug_vps(instance, request, prefix, RLM_SHOW_ATTRS_REPLY);

	return(RLM_MODULE_NOOP);
}


rlm_rcode_t
rlm_show_attrs_debug_vps(
	void *				instance,
	REQUEST *			request,
	const char *			prefix,
	int				scope )
{
	rlm_show_attrs_t *	inst;
	VALUE_PAIR *		vps;
	const char *		scope_name;
	char 			value[1024];
	size_t			len;
	size_t			pos;
	struct tm		tm;
	time_t			t;

	rad_assert(instance != NULL);
	rad_assert(request != NULL);
	rad_assert(prefix != NULL);

	inst = instance;

	switch(scope)
	{	case RLM_SHOW_ATTRS_REQUEST:
			if (!(inst->request_attrs))
				return(RLM_MODULE_NOOP);
			vps 		= request->packet->vps;
			scope_name	= "request:";
			break;

		case RLM_SHOW_ATTRS_CONTROL:
			if (!(inst->control_attrs))
				return(RLM_MODULE_NOOP);
			vps 		= request->config;
			scope_name	= "control:";
			break;

		case RLM_SHOW_ATTRS_REPLY:
			if (!(inst->reply_attrs))
				return(RLM_MODULE_NOOP);
			vps 		= request->reply->vps;
			scope_name	= "reply:";
			break;

		default:
			return(RLM_MODULE_NOOP);
	};

	while ((vps))
	{	switch(vps->da->type)
		{	// A truth value
			case PW_TYPE_BOOLEAN:
				snprintf(value, sizeof(value), "%s", (((vps->data.byte)) ? "yes" : "no"));
				break;

			// 8 Bit unsigned integer
			case PW_TYPE_BYTE:
				snprintf(value, sizeof(value), "0x%02x", (unsigned)vps->data.byte);
				break;

			// 32 Bit Unix timestamp
			case PW_TYPE_DATE:
				t = (time_t)vps->data.date;
				if ((inst->use_gmtime))
					gmtime_r(&t, &tm);
				else
					localtime_r(&t, &tm);
				strftime(value, (sizeof(value)-1), "%FT%T%z", &tm);
				value[sizeof(value)-1] = '\0';
				break;


			// 48 Bit Mac-Address
			case PW_TYPE_ETHERNET:
				snprintf( value, sizeof(value), "%02x:%02x:%02x:%02x:%02x:%02x",
					  vps->data.ether[0], vps->data.ether[1], vps->data.ether[2],
					  vps->data.ether[3], vps->data.ether[4], vps->data.ether[5] );
				break;

//			// Interface ID
//			case PW_TYPE_IFID:

			// 32 Bit unsigned integer
			case PW_TYPE_INTEGER:
				snprintf(value, sizeof(value), "%"PRIu32, (uint32_t)vps->data.integer);
				break;

			// 64 Bit unsigned integer
			case PW_TYPE_INTEGER64:
				snprintf(value, sizeof(value), "%"PRId64, (int64_t)vps->data.integer64);
				break;

			// 32 Bit IPv4 Address
			case PW_TYPE_IPV4_ADDR:
				if (inet_ntop(AF_INET, &vps->data.ipaddr, value, (sizeof(value)-1)) == NULL)
					value[0] = '\0';
				value[sizeof(value)-1] = '\0';
				break;

//			// IPv4 Prefix
//			case PW_TYPE_IPV4_PREFIX:

			// 128 Bit IPv6 Address
			case PW_TYPE_IPV6_ADDR:
				if (inet_ntop(AF_INET6, &vps->data.ipv6addr, value, (sizeof(value)-1)) == NULL)
					value[0] = '\0';
				value[sizeof(value)-1] = '\0';
				break;

//			// IPv6 Prefix
//			case PW_TYPE_IPV6_PREFIX:

			// Raw octets
			case PW_TYPE_OCTETS:
				len 	= (((sizeof(value)/2)-2) < vps->length)
					? (sizeof(value)/2)-2
					: vps->length;
				value[0] = '0';
				value[1] = 'x';
				for(pos = 0; (pos < len); pos++)
				{	value[(pos/2)+2] = base16_map[(vps->data.octets[pos] >> 4) & 0x0f];
					value[(pos/2)+3] = base16_map[vps->data.octets[pos] & 0x0f];
				};
				value[(pos/2)+2] = '\0';
				break;

			// 16 Bit unsigned integer
			case PW_TYPE_SHORT:
				snprintf(value, sizeof(value), "%"PRIu16, (uint16_t)vps->data.ushort);
				break;

			// 32 Bit signed integer
			case PW_TYPE_SIGNED:
				snprintf(value, sizeof(value), "%"PRId32, (int32_t)vps->data.sinteger);
				break;

			// String of printable characters
			case PW_TYPE_STRING:
				len 	= ((sizeof(value)-3) < vps->length)
					? sizeof(value)-3
					: vps->length;
				memcpy(&value[1], vps->data.strvalue, len);
				value[0]     = '"';
				value[len+1] = '"';
				value[len+2] = '\0';
				break;

//			// Time value (struct timeval), only for config items.
//			case PW_TYPE_TIMEVAL:

			default:
				value[0] = '\0';
				break;
		};
		RDEBUG("%s: %s %-35s = %s", prefix, scope_name, vps->da->name, value);
		vps = vps->next;
	};

	return(RLM_MODULE_NOOP);
}

/* end of source */
