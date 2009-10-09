// -*-c++-*-
/* $Id: parr.h 2784 2007-04-20 16:32:00Z max $ */


#ifndef _LIBPUB_PUB3FUNC_H_
#define _LIBPUB_PUB3FUNC_H_

#include "pub.h"
#include "pub3expr.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  class expr_statement_t {
  public:
    expr_statement_t (ptr<pub3::expr_t> e, int l) 
      : _expr (e), _lineno (l) {}
    expr_statement_t (const xpub3_expr_statement_t &x);
    void output (output_t *o, penv_t *e) const;
    bool publish_nonblock (pub2_iface_t *, output_t *o, penv_t *e) const;
    pfile_el_type_t get_type () const { return PFILE_PUB3_EXPR_STATEMENT; }
    const char *get_obj_name () const { return "pub3::expr_statement_t"; }
    void dump2 (dumper_t *d) const { /* XXX - implement me */ }
    bool to_xdr (xpub_obj_t *x) const;
  private:
    ptr<expr_t> _expr;
    const int _lineno;
  };

  //-----------------------------------------------------------------------

  class runtime_fn_t : public expr_t {
  public:
    runtime_fn_t (const str &n, ptr<expr_list_t> a, int l) 
      : expr_t (l), _name (n), _arglist (a) {}

    ptr<expr_list_t> args () const { return _arglist; }
    str name () const { return _name; }

    bool to_xdr (xpub3_expr_t *x) const;
    const char *get_obj_name () const { return "pub3::runtime_fn_t"; }

    ptr<const pval_t> eval (eval_t e) const { return NULL; }
    ptr<pval_t> eval_freeze (eval_t e) const { return NULL; }

  protected:
    str _name;
    ptr<expr_list_t> _arglist;
  };

  //-----------------------------------------------------------------------

  class runtime_fn_stub_t : public runtime_fn_t {
  public:
    runtime_fn_stub_t (const str &n, ptr<expr_list_t> a, int l) 
      : runtime_fn_t (n, a, l) {}

    static ptr<runtime_fn_stub_t> alloc (const str &n, ptr<expr_list_t> l);

    const char *get_obj_name () const { return "pub3::runtime_fn_stub_t"; }

    ptr<const pval_t> eval (eval_t e) const;
    ptr<pval_t> eval_freeze (eval_t e) const;
    bool unshift_argument (ptr<expr_t> e);

  protected:
    mutable ptr<const expr_t> _rfn;
    ptr<const expr_t> get_rfn () const;
  };

  //-----------------------------------------------------------------------

  class error_fn_t : public runtime_fn_t {
  public:
    error_fn_t (const str &n, ptr<expr_list_t> a, int l, const str &err)
      : runtime_fn_t (n, a, l), _err (err) {}

    ptr<const pval_t> eval (eval_t e) const;
    ptr<pval_t> eval_freeze (eval_t e) const;
    const char *get_obj_name () const { return "pub3::error_fn_t"; }
  protected:
    str _err;
  };

  //-----------------------------------------------------------------------

  //
  // rfn_factory: runtime function factory
  //
  //  allocates runtime functions.  OKWS core supports some of them
  //  but specific okws instances are welcome to support more.  
  //  details still uncertain, but it will most likely involve
  //  local changes to pubd (via dynamic loading?) and also changes
  //  to specific services -- similar to the Apache module system...
  //
  class rfn_factory_t {
  public:
    rfn_factory_t () {}
    virtual ~rfn_factory_t () {}

    virtual ptr<runtime_fn_t>
    alloc (const str &s, ptr<expr_list_t> l, int lineno) = 0;

    ptr<runtime_fn_t> alloc (const xpub3_fn_t &x);

    // Access the singleton runtime function factory; by default
    // it's set to a null factory, but can be explanded any which
    // way.
    static ptr<rfn_factory_t> _curr;
    static void set (ptr<rfn_factory_t> f);
    static ptr<rfn_factory_t> get ();
  };

  //-----------------------------------------------------------------------

  class null_rfn_factory_t : public rfn_factory_t {
  public:
    null_rfn_factory_t  () : rfn_factory_t () {}
    ptr<runtime_fn_t> alloc (const str &s, ptr<expr_list_t> l, int lineno);

  };

  //-----------------------------------------------------------------------
};

#endif /* _LIBPUB_PUB3FUNC_H_ */
