#ifndef _OPT_H_
#define _OPT_H_

/** @name Enable/disable debug messages completely (LWIP_DBG_TYPES_ON)
 * @{
 */
/** flag for pr_debug to enable that debug message */
#define DBG_ON	0x80U
/** flag for pr_debug to disable that debug message */
#define DBG_OFF	0x00U

#define IPC_DEBUG       DBG_ON
#define SOCKET_DEBUG    DBG_ON
#define PBUF_DEBUG      DBG_ON
#define FS_DEBUG        DBG_ON
#define FILE_DEBUG      DBG_ON
#define SCHED_DEBUG     DBG_OFF
#define STACK_DEBUG     DBG_ON

#endif  /* _OPT_H_ */