#include "okcgi.h"

/**
 * a class for things that will be jailed into OKWS's runtime directory,
 * such as compile scripts, and also Python interpreters. Mainly, it
 * will be involved with fiddling access mode bits, settings users/groups
 * and so on.
 */
class okld_jailed_exec_t {
public:
  okld_jailed_exec_t (const str &e, okld_t *o, const str &l) :
    rexecpath (e), okld (o), cfgfile_loc (l), have_ustat (false),
    exec_uid (-1), exec_gid (-1), mode (-1) {}
  ~okld_jailed_exec_t () {}

  bool get_unix_stat ();
  int get_exec_mode ();
  int get_exec_uid ();
  int get_exec_gid ();
  void assign_exec_ownership (int u, int g);
  bool chown ();
  const str &loc () const { return cfgfile_loc; }
  void assign_mode (int m) { mode = m; }
  bool chmod (int m);

  /**
   * get the execpath relative to current file system root, adjusted
   * to whether or not we are jailed.
   *
   */
  str get_execpath () const ;

  /**
   * Change the executable file to have the given uid, gid, and mode.
   * Set the object to have the appropriate intended values for
   * uid_new, gid_new, and mode_new, and then call fix_exec().
   *
   * @param uid_new new UID to assign
   * @param gid_new new GID to assign
   * @param mode_new new file mode to assign
   */
  bool fixup (int uid_new, int gid_new, int mode_new);

  const str & get_execpath_relative_to_chroot () { return rexecpath; }

protected:

  /**
   * Fix an executable to have the right ownership and permissions;
   * do this before every reboot of a service, in case it was replaced
   * and rebooted. This will actually call chmod()/chown() if needed.
   */
  bool fix_exec ();

  const str rexecpath;      // execpath relative to jaildir (starts with '/')
  okld_t *okld;
  str cfgfile_loc;
  bool have_ustat;
  struct stat ustat;

  int exec_uid, exec_gid;  // UID/GID of the executable!
  int mode;

};

/**
 * wrapper class around an interpreter such as Python. Note that
 * for each interpreter, we should have a new group and user.
 * The last thing we want is for a compiled Web service to launch
 * a Python shell if hacked.
 */
class okld_interpreter_t : public okld_jailed_exec_t {
public:
  okld_interpreter_t (const str &n, const str &u, const str &g, 
		      const vec<str> &e, const str &p,
		      const vec<str> &a, okld_t *ol,
		      const str &cfl)
    : okld_jailed_exec_t (p, ol, cfl),
      _name (n), _user (u), _group (g), _env (e), _args (a) {}

  /**
   * specialize an interpreter for a particular UID/GID pair.
   * Name the interpreter /bin/python-5234, where 5234 is the
   * gid the interpreter will be owned by.
   *
   * @param i the existing interpreter to base the new one on
   * @param uid the uid that the new interpreter will be owend by
   * @param gid the gid that the new interpreter will belong to
   */
  okld_interpreter_t (const okld_interpreter_t &i, int uid, int gid)
    : okld_jailed_exec_t (strbuf ("%s-%d", i.rexecpath.cstr (), gid), 
			  i.okld, i.cfgfile_loc),
      _name (i._name),
      _user (uid), _group (gid), _args (i._args) {}


  /**
   * to be called while parsing the configuration line from within
   * the main config file parse file
   */
  bool check (str *err);

  /**
   * Initialize the base interpreter (not the one copied over per Script)
   */
  bool base_init ();

  /**
   * examples include Python, PERL, PYTHON-2.3, etc...
   */
  const str _name;
  ihash_entry<okld_interpreter_t> _link;
  vec<str> get_env () const { return _env; }
private:
  ok_usr_t _user;
  ok_grp_t _group;
  vec<str> _env;
  vec<str> _args;
};

class okld_t;
class okld_ch_t : public okld_jailed_exec_t { // OK Launch Daemon Child Handle
	     ok_usr_t *u, vec<str> env, okws1_port_t p = 0) ;
  virtual ~okld_ch_t () { if (uid) delete uid ;  }
  // who we will setuid to after the spawn
  ok_usr_t *uid;           // UID of whoever will be running this thing
  int gid;                 // GID of whever will be running this thing

  bool can_exec ();
  void assign_uid (int u);
  void assign_gid (int u) { gid = u; }
  ok_usr_t *usr () { return uid; }
  virtual int get_desired_execfile_mode () const { return ok_svc_mode; }
  virtual str get_interpreter () const { return NULL; }
  virtual bool fixup_doall (int uo, int un, int go, int gn, int mo);

  vec<str> env;

};

class okld_ch_script_t : public okld_ch_t {
public:
  okld_ch_script_t (const str &e, const str &s, okld_t *o, const str &cfl, 
		    okld_interpreter_t *ipret,
		    ok_usr_t *u, vec<str> env,
		    okws1_port_t p = 0)
    : okld_ch_t (e, s, o, cfl, u, env, p), _ipret (ipret), 
      _free_ipret (false) {}
  ~okld_ch_script_t () { if (_free_ipret && _ipret) delete _ipret; }
  int get_desired_execfile_mode () const { return ok_script_mode; }
  str get_interpreter () const { return _ipret->get_execpath (); }
  bool fixup_doall (int uo, int un, int go, int gn, int mo);
private:
  okld_interpreter_t *_ipret;
  bool _free_ipret;
      used_primary_port (false) {}
      
  void got_service (bool script, vec<str> s, str loc, bool *errp);
  void got_interpreter (vec<str> s, str log, bool *errp);
  
  void okld_exit (int rc);
  bool init_interpreters ();
  argv_t okdenv;

  ihash<const str, okld_interpreter_t, 
	&okld_interpreter_t::_name,
	&okld_interpreter_t::_link> interpreters;