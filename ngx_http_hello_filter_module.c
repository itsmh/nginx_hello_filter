#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#define HELLO_WORLD "hello world"

static ngx_int_t ngx_http_hello_filter_init(ngx_conf_t *cf);
static void * ngx_http_hello_create_loc_conf(ngx_conf_t *cf);
static char * ngx_http_hello_merge_loc_conf(ngx_conf_t *cf,void *parent, void *child);


typedef struct {
//    ngx_uint_t  methods;
//    ngx_flag_t  create_full_put_path;
    ngx_str_t  hello_world;
} ngx_http_hello_conf_t;


static ngx_command_t ngx_http_hello_world_commands[] = {

    { ngx_string("hello_world"), /* directive */
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1, /* location context and takes no arguments*/
      ngx_conf_set_str_slot, /* configuration setup function */
      NGX_HTTP_LOC_CONF_OFFSET, /* No offset. Only one context is supported. */
      offsetof(ngx_http_hello_conf_t, hello_world), /* No offset when storing the module configuration on struct. */
      NULL},

    ngx_null_command /* command termination */
};

/* The module context. */
static ngx_http_module_t ngx_http_hello_filter_module_ctx = {
    NULL, /* preconfiguration */
    ngx_http_hello_filter_init, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    ngx_http_hello_create_loc_conf, /* create location configuration */
    ngx_http_hello_merge_loc_conf /* merge location configuration */
};


/* Module definition. */
ngx_module_t ngx_http_hello_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_hello_filter_module_ctx, /* module context */
    ngx_http_hello_world_commands, /* module directives */
    NGX_HTTP_MODULE, /* module type */
    NULL, /* init master */
    NULL, /* init module */
    NULL, /* init process */
    NULL, /* init thread */
    NULL, /* exit thread */
    NULL, /* exit process */
    NULL, /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;


static ngx_int_t
ngx_http_hello_header_filter(ngx_http_request_t *r)
{
    ngx_http_hello_conf_t   *ctx;
    
    ctx = ngx_http_get_module_loc_conf(r, ngx_http_hello_filter_module);
    if  (   ctx == NULL ||
            ctx->hello_world.len < 1
        ) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0 , "Context is null, skipping" );
        return ngx_http_next_header_filter(r);
    }
    
    
    ngx_http_set_ctx(r, ctx, ngx_http_hello_filter_module);

    if (r->headers_out.content_length_n != -1) {
        r->headers_out.content_length_n += ctx->hello_world.len;
    }

    if (r->headers_out.content_length) {
        r->headers_out.content_length->hash = 0;
        r->headers_out.content_length = NULL;
    }
    
    ngx_http_clear_accept_ranges(r);
    
    return ngx_http_next_header_filter(r);
}


static ngx_int_t
ngx_http_hello_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_buf_t             *buf;
    ngx_uint_t             last;
    ngx_chain_t           *cl, *nl;
    ngx_http_hello_conf_t   *ctx;
    
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0 , "hello: inside body filter module" );
    
    ctx = ngx_http_get_module_ctx(r, ngx_http_hello_filter_module);
    if (ctx == NULL) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0 , "hello : context is null , skipping" );
        return ngx_http_next_body_filter(r, in);
    } else {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0 , "hello: data found" );
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0 , (const char*)ctx->hello_world.data );
    }
    
    last = 0;

    for (cl = in; cl; cl = cl->next) {
         if (cl->buf->last_buf) {
             last = 1;
             break;
         }
    }
    
    if (!last) {
        return ngx_http_next_body_filter(r, in);
    }

    buf = ngx_calloc_buf(r->pool);
    if (buf == NULL) {
        return NGX_ERROR;
    }
    
    buf->pos = (u_char *) ctx->hello_world.data;
    buf->last = buf->pos + ctx->hello_world.len;
    buf->start = buf->pos;
    buf->end = buf->last;
    buf->last_buf = 1;
    buf->memory = 1;
    
   
    
    if (ngx_buf_size(cl->buf) == 0) {
        cl->buf = buf;
    } else {
        nl = ngx_alloc_chain_link(r->pool);
        if (nl == NULL) {
            return NGX_ERROR;
        }

        nl->buf = buf;
        nl->next = NULL;
        cl->next = nl;
        cl->buf->last_buf = 0;
    }
    
    
    return ngx_http_next_body_filter(r, in);
}


static ngx_int_t
ngx_http_hello_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_hello_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_hello_body_filter;

    return NGX_OK;
}

static char * ngx_http_hello_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_hello_conf_t *prev = parent;
    ngx_http_hello_conf_t *conf = child;
    
    ngx_conf_merge_str_value(conf->hello_world, prev->hello_world,"");

    return NGX_CONF_OK;
}

static void *
ngx_http_hello_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_hello_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_hello_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    return conf;
}
