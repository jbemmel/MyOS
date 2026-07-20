#ifndef MYOSRESULT_H
#define MYOSRESULT_H

namespace MyOS {

/// Result codes of many interface calls, negative value indicates error
enum myos_result_t {
   E_MYOS_SUCCESS    = 0,   //< no errors
   E_MYOS_MORE       = 0,   //< Success, and can process more
   E_MYOS_PROCESSING = 0,   //< Processing asynchronously

   /**
    * Sign bit indicates error, use "<code> & E_MYOS_ERROR" to test
    * @note '- <error>' causes GCC to allocate 64 bit !
    */
   E_MYOS_ERROR   = 0x80000000,

   E_MYOS_E_UNEXPECTED   = E_MYOS_ERROR | 0x4000, /**< Unexpected error */
   E_MYOS_E_NOTIMPL      = E_MYOS_ERROR | 0x4001, /**< Not implemented */
   E_MYOS_E_NOINTERFACE	 = E_MYOS_ERROR | 0x4002, /**< Interface not supported */
   E_MYOS_E_POINTER      = E_MYOS_ERROR | 0x4003, /**< Bad pointer */
   E_MYOS_E_ABORT        = E_MYOS_ERROR | 0x4004, /**< Operation aborted */
   E_MYOS_E_FAIL         = E_MYOS_ERROR | 0x4005, /**< Operation failed */
   E_MYOS_E_ACCESSDENIED = E_MYOS_ERROR | 0x4005, /**< Access denied */
   E_MYOS_E_OUTOFMEMORY  = E_MYOS_ERROR | 0x4006, /**< Out of memory */
   E_MYOS_E_INVALIDARG   = E_MYOS_ERROR | 0x4007, /**< Invalid argument */

   /*  ISO/ANSI CE_1990 errors */
   E_MYOS_EDOM      = E_MYOS_ERROR | 0x001, /**< Argument out of domain */
   E_MYOS_ERANGE    = E_MYOS_ERROR | 0x002, /**< Result too large */

   /*  POSIXE_MYOS_ERROR | 0x1990 errors */
   E_MYOS_E2BIG      = E_MYOS_ERROR | 0x101, /**< Argument list too long */
   E_MYOS_EACCES	   = E_MYOS_ERROR | 0x102, /**< Permission denied */
   E_MYOS_EAGAIN     = E_MYOS_ERROR | 0x103, /**< Rsrc temporarily unavail */
   E_MYOS_EBADF      = E_MYOS_ERROR | 0x104, /**< Bad file descriptor */
   E_MYOS_EBUSY      = E_MYOS_ERROR | 0x105, /**< Device busy */
   E_MYOS_ECHILD     = E_MYOS_ERROR | 0x106, /**< No child processes */
   E_MYOS_EDEADLK    = E_MYOS_ERROR | 0x107, /**< Resource deadlock avoided */
   E_MYOS_EEXIST     = E_MYOS_ERROR | 0x108, /**< File exists */
   E_MYOS_EFAULT     = E_MYOS_E_POINTER,     /**< Bad address */
   E_MYOS_EFBIG      = E_MYOS_ERROR | 0x109, /**< File too large */
   E_MYOS_EINTR      = E_MYOS_ERROR | 0x10a, /**< Interrupted system call */
   E_MYOS_EINVAL     = E_MYOS_E_INVALIDARG, /**<  Invalid argument */
   E_MYOS_EIO        = E_MYOS_ERROR | 0x10b, /**< Input/output error */
   E_MYOS_EISDIR	   = E_MYOS_ERROR | 0x10c, /**< Is a directory */
   E_MYOS_EMFILE	   = E_MYOS_ERROR | 0x10d, /**< Too many open files */
   E_MYOS_EMLINK	   = E_MYOS_ERROR | 0x10e, /**< Too many links */
   E_MYOS_ENAMETOOLONG	= E_MYOS_ERROR | 0x10f, /**< File name too long */
   E_MYOS_ENFILE		= E_MYOS_ERROR | 0x110, /**< Max files open in system */
   E_MYOS_ENODEV		= E_MYOS_ERROR | 0x111, /**< Op not supported by device */
   E_MYOS_ENOENT		= E_MYOS_ERROR | 0x112, /**< No such file or directory */
   E_MYOS_ENOEXEC		= E_MYOS_ERROR | 0x113, /**< Exec format error */
   E_MYOS_ENOLCK		= E_MYOS_ERROR | 0x114, /**< No locks available */
   E_MYOS_ENOMEM		= E_MYOS_E_OUTOFMEMORY, /**<  Cannot allocate memory */
   E_MYOS_ENOSPC		= E_MYOS_ERROR | 0x115, /**< No space left on device */
   E_MYOS_ENOSYS		= E_MYOS_E_NOTIMPL,     /**< Function not implemented */
   E_MYOS_ENOTDIR		= E_MYOS_ERROR | 0x116, /**< Not a directory */
   E_MYOS_ENOTEMPTY	= E_MYOS_ERROR | 0x117, /**< Directory not empty */
   E_MYOS_ENOTTY		= E_MYOS_ERROR | 0x118, /**< Inappropriate ioctl */
   E_MYOS_ENXIO		= E_MYOS_ERROR | 0x119, /**< Device not configured */
   E_MYOS_EPERM		= E_MYOS_E_ACCESSDENIED, /**<  Op not permitted */
   E_MYOS_EPIPE		= E_MYOS_ERROR | 0x11a, /**< Broken pipe */
   E_MYOS_EROFS		= E_MYOS_ERROR | 0x11b, /**< Readonly file system */
   E_MYOS_ESPIPE		= E_MYOS_ERROR | 0x11c, /**< Illegal seek */
   E_MYOS_ESRCH		= E_MYOS_ERROR | 0x11d, /**< No such process */
   E_MYOS_EXDEV		= E_MYOS_ERROR | 0x11e, /**< Cross-device link */

   /*  POSIX_1993 errors */
   E_MYOS_EBADMSG	     = E_MYOS_ERROR | 0x120, /**< Bad message */
   E_MYOS_ECANCELED    = E_MYOS_ERROR | 0x121, /**< Operation canceled */
   E_MYOS_EINPROGRESS  = E_MYOS_ERROR | 0x122, /**< Operation in progress */
   E_MYOS_EMSGSIZE     = E_MYOS_ERROR | 0x123, /**< Bad message buffer length */
   E_MYOS_ENOTSUP	     = E_MYOS_ERROR | 0x124, /**< Not supported */

   /*  POSIX_1996 errors */
   E_MYOS_ETIMEDOUT    = E_MYOS_ERROR | 0x130, /**< Operation timed out */

   /*  X/Open UNIX 1994 errors */
   E_MYOS_EADDRINUSE       = E_MYOS_ERROR | 0x200, /**< Address in use */
   E_MYOS_EADDRNOTAVAIL	   = E_MYOS_ERROR | 0x201, /**< Address not available */
   E_MYOS_EAFNOSUPPORT	   = E_MYOS_ERROR | 0x202, /**< Address family unsupported */
   E_MYOS_EALREADY         = E_MYOS_ERROR | 0x203, /**< Already connected */
   E_MYOS_ECONNABORTED	   = E_MYOS_ERROR | 0x204, /**< Connection aborted */
   E_MYOS_ECONNREFUSED	   = E_MYOS_ERROR | 0x205, /**< Connection refused */
   E_MYOS_ECONNRESET       = E_MYOS_ERROR | 0x206, /**< Connection reset */
   E_MYOS_EDESTADDRREQ	   = E_MYOS_ERROR | 0x207, /**< Dest address required */
   E_MYOS_EDQUOT           = E_MYOS_ERROR | 0x208, /**< Reserved */
   E_MYOS_EHOSTUNREACH	   = E_MYOS_ERROR | 0x209, /**< Host is unreachable */
   E_MYOS_EIDRM            = E_MYOS_ERROR | 0x20a, /**< Identifier removed */
   E_MYOS_EILSEQ           = E_MYOS_ERROR | 0x20b, /**< Illegal byte sequence */
   E_MYOS_EISCONN          = E_MYOS_ERROR | 0x20c, /**< Connection in progress */
   E_MYOS_ELOOP            = E_MYOS_ERROR | 0x20d, /**< Too many symbolic links */
   E_MYOS_EMULTIHOP        = E_MYOS_ERROR | 0x20e, /**< Reserved */
   E_MYOS_ENETDOWN         = E_MYOS_ERROR | 0x20f, /**< Network is down */
   E_MYOS_ENETUNREACH	   = E_MYOS_ERROR | 0x210, /**< Network unreachable */
   E_MYOS_ENOBUFS          = E_MYOS_ERROR | 0x211, /**< No buffer space available */
   E_MYOS_ENODATA          = E_MYOS_ERROR | 0x212, /**< No message is available */
   E_MYOS_ENOLINK          = E_MYOS_ERROR | 0x213, /**< Reserved */
   E_MYOS_ENOMSG           = E_MYOS_ERROR | 0x214, /**< No message of desired type */
   E_MYOS_ENOPROTOOPT	   = E_MYOS_ERROR | 0x215, /**< Protocol not available */
   E_MYOS_ENOSR            = E_MYOS_ERROR | 0x216, /**< No STREAM resources */
   E_MYOS_ENOSTR           = E_MYOS_ERROR | 0x217, /**< Not a STREAM */
   E_MYOS_ENOTCONN         = E_MYOS_ERROR | 0x218, /**< Socket not connected */
   E_MYOS_ENOTSOCK         = E_MYOS_ERROR | 0x219, /**< Not a socket */
   E_MYOS_EOPNOTSUPP       = E_MYOS_ERROR | 0x21a, /**< Op not supported on socket */
   E_MYOS_EOVERFLOW        = E_MYOS_ERROR | 0x21b, /**< Value too large */
   E_MYOS_EPROTO           = E_MYOS_ERROR | 0x21c, /**< Protocol error */
   E_MYOS_EPROTONOSUPPORT  = E_MYOS_ERROR | 0x21d, /**< Protocol not supported */
   E_MYOS_EPROTOTYPE       = E_MYOS_ERROR | 0x21e, /**< Socket type not supported */
   E_MYOS_ESTALE           = E_MYOS_ERROR | 0x21f, /**< Reserved */
   E_MYOS_ETIME		       = E_MYOS_ERROR | 0x220, /**< Stream ioctl timeout */
   E_MYOS_ETXTBSY		   = E_MYOS_ERROR | 0x221, /**< Text file busy */
   E_MYOS_EWOULDBLOCK	   = E_MYOS_ERROR | 0x222, /**< Operation would block */

   // My own additions
   E_MYOS_BAD_TOKENS    = E_MYOS_ERROR | 0x10000,
   E_MYOS_E_REGFAILED   = E_MYOS_ERROR | 0x10001,  ///> Interface registration failed
   E_MYOS_E_MISREQINTF  = E_MYOS_ERROR | 0x10002,  ///> Missing required interface
   E_MYOS_E_NOTINIT     = E_MYOS_ERROR | 0x10003,  ///> Not initialized

   // Success codes
   E_MYOS_REUSED           = 0x0001,
   E_MYOS_OK_FINISHED      = 0x0002,   // (Component code) OK & deinitialize me
};

}  // namespace

#endif
