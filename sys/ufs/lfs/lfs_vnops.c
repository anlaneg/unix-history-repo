/*
 * Copyright (c) 1986, 1989, 1991, 1993, 1995
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 *	@(#)lfs_vnops.c	8.13 (Berkeley) 6/10/95
 * $Id: lfs_vnops.c,v 1.25 1997/10/15 09:21:59 phk Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include <vm/vm.h>
#include <vm/vm_prot.h>
#include <vm/vm_page.h>
#include <vm/vm_extern.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/fifofs/fifo.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

static int	 lfs_close __P((struct vop_close_args *));
static int	 lfs_fsync __P((struct vop_fsync_args *));
static int	 lfs_getattr __P((struct vop_getattr_args *));
static int	 lfs_inactive __P((struct vop_inactive_args *));
static int	 lfs_read __P((struct vop_read_args *));
static int	 lfs_write __P((struct vop_write_args *));


/* Global vfs data structures for lfs. */
vop_t **lfs_vnodeop_p;
static struct vnodeopv_entry_desc lfs_vnodeop_entries[] = {
	{ &vop_default_desc,		(vop_t *) vn_default_error },
	{ &vop_abortop_desc,		(vop_t *) ufs_abortop },
	{ &vop_access_desc,		(vop_t *) ufs_access },
	{ &vop_advlock_desc,		(vop_t *) ufs_advlock },
	{ &vop_blkatoff_desc,		(vop_t *) lfs_blkatoff },
	{ &vop_bmap_desc,		(vop_t *) ufs_bmap },
	{ &vop_bwrite_desc,		(vop_t *) lfs_bwrite },
	{ &vop_close_desc,		(vop_t *) lfs_close },
	{ &vop_create_desc,		(vop_t *) ufs_create },
	{ &vop_fsync_desc,		(vop_t *) lfs_fsync },
	{ &vop_getattr_desc,		(vop_t *) lfs_getattr },
	{ &vop_inactive_desc,		(vop_t *) ufs_inactive },
	{ &vop_ioctl_desc,		(vop_t *) ufs_ioctl },
	{ &vop_islocked_desc,		(vop_t *) ufs_islocked },
	{ &vop_lease_desc,		(vop_t *) ufs_lease_check },
	{ &vop_link_desc,		(vop_t *) ufs_link },
	{ &vop_lock_desc,		(vop_t *) ufs_lock },
	{ &vop_lookup_desc,		(vop_t *) ufs_lookup },
	{ &vop_mkdir_desc,		(vop_t *) ufs_mkdir },
	{ &vop_mknod_desc,		(vop_t *) ufs_mknod },
	{ &vop_mmap_desc,		(vop_t *) ufs_mmap },
	{ &vop_open_desc,		(vop_t *) ufs_open },
	{ &vop_pathconf_desc,		(vop_t *) ufs_pathconf },
	{ &vop_poll_desc,		(vop_t *) ufs_poll },
	{ &vop_print_desc,		(vop_t *) ufs_print },
	{ &vop_read_desc,		(vop_t *) lfs_read },
	{ &vop_readdir_desc,		(vop_t *) ufs_readdir },
	{ &vop_readlink_desc,		(vop_t *) ufs_readlink },
	{ &vop_reclaim_desc,		(vop_t *) ufs_reclaim },
	{ &vop_remove_desc,		(vop_t *) ufs_remove },
	{ &vop_rename_desc,		(vop_t *) ufs_rename },
	{ &vop_revoke_desc,		(vop_t *) ufs_revoke },
	{ &vop_rmdir_desc,		(vop_t *) ufs_rmdir },
	{ &vop_seek_desc,		(vop_t *) ufs_seek },
	{ &vop_setattr_desc,		(vop_t *) ufs_setattr },
	{ &vop_strategy_desc,		(vop_t *) ufs_strategy },
	{ &vop_symlink_desc,		(vop_t *) ufs_symlink },
	{ &vop_truncate_desc,		(vop_t *) lfs_truncate },
	{ &vop_unlock_desc,		(vop_t *) ufs_unlock },
	{ &vop_update_desc,		(vop_t *) lfs_update },
	{ &vop_valloc_desc,		(vop_t *) lfs_valloc },
	{ &vop_vfree_desc,		(vop_t *) lfs_vfree },
	{ &vop_whiteout_desc,		(vop_t *) ufs_whiteout },
	{ &vop_write_desc,		(vop_t *) lfs_write },
	{ NULL, NULL }
};
static struct vnodeopv_desc lfs_vnodeop_opv_desc =
	{ &lfs_vnodeop_p, lfs_vnodeop_entries };

vop_t **lfs_specop_p;
static struct vnodeopv_entry_desc lfs_specop_entries[] = {
	{ &vop_default_desc,		(vop_t *) vn_default_error },
	{ &vop_abortop_desc,		(vop_t *) spec_abortop },
	{ &vop_access_desc,		(vop_t *) ufs_access },
	{ &vop_advlock_desc,		(vop_t *) spec_advlock },
	{ &vop_blkatoff_desc,		(vop_t *) spec_blkatoff },
	{ &vop_bmap_desc,		(vop_t *) spec_bmap },
	{ &vop_bwrite_desc,		(vop_t *) lfs_bwrite },
	{ &vop_close_desc,		(vop_t *) ufsspec_close },
	{ &vop_create_desc,		(vop_t *) spec_create },
	{ &vop_fsync_desc,		(vop_t *) spec_fsync },
	{ &vop_getattr_desc,		(vop_t *) lfs_getattr },
	{ &vop_inactive_desc,		(vop_t *) ufs_inactive },
	{ &vop_ioctl_desc,		(vop_t *) spec_ioctl },
	{ &vop_islocked_desc,		(vop_t *) ufs_islocked },
	{ &vop_lease_desc,		(vop_t *) spec_lease_check },
	{ &vop_link_desc,		(vop_t *) spec_link },
	{ &vop_lock_desc,		(vop_t *) ufs_lock },
	{ &vop_lookup_desc,		(vop_t *) spec_lookup },
	{ &vop_mkdir_desc,		(vop_t *) spec_mkdir },
	{ &vop_mknod_desc,		(vop_t *) spec_mknod },
	{ &vop_mmap_desc,		(vop_t *) spec_mmap },
	{ &vop_open_desc,		(vop_t *) spec_open },
	{ &vop_pathconf_desc,		(vop_t *) spec_pathconf },
	{ &vop_poll_desc,		(vop_t *) spec_poll },
	{ &vop_print_desc,		(vop_t *) ufs_print },
	{ &vop_read_desc,		(vop_t *) ufsspec_read },
	{ &vop_readdir_desc,		(vop_t *) spec_readdir },
	{ &vop_readlink_desc,		(vop_t *) spec_readlink },
	{ &vop_reclaim_desc,		(vop_t *) ufs_reclaim },
	{ &vop_remove_desc,		(vop_t *) spec_remove },
	{ &vop_rename_desc,		(vop_t *) spec_rename },
	{ &vop_revoke_desc,		(vop_t *) spec_revoke },
	{ &vop_rmdir_desc,		(vop_t *) spec_rmdir },
	{ &vop_seek_desc,		(vop_t *) spec_seek },
	{ &vop_setattr_desc,		(vop_t *) ufs_setattr },
	{ &vop_strategy_desc,		(vop_t *) spec_strategy },
	{ &vop_symlink_desc,		(vop_t *) spec_symlink },
	{ &vop_truncate_desc,		(vop_t *) spec_truncate },
	{ &vop_unlock_desc,		(vop_t *) ufs_unlock },
	{ &vop_update_desc,		(vop_t *) lfs_update },
	{ &vop_valloc_desc,		(vop_t *) spec_valloc },
	{ &vop_vfree_desc,		(vop_t *) lfs_vfree },
	{ &vop_write_desc,		(vop_t *) ufsspec_write },
	{ NULL, NULL }
};
static struct vnodeopv_desc lfs_specop_opv_desc =
	{ &lfs_specop_p, lfs_specop_entries };

vop_t **lfs_fifoop_p;
static struct vnodeopv_entry_desc lfs_fifoop_entries[] = {
	{ &vop_default_desc,		(vop_t *) vn_default_error },
	{ &vop_abortop_desc,		(vop_t *) fifo_abortop },
	{ &vop_access_desc,		(vop_t *) ufs_access },
	{ &vop_advlock_desc,		(vop_t *) fifo_advlock },
	{ &vop_blkatoff_desc,		(vop_t *) fifo_blkatoff },
	{ &vop_bmap_desc,		(vop_t *) fifo_bmap },
	{ &vop_bwrite_desc,		(vop_t *) lfs_bwrite },
	{ &vop_close_desc,		(vop_t *) ufsfifo_close },
	{ &vop_create_desc,		(vop_t *) fifo_create },
	{ &vop_fsync_desc,		(vop_t *) fifo_fsync },
	{ &vop_getattr_desc,		(vop_t *) lfs_getattr },
	{ &vop_inactive_desc,		(vop_t *) ufs_inactive },
	{ &vop_ioctl_desc,		(vop_t *) fifo_ioctl },
	{ &vop_islocked_desc,		(vop_t *) ufs_islocked },
	{ &vop_lease_desc,		(vop_t *) fifo_lease_check },
	{ &vop_link_desc,		(vop_t *) fifo_link },
	{ &vop_lock_desc,		(vop_t *) ufs_lock },
	{ &vop_lookup_desc,		(vop_t *) fifo_lookup },
	{ &vop_mkdir_desc,		(vop_t *) fifo_mkdir },
	{ &vop_mknod_desc,		(vop_t *) fifo_mknod },
	{ &vop_mmap_desc,		(vop_t *) fifo_mmap },
	{ &vop_open_desc,		(vop_t *) fifo_open },
	{ &vop_pathconf_desc,		(vop_t *) fifo_pathconf },
	{ &vop_poll_desc,		(vop_t *) fifo_poll },
	{ &vop_print_desc,		(vop_t *) ufs_print },
	{ &vop_read_desc,		(vop_t *) ufsfifo_read },
	{ &vop_readdir_desc,		(vop_t *) fifo_readdir },
	{ &vop_readlink_desc,		(vop_t *) fifo_readlink },
	{ &vop_reclaim_desc,		(vop_t *) ufs_reclaim },
	{ &vop_remove_desc,		(vop_t *) fifo_remove },
	{ &vop_rename_desc,		(vop_t *) fifo_rename },
	{ &vop_revoke_desc,		(vop_t *) fifo_revoke },
	{ &vop_rmdir_desc,		(vop_t *) fifo_rmdir },
	{ &vop_seek_desc,		(vop_t *) fifo_seek },
	{ &vop_setattr_desc,		(vop_t *) ufs_setattr },
	{ &vop_strategy_desc,		(vop_t *) fifo_strategy },
	{ &vop_symlink_desc,		(vop_t *) fifo_symlink },
	{ &vop_truncate_desc,		(vop_t *) fifo_truncate },
	{ &vop_unlock_desc,		(vop_t *) ufs_unlock },
	{ &vop_update_desc,		(vop_t *) lfs_update },
	{ &vop_valloc_desc,		(vop_t *) fifo_valloc },
	{ &vop_vfree_desc,		(vop_t *) lfs_vfree },
	{ &vop_write_desc,		(vop_t *) ufsfifo_write },
	{ NULL, NULL }
};
static struct vnodeopv_desc lfs_fifoop_opv_desc =
	{ &lfs_fifoop_p, lfs_fifoop_entries };

VNODEOP_SET(lfs_vnodeop_opv_desc);
VNODEOP_SET(lfs_specop_opv_desc);
VNODEOP_SET(lfs_fifoop_opv_desc);

#define	LFS_READWRITE
#include <ufs/ufs/ufs_readwrite.c>
#undef	LFS_READWRITE

/*
 * Synch an open file.
 */
/* ARGSUSED */
static int
lfs_fsync(ap)
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int a_waitfor;
		struct proc *a_p;
	} */ *ap;
{
	struct timeval tv;
	int error;

	gettime(&tv);
	error = (VOP_UPDATE(ap->a_vp, &tv, &tv,
	    ap->a_waitfor == MNT_WAIT ? LFS_SYNC : 0));
	if(ap->a_waitfor == MNT_WAIT && ap->a_vp->v_dirtyblkhd.lh_first != NULL)
	       panic("lfs_fsync: dirty bufs");
	return( error );
}

/*
 * These macros are used to bracket UFS directory ops, so that we can
 * identify all the pages touched during directory ops which need to
 * be ordered and flushed atomically, so that they may be recovered.
 */
#define	SET_DIROP(fs) {							\
	if ((fs)->lfs_writer)						\
		tsleep(&(fs)->lfs_dirops, PRIBIO + 1, "lfs_dirop", 0);	\
	++(fs)->lfs_dirops;						\
	(fs)->lfs_doifile = 1;						\
}

#define	SET_ENDOP(fs) {							\
	--(fs)->lfs_dirops;						\
	if (!(fs)->lfs_dirops)						\
		wakeup(&(fs)->lfs_writer);				\
}

#define	MARK_VNODE(dvp)	(dvp)->v_flag |= VDIROP


/* XXX hack to avoid calling ITIMES in getattr */
static int
lfs_getattr(ap)
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct inode *ip = VTOI(vp);
	register struct vattr *vap = ap->a_vap;
	/*
	 * Copy from inode table
	 */
	vap->va_fsid = ip->i_dev;
	vap->va_fileid = ip->i_number;
	vap->va_mode = ip->i_mode & ~IFMT;
	vap->va_nlink = ip->i_nlink;
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	vap->va_rdev = (dev_t)ip->i_rdev;
	vap->va_size = ip->i_din.di_size;
	vap->va_atime.tv_sec = ip->i_atime;
	vap->va_atime.tv_nsec = ip->i_atimensec;
	vap->va_mtime.tv_sec = ip->i_mtime;
	vap->va_mtime.tv_nsec = ip->i_mtimensec;
	vap->va_ctime.tv_sec = ip->i_ctime;
	vap->va_ctime.tv_nsec = ip->i_ctimensec;
	vap->va_flags = ip->i_flags;
	vap->va_gen = ip->i_gen;
	/* this doesn't belong here */
	if (vp->v_type == VBLK)
		vap->va_blocksize = BLKDEV_IOSIZE;
	else if (vp->v_type == VCHR)
		vap->va_blocksize = MAXBSIZE;
	else
		vap->va_blocksize = vp->v_mount->mnt_stat.f_iosize;
	vap->va_bytes = dbtob(ip->i_blocks);
	vap->va_type = vp->v_type;
	vap->va_filerev = ip->i_modrev;
	return (0);
}
/*
 * Close called
 *
 * XXX -- we were using ufs_close, but since it updates the
 * times on the inode, we might need to bump the uinodes
 * count.
 */
/* ARGSUSED */
static int
lfs_close(ap)
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct inode *ip = VTOI(vp);
	int mod;

	simple_lock(&vp->v_interlock);
	if (vp->v_usecount > 1) {
		mod = ip->i_flag & IN_MODIFIED;
		ITIMES(ip, &time, &time);
		if (!mod && ip->i_flag & IN_MODIFIED)
			ip->i_lfs->lfs_uinodes++;
	}
	simple_unlock(&vp->v_interlock);
	return (0);
}
