// -*-c++-*-

#pragma once

#include "pub3pub.h"
#include "pub3file.h"
#include "pub3ast.h"
#include "pub3expr.h"
#include "pub3eval.h"
#include "pub3cache.h"

namespace pub3 {

  typedef xpub_status_t status_t;
  typedef event<status_t, ptr<file_t> >::ref getfile_ev_t;

  //-----------------------------------------------------------------------
  //
  // Global data shared across all pub objects -- such as global bindings
  // published from config files are startup.
  //
  class global_t {
  public:
    global_t ();
    static ptr<global_t> get ();
    ptr<bindtab_t> universals () { return _universals; }
  private:
    ptr<bindtab_t> _universals;
  };

  //
  //-----------------------------------------------------------------------

  //-----------------------------------------------------------------------
  //
  // Hi-level interface for all okpublishers, copied for the most part
  // from pub v2.  There are a few changes here --- first, we're passing
  // in expr_dict_t's rather than 
  //

  class ok_iface_t : public virtual refcount {
  public:
    virtual ~ok_iface_t () {}

    /**
     * Run the publisher on file fn, writing the output to buffer b.
     *
     * @param b the buffer to output to
     * @param fn the filename to publish
     * @param ev the event to trigger after complete; true for success.
     * @param opts flags to affect run behavior
     * @param sp Status pointer -- return the full pub status if the 
     *      caller specifies a non-null slot.
     * @param fp File pointer --- return the pubbed file pointer if the
     *      caller specified a non-null slot.
     */
    virtual void run (zbuf *b, str fn, evb_t ev, 
		      ptr<expr_dict_t> d = NULL, 
		      opts_t opts = -1, 
		      status_t *sp = NULL, 
		      ptr<file_t> *fp = NULL, CLOSURE) = 0;

    /**
     * Run the publisher, ignoring output, just amassing all universals
     * and globals into the given dictionary.]
     *
     * @param fn the file to publisher
     * @param ev the event to trigger on completion; true for success
     * @param d the dinctionary to publish to. if none given, assume universals.
     * @param sp status pointer -- return the full pub status if the
     *      caller specified a non-null slot.
     *
     */  
    virtual void run_cfg (str fn, evb_t ev, ptr<expr_dict_t> d = NULL,
			  status_t *sp = NULL, CLOSURE) = 0;

    /**
     * syntax check the file
     *
     * @param f the file to syntax check
     * @param err report the error here (if the caller specified non-null)
     * @param ev trigger this event with 0 for success and -1 for error
     */
    virtual void syntax_check (str f, str *err, evi_t ev, CLOSURE) = 0;

    /**
     * Publish the given file
     * @param p the publishing state
     * @param fn the file to publish
     * @param ev the event to trigger with the status
     */
    virtual void publish (publish_t p, str fn, getfile_ev_t ev, CLOSURE) = 0;

    // set/get global ops for this publishing interface.
    virtual opts_t opts () const = 0;
    virtual void set_opts (opts_t i) = 0;

  };

  //--------------------------------------------------------------------
  //
  // The abstract publisher implements most of the above functions,
  // but depends on a subclass to implement getfile().
  //
  class abstract_publisher_t : public ok_iface_t {
  public:
    abstract_publisher_t (opts_t o = 0);

    void run (zbuf *b, str fn, evb_t ev, ptr<expr_dict_t> d = NULL, 
	      opts_t opts = 0, status_t *sp = NULL, ptr<file_t> *fp = NULL,
	      CLOSURE);

    void run_cfg (str fn, evb_t ev, ptr<expr_dict_t> d = NULL,
		  status_t *sp = NULL, CLOSURE);

    void syntax_check (str f, str *err, evi_t ev, CLOSURE);
    opts_t opts () const { return _opts; }
    void set_opts (opts_t o) { _opts = o; }

  protected:
    // to be filled in by the sub classes
    virtual void getfile (pfnm_t fn, getfile_ev_t cb, u_int o = 0) = 0;
    virtual bool is_remote () const = 0;

    void publish (publish_t p, str fn, getfile_ev_t ev, CLOSURE);
    void list_files_to_check (str cwd, str n, vec<str> *out, 
			      ptr<const localizer_t> l);
  private:
    opts_t _opts;
  };

  //=======================================================================

  /**
   * The most basic remote publisher.  Never caches anything.
   */
  class remote_publisher_t : public abstract_publisher_t {
  public:
    remote_publisher_t (ptr<axprt_stream> x, u_int o = 0);
    virtual ~remote_publisher_t () {}

    virtual void connect (evb_t cb, CLOSURE);

    void getfile (pfnm_t fn, getfile_ev_t cb, u_int o = 0) = 0;

    void dispatch (svccb *sbp);
    virtual void lost_connection () {}
    virtual void handle_new_deltas (svccb *sbp);
    

  protected:

    //------------------------------------------------------------
    // All involved with getting a file, and dealing with its
    // chunks, if necessary.
    //
    void getfile_body (str nm, const xpub3_getfile_res_t *res, 
		       getfile_ev_t cb, u_int opt, CLOSURE);

    void getfile_chunked (const xpub3_chunkshdr_t &hdr, u_int opts,
			  xpub3_file_t *file, status_ev_t cb, CLOSURE);

    void getchunk (const xpub3_hash_t &key, u_int opts,
		   size_t offset, size_t sz, char *buf, evb_t ok, CLOSURE);
    //
    //-----------------------------------------------------------------------


    bool is_remote () const { return true; }

    virtual bool prepare_getfile (const cache_key_t &k, 
				  xpub3_getfile_arg_t *arg, 
				  ptr<file_t> *f,
				  status_t *status);

    virtual void cache_getfile (const cache_key_t &k, ptr<file_t> file) {}
    virtual ptr<file_t> file_nochange (const cache_key_t &k) { return NULL; }
    virtual void cache_noent (str nm) {}

  protected:
    ptr<axprt_stream> _x;
    ptr<aclnt> _cli;
    ptr<asrv> _srv;
    size_t _maxsz;
  };

  //=======================================================================

};