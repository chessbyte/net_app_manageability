#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ruby.h"

#ifdef HAVE_NETAPP_API_H
#include "netapp_api.h"

static void    marshal_args(na_elem_t *elem, VALUE rObj);

static const char *module_name     = "NetAppManageability";
static const char *class_name      = "API";
static const char *exception_name  = "Error";
static const char *hash_class_name = "NAMHash";

static	VALUE	mNetAppManageability;
static	VALUE	cAPI;
static	VALUE	rb_eAPIError;
static	VALUE	cNAMHash;

static	ID		id_to_s;
static	ID		id_logger;
static	ID		id_verbose;
static	ID		id_wire_dump;
static	ID		id_info;
static	ID		id_debug;

/*
 * Create a class constant based on the given object-like macro.
 */
#define INTDEF2CONST(klass, intdef) \
	rb_define_const(klass, #intdef, INT2NUM(intdef))

#define INT2BOOL(v) \
	(v ? Qtrue : Qfalse)

#define LOG_VERBOSE \
	(RTEST(rb_funcall(cAPI, id_verbose, 0)) ? id_info : id_debug)

#define WIRE_DUMP \
	(RTEST(rb_funcall(cAPI, id_wire_dump, 0)))

static void
server_free(void *p)	{
	(void)na_server_close((na_server_t *)p);
}

static VALUE
obj_to_s(VALUE obj) {
	return rb_funcall(obj, id_to_s, 0);
}

static void
rb_log(ID level, const char *fmt, ...)	{
	va_list ap;
	VALUE logger;
	char *p, *np;
	int n, size = 128;

	logger = rb_funcall(cAPI, id_logger, 0);
	if (logger == Qnil) {
		return;
	}

	if ((p = malloc(size)) == NULL)	{
		return;
	}

	va_start(ap, fmt);
	n = vsnprintf(p, size, fmt, ap);
	va_end(ap);

	if (n >= size)	{
		size = n + 1;
		if ((np = realloc(p, size)) == NULL) {
			free(p);
			return;
		}
		p = np;

		va_start(ap, fmt);
		vsnprintf(p, size, fmt, ap);
		va_end(ap);
	}

	rb_funcall(logger, level, 1, rb_str_new2(p));
	free(p);
}

/*
 * The "server_open" class method.
 */
static VALUE
server_open(VALUE rSelf, VALUE rServer, VALUE rMajor, VALUE rMinor) {
	int				cMajor;
	int				cMinor;
	char			*cServer;
	na_server_t		*s;

	cServer	= StringValuePtr(rServer);
	cMajor	= NUM2UINT(rMajor);
	cMinor	= NUM2UINT(rMinor);

	rb_log(LOG_VERBOSE, "%s.server_open [calling]: server = %s, major = %d, minor = %d", class_name, cServer, cMajor, cMinor);
	if ((s = na_server_open(cServer, cMajor, cMinor)) == NULL) {
		rb_raise(rb_eAPIError, "%s.server_open: could not open connection to server %s", class_name, cServer);
	}
	rb_log(LOG_VERBOSE, "%s.server_open [returned]: server = %s, major = %d, minor = %d", class_name, cServer, cMajor, cMinor);
	return Data_Wrap_Struct(rSelf, 0, server_free, s);
}

/*
 * The "server_get_style" class method.
 */
static VALUE
server_get_style(VALUE rSelf, VALUE rServer) {
	na_server_t		*s;
	na_style_t		rv;

	Data_Get_Struct(rServer, na_server_t, s);
	rb_log(LOG_VERBOSE, "%s.server_get_style [calling]", class_name);
	rv = na_server_get_style(s);
	rb_log(LOG_VERBOSE, "%s.server_get_style [returned]: style = %d", class_name, rv);
	return INT2NUM(rv);
}

/*
 * The "server_get_transport_type" class method.
 */
static VALUE
server_get_transport_type(VALUE rSelf, VALUE rServer) {
	na_server_t				*s;
	na_server_transport_t	rv;

	Data_Get_Struct(rServer, na_server_t, s);
	rb_log(LOG_VERBOSE, "%s.na_server_get_transport_type [calling]", class_name);
	rv = na_server_get_transport_type(s);
	rb_log(LOG_VERBOSE, "%s.na_server_get_transport_type [returned]: type = %d", class_name, rv);
	return INT2NUM(rv);
}

/*
 * The "server_get_port" class method.
 */
static VALUE
server_get_port(VALUE rSelf, VALUE rServer) {
	na_server_t		*s;
	int				rv;

	Data_Get_Struct(rServer, na_server_t, s);
	rb_log(LOG_VERBOSE, "%s.server_get_port [calling]", class_name);
	rv = na_server_get_port(s);
	rb_log(LOG_VERBOSE, "%s.server_get_port [returned]: port = %d", class_name, rv);
	return INT2NUM(rv);
}

/*
 * The "server_get_timeout" class method.
 */
static VALUE
server_get_timeout(VALUE rSelf, VALUE rServer) {
	na_server_t		*s;
	int				rv;

	Data_Get_Struct(rServer, na_server_t, s);
	rb_log(LOG_VERBOSE, "%s.server_get_timeout [calling]", class_name);
	rv = na_server_get_timeout(s);
	rb_log(LOG_VERBOSE, "%s.server_get_timeout [returned]: timeout = %d", class_name, rv);
	return INT2NUM(rv);
}

/*
 * The "server_style" class method.
 */
static VALUE
server_style(VALUE rSelf, VALUE rServer, VALUE rStyle) {
	na_server_t		*s;
	na_style_t		cStyle;

	cStyle	= NUM2UINT(rStyle);
	Data_Get_Struct(rServer, na_server_t, s);
	rb_log(LOG_VERBOSE, "%s.server_style [calling]: style = %d", class_name, cStyle);
	na_server_style(s, cStyle);
	rb_log(LOG_VERBOSE, "%s.server_style [returned]", class_name);
	return Qnil;
}

/*
 * The "server_set_debugstyle" class method.
 */
static VALUE
server_set_debugstyle(VALUE rSelf, VALUE rServer, VALUE rStyle) {
	na_server_t		*s;
	na_style_t		cStyle;

	cStyle	= NUM2UINT(rStyle);
	Data_Get_Struct(rServer, na_server_t, s);
	rb_log(LOG_VERBOSE, "%s.na_server_set_debugstyle [calling]: style = %d", class_name, cStyle);
	na_server_set_debugstyle(s, cStyle);
	rb_log(LOG_VERBOSE, "%s.na_server_set_debugstyle [returned]", class_name);
	return Qnil;
}

/*
 * The "server_set_server_type" class method.
 */
static VALUE
server_set_server_type(VALUE rSelf, VALUE rServer, VALUE rType) {
	na_server_t		*s;
	int				cType, rv;

	cType	= NUM2UINT(rType);
	Data_Get_Struct(rServer, na_server_t, s);
	rb_log(LOG_VERBOSE, "%s.server_set_server_type [calling]: type = %d", class_name, cType);
	rv = na_server_set_server_type(s, cType);
	rb_log(LOG_VERBOSE, "%s.server_set_server_type [returned]: rv = %d", class_name, rv);
	return INT2BOOL(rv);
}

/*
 * The "server_set_transport_type" class method.
 */
static VALUE
server_set_transport_type(VALUE rSelf, VALUE rServer, VALUE rType) {
	na_server_t		*s;
	int				cType, rv;

	cType	= NUM2UINT(rType);
	Data_Get_Struct(rServer, na_server_t, s);
	rb_log(LOG_VERBOSE, "%s.server_set_transport_type [calling]: type = %d", class_name, cType);
	rv = na_server_set_transport_type(s, cType, 0);
	rb_log(LOG_VERBOSE, "%s.server_set_transport_type [returned]: rv = %d", class_name, rv);
	return INT2BOOL(rv);
}

/*
 * The "server_set_port" class method.
 */
static VALUE
server_set_port(VALUE rSelf, VALUE rServer, VALUE rPort) {
	na_server_t		*s;
	int				cPort, rv;

	cPort	= NUM2UINT(rPort);
	Data_Get_Struct(rServer, na_server_t, s);
	rb_log(LOG_VERBOSE, "%s.server_set_port [calling]: port = %d", class_name, cPort);
	rv = na_server_set_port(s, cPort);
	rb_log(LOG_VERBOSE, "%s.server_set_port [returned]: rv = %d", class_name, rv);
	return INT2BOOL(rv);
}

/*
 * The "server_set_timeout" class method.
 */
static VALUE
server_set_timeout(VALUE rSelf, VALUE rServer, VALUE rTimeout) {
	na_server_t		*s;
	int				cTimeout, rv;

	cTimeout	= NUM2UINT(rTimeout);
	Data_Get_Struct(rServer, na_server_t, s);
	rb_log(LOG_VERBOSE, "%s.server_set_timeout [calling]: timeout = %d", class_name, cTimeout);
	rv = na_server_set_timeout(s, cTimeout);
	rb_log(LOG_VERBOSE, "%s.server_set_timeout [returned]: rv = %d", class_name, rv);
	return INT2BOOL(rv);
}

/*
 * The "server_adminuser" class method.
 */
static VALUE
server_adminuser(VALUE rSelf, VALUE rServer, VALUE rLogin, VALUE rPwd) {
	na_server_t		*s;
	char			*cLogin;
	char			*cPwd;
	int				rv;

	cLogin	= StringValuePtr(rLogin);
	cPwd	= StringValuePtr(rPwd);
	Data_Get_Struct(rServer, na_server_t, s);
	rb_log(LOG_VERBOSE, "%s.server_adminuser [calling]: login = %s", class_name, cLogin);
	rv = na_server_adminuser(s, cLogin, cPwd);
	rb_log(LOG_VERBOSE, "%s.server_adminuser [returned]: rv = %d", class_name, rv);
	return INT2BOOL(rv);
}

typedef struct  {
	char		*key;
	na_elem_t	*elem;
} array_iter_arg_t;

static VALUE
array_iter_func(VALUE ae, array_iter_arg_t *aia) {
	na_elem_t	*ce;

	ce = na_elem_new(aia->key);
	na_child_add(aia->elem, ce);
	marshal_args(ce, ae);

	return Qnil;
}

static int
hash_iter_func(VALUE rKey, VALUE val, na_elem_t *elem) {
	char		*cKey;
	na_elem_t	*ce;

	if (TYPE(rKey) == T_SYMBOL) {
		VALUE tv = obj_to_s(rKey);
		cKey = StringValuePtr(tv);
	}
	else if (TYPE(rKey) == T_STRING) {
		cKey = StringValuePtr(rKey);
	}
	else {
		rb_raise(rb_eTypeError, "%s.hash_iter_func: hash key must be a string or symbol", class_name);
	}

	if (TYPE(val) == T_ARRAY) {
		array_iter_arg_t aia;
		aia.key		= cKey;
		aia.elem	= elem;
		rb_iterate(rb_each, val, array_iter_func, (VALUE)&aia);
		return 0;
	}

	ce = na_elem_new(cKey);
	na_child_add(elem, ce);
	marshal_args(ce, val);

	return 0;
}

static void
marshal_args(na_elem_t *elem, VALUE rObj) {
	VALUE	rsv;
	char	*csv;

	switch ( TYPE(rObj) ) {
		case T_NIL:
			return;

		case T_HASH:
			rb_hash_foreach(rObj, hash_iter_func, (VALUE)elem);
			break;

		case T_ARRAY:
			break;

		case T_STRING:
			csv = StringValuePtr(rObj);
			na_elem_set_content(elem, csv);
			break;

		case T_FIXNUM:
		case T_BIGNUM:
		case T_TRUE:
		case T_FALSE:
			rsv = obj_to_s(rObj);
			csv = StringValuePtr(rsv);
			na_elem_set_content(elem, csv);
			break;

		default:
			rb_raise(rb_eTypeError, "%s.marshal_args: Type = %d, not valid value", class_name, TYPE(rObj));
			break;
	}
}

static VALUE
nam_hash_new(void) {
	return rb_class_new_instance(0, 0, cNAMHash);
}

static VALUE
unmarshal_elem(na_elem_t *elem) {
	na_elem_t		*ce;
	na_elem_iter_t	iter;
	VALUE			rv, hv, ta, cn;

	if (!na_elem_has_children(elem)) {
		return rb_str_new2(na_elem_get_content(elem));
	}

	rv = nam_hash_new();

	for (iter = na_child_iterator(elem); (ce = na_iterator_next(&iter)) != NULL;) {
		cn = rb_str_new2(na_elem_get_name(ce));

		if ((hv = rb_hash_aref(rv, cn)) == Qnil) {
			rb_hash_aset(rv, cn, unmarshal_elem(ce));
			continue;
		}

		if (TYPE(hv) != T_ARRAY) {
			ta = rb_ary_new();
			rb_ary_push(ta, hv);
			rb_hash_aset(rv, cn, ta);
		}
		else {
			ta = hv;
		}
		rb_ary_push(ta, unmarshal_elem(ce));
	}
	return rv;
}

typedef struct  {
	na_server_t		*s;
	na_elem_t		*in;
	na_elem_t		*out;
	VALUE			rArgs;
	char			*cCmd;
} invoke_protect_arg_t;

static VALUE
invoke_protect(VALUE arg) {
	char *xml;

	invoke_protect_arg_t *ipap = (invoke_protect_arg_t *)arg;

	ipap->in = na_elem_new(ipap->cCmd);
	marshal_args(ipap->in, ipap->rArgs);

	if (WIRE_DUMP && ((xml = na_elem_sprintf(ipap->in)) != NULL)) {
		rb_log(LOG_VERBOSE, "%s.server_invoke: REQUEST START", class_name);
		rb_log(LOG_VERBOSE, "%s", xml);
		rb_log(LOG_VERBOSE, "%s.server_invoke: REQUEST END", class_name);
		na_free(xml);
	}

	ipap->out = na_server_invoke_elem(ipap->s, ipap->in);

	if (WIRE_DUMP && ((xml = na_elem_sprintf(ipap->out)) != NULL)) {
		rb_log(LOG_VERBOSE, "%s.server_invoke: RESPONSE START", class_name);
		rb_log(LOG_VERBOSE, "%s", xml);
		rb_log(LOG_VERBOSE, "%s.server_invoke: RESPONSE END", class_name);
		na_free(xml);
	}

	if (na_results_status(ipap->out) != NA_OK) {
		rb_raise(rb_eAPIError, "%s.server_invoke: Error %d: %s", class_name,
					na_results_errno(ipap->out),
					na_results_reason(ipap->out));
	}
	return unmarshal_elem(ipap->out);
}

/*
 * The "server_invoke" class method.
 */
static VALUE
server_invoke(VALUE rSelf, VALUE rServer, VALUE rCmd, VALUE rArgs) {
	invoke_protect_arg_t	ipa;
	VALUE					rv;
	int						exception;

	ipa.rArgs	= rArgs;
	ipa.in		= NULL;
	ipa.out		= NULL;
	ipa.cCmd	= StringValuePtr(rCmd);
	Data_Get_Struct(rServer, na_server_t, ipa.s);

	rb_log(LOG_VERBOSE, "%s.server_invoke [calling]: command = %s", class_name, ipa.cCmd);
	rv = rb_protect(invoke_protect, (VALUE)&ipa, &exception);
	rb_log(LOG_VERBOSE, "%s.server_invoke [returned]: command = %s", class_name, ipa.cCmd);

	if (ipa.in  != NULL) na_elem_free(ipa.in);
	if (ipa.out != NULL) na_elem_free(ipa.out);

	if (exception) {
		rb_jump_tag(exception);
	}
	return rv;
}
#endif

/*
 * Initialize the class.
 */
void Init_net_app_manageability()	{
#ifdef HAVE_NETAPP_API_H
	char	err[256];

	mNetAppManageability	= rb_const_get(rb_cObject,				rb_intern(module_name));
	cAPI					= rb_const_get(mNetAppManageability,	rb_intern(class_name));

	/*
	 * Define class methods.
	 */
	rb_define_singleton_method(cAPI, "server_open",					server_open,				3);
	rb_define_singleton_method(cAPI, "server_get_style",			server_get_style,			1);
	rb_define_singleton_method(cAPI, "server_get_transport_type",	server_get_transport_type,	1);
	rb_define_singleton_method(cAPI, "server_get_port",				server_get_port,			1);
	rb_define_singleton_method(cAPI, "server_get_timeout",			server_get_timeout,			1);
	rb_define_singleton_method(cAPI, "server_style",				server_style,				2);
	rb_define_singleton_method(cAPI, "server_set_debugstyle",		server_set_debugstyle,		2);
	rb_define_singleton_method(cAPI, "server_set_server_type",		server_set_server_type,		2);
	rb_define_singleton_method(cAPI, "server_set_transport_type",	server_set_transport_type,	2);
	rb_define_singleton_method(cAPI, "server_set_port",				server_set_port,			2);
	rb_define_singleton_method(cAPI, "server_set_timeout",			server_set_timeout,			2);
	rb_define_singleton_method(cAPI, "server_adminuser",			server_adminuser,			3);
	rb_define_singleton_method(cAPI, "server_invoke",				server_invoke,				3);

	/*
	 * Create constants in this class based on values defined in netapp_api.h
	 */
	INTDEF2CONST(cAPI, NA_STYLE_LOGIN_PASSWORD);
	INTDEF2CONST(cAPI, NA_STYLE_RPC);
	INTDEF2CONST(cAPI, NA_STYLE_HOSTSEQUIV);

	INTDEF2CONST(cAPI, NA_SERVER_TRANSPORT_HTTP);
	INTDEF2CONST(cAPI, NA_SERVER_TRANSPORT_HTTPS);

	INTDEF2CONST(cAPI, NA_SERVER_TYPE_FILER);
	INTDEF2CONST(cAPI, NA_SERVER_TYPE_NETCACHE);
	INTDEF2CONST(cAPI, NA_SERVER_TYPE_AGENT);
	INTDEF2CONST(cAPI, NA_SERVER_TYPE_DFM);
	INTDEF2CONST(cAPI, NA_SERVER_TYPE_CLUSTER);

	INTDEF2CONST(cAPI, NA_NO_DEBUG);
	INTDEF2CONST(cAPI, NA_PRINT_DONT_PARSE);
	INTDEF2CONST(cAPI, NA_DONT_PRINT_DONT_PARSE);

	rb_eAPIError	= rb_const_get(cAPI,					rb_intern(exception_name));
	cNAMHash		= rb_const_get(mNetAppManageability,	rb_intern(hash_class_name));

	id_to_s			= rb_intern("to_s");
	id_logger		= rb_intern("logger");
	id_verbose		= rb_intern("verbose");
	id_wire_dump	= rb_intern("wire_dump");
	id_info			= rb_intern("info");
	id_debug		= rb_intern("debug");

	/*
	 * Initialize the library.
	 */
	if (!na_startup(err, sizeof(err))) {
		rb_raise(rb_eAPIError, "Error in na_startup: %s", err);
	}
#endif
}
