#include "wrapper.h"

#include "_cgo_export.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static const struct stat emptyStat;
static const struct fuse_entry_param emptyEntryParam;
static const struct statvfs emptyStatVfs;

void bridge_init(void *userdata, struct fuse_conn_info *conn) {
  int id = *(int *)userdata;
  ll_Init(id, conn);
}

void bridge_destroy(void *userdata) {
  int id = *(int *)userdata;
  ll_Destroy(id);
}

void bridge_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
  int id = *(int *)fuse_req_userdata(req);
  struct fuse_entry_param param = emptyEntryParam;
  int err = ll_Lookup(id, parent, (char *)name, &param);
  if (err != 0) {
    fuse_reply_err(req, err);
  } else if (fuse_reply_entry(req, &param) == -ENOENT) {
    // Request aborted, tell filesystem that reference was dropped.
    ll_Forget(id, param.ino, 1);
  }
}

void bridge_forget(fuse_req_t req, fuse_ino_t ino, unsigned long nlookup) {
  int id = *(int *)fuse_req_userdata(req);
  ll_Forget(id, ino, (int)nlookup);
  fuse_reply_none(req);
}

void bridge_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
  int id = *(int *)fuse_req_userdata(req);
  struct stat attr = emptyStat;
  attr.st_uid = getuid();
  attr.st_gid = getgid();
  double attr_timeout = 1.0;
  int err = ll_GetAttr(id, ino, fi, &attr, &attr_timeout);
  if (err != 0) {
    fuse_reply_err(req, err);
  } else {
    fuse_reply_attr(req, &attr, attr_timeout);
  }
}

void bridge_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
                    int to_set, struct fuse_file_info *fi);
void bridge_readlink(fuse_req_t req, fuse_ino_t ino);
void bridge_mknod(fuse_req_t req, fuse_ino_t parent, const char *name,
                  mode_t mode, dev_t rdev);
void bridge_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
                  mode_t mode);
void bridge_unlink(fuse_req_t req, fuse_ino_t parent, const char *name);
void bridge_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name);
void bridge_symlink(fuse_req_t req, const char *link, fuse_ino_t parent,
                    const char *name);
void bridge_rename(fuse_req_t req, fuse_ino_t parent, const char *name,
                   fuse_ino_t newparent, const char *newname);
void bridge_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent,
                 const char *newname);

void bridge_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
  int id = *(int *)fuse_req_userdata(req);
  int err = ll_Open(id, ino, fi);
  if (err != 0) {
    fuse_reply_err(req, err);
  } else if (fuse_reply_open(req, fi) == -ENOENT) {
    // TODO: Request aborted, let Go wrapper know.
  }
}

void bridge_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                 struct fuse_file_info *fi) {
  int id = *(int *)fuse_req_userdata(req);
  char *buf = malloc(size);
  if (!buf) {
    fuse_reply_err(req, EINTR);
  }
  int n = size;
  int err = ll_Read(id, ino, off, fi, buf, &n);
  if (err != 0) {
    fuse_reply_err(req, err);
  }

  fuse_reply_buf(req, buf, n);
  free(buf);
}

void bridge_write(fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size,
                  off_t off, struct fuse_file_info *fi);
void bridge_flush(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
void bridge_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);
void bridge_fsync(fuse_req_t req, fuse_ino_t ino, int datasync,
                  struct fuse_file_info *fi);
void bridge_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);

void bridge_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                    struct fuse_file_info *fi) {
  int id = *(int *)fuse_req_userdata(req);
  struct DirBuf db;
  db.req = req;
  db.size = size < 4096 ? 4096 : size;
  db.buf = malloc(db.size);
  db.offset = 0;

  int err = ll_ReadDir(id, ino, size, off, fi, &db);
  if (err != 0) {
    fuse_reply_err(req, err);
  } else {
    fuse_reply_buf(req, db.buf, db.offset);
  }

  free(db.buf);
}

void bridge_releasedir(fuse_req_t req, fuse_ino_t ino,
                       struct fuse_file_info *fi);
void bridge_fsyncdir(fuse_req_t req, fuse_ino_t ino, int datasync,
                     struct fuse_file_info *fi);

void bridge_statfs(fuse_req_t req, fuse_ino_t ino) {
  int id = *(int *)fuse_req_userdata(req);
  struct statvfs stat = emptyStatVfs;
  int err = ll_StatFs(id, ino, &stat);
  if (err != 0) {
    fuse_reply_err(req, err);
  } else {
    fuse_reply_statfs(req, &stat);
  }
}

void bridge_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                     const char *value, size_t size, int flags);
void bridge_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                     size_t size);
void bridge_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size);
void bridge_removexattr(fuse_req_t req, fuse_ino_t ino, const char *name);
void bridge_access(fuse_req_t req, fuse_ino_t ino, int mask);
void bridge_create(fuse_req_t req, fuse_ino_t parent, const char *name,
                   mode_t mode, struct fuse_file_info *fi);
void bridge_getlk(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi,
                  struct flock *lock);
void bridge_setlk(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi,
                  struct flock *lock, int sleep);
void bridge_bmap(fuse_req_t req, fuse_ino_t ino, size_t blocksize,
                 uint64_t idx);
void bridge_ioctl(fuse_req_t req, fuse_ino_t ino, int cmd, void *arg,
                  struct fuse_file_info *fi, unsigned flags, const void *in_buf,
                  size_t in_bufsz, size_t out_bufsz);
void bridge_poll(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi,
                 struct fuse_pollhandle *ph);
void bridge_write_buf(fuse_req_t req, fuse_ino_t ino, struct fuse_bufvec *bufv,
                      off_t off, struct fuse_file_info *fi);
void bridge_retrieve_reply(fuse_req_t req, void *cookie, fuse_ino_t ino,
                           off_t offset, struct fuse_bufvec *bufv);
void bridge_forget_multi(fuse_req_t req, size_t count,
                         struct fuse_forget_data *forgets);
void bridge_flock(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi,
                  int op);
void bridge_fallocate(fuse_req_t req, fuse_ino_t ino, int mode, off_t offset,
                      off_t length, struct fuse_file_info *fi);

static struct fuse_lowlevel_ops bridge_ll_ops = {.init = bridge_init,
                                                 .destroy = bridge_destroy,
                                                 .lookup = bridge_lookup,
                                                 .getattr = bridge_getattr,
                                                 .readdir = bridge_readdir,
                                                 .open = bridge_open,
                                                 .read = bridge_read, };

int MountAndRun(int id, int argc, char *argv[]) {
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  struct fuse_chan *ch;
  char *mountpoint;
  int err = -1;

  if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) != -1 &&
      (ch = fuse_mount(mountpoint, &args)) != NULL) {
    struct fuse_session *se;

    se = fuse_lowlevel_new(&args, &bridge_ll_ops, sizeof(bridge_ll_ops), &id);
    if (se != NULL) {
      if (fuse_set_signal_handlers(se) != -1) {
        fuse_session_add_chan(se, ch);
        err = fuse_session_loop(se);
        fuse_remove_signal_handlers(se);
        fuse_session_remove_chan(ch);
      }
      fuse_session_destroy(se);
    }
    fuse_unmount(mountpoint, ch);
  }
  fuse_opt_free_args(&args);

  return err ? 1 : 0;
}

int DirBufAdd(struct DirBuf *db, const char *name, fuse_ino_t ino, int mode,
              off_t next) {
  struct stat stbuf = emptyStat;
  stbuf.st_ino = ino;
  stbuf.st_mode = mode;
  stbuf.st_uid = getuid();
  stbuf.st_gid = getgid();

  char *buf = db->buf + db->offset;
  size_t left = db->size - db->offset;
  size_t size = fuse_add_direntry(db->req, buf, left, name, &stbuf, next);
  if (size < left) {
    db->offset += size;
    return 0;
  }

  return 1;
}
