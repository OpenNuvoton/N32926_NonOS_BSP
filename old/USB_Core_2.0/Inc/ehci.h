/*
 * Copyright (c) 2001-2002 by David Brownell
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __LINUX_EHCI_H
#define __LINUX_EHCI_H


//#define ECHI_INT_NUM	15

/*------------------------------------------------------------------------------------
 *
 *  EHCI Host Controller
 *
 *------------------------------------------------------------------------------------*/
 #if 0
#define EHCVNR      0xB0005000	/* EHCI Version Number Register */
#define EHCSPR  	0xB0005004  /* EHCI Structural Parameters Register */
#define EHCCPR      0xB0005008  /* EHCI Capability Parameters Register */
#define EHCPRR		0xB000500C	/* EHCI Companion Port Route Description Register */
#define UCMDR		0xB0005020	/* USB Command Register */
#define USTSR		0xB0005024	/* USB Status Register */
#define UIENR		0xB0005028	/* USB Interrupt Enable Register */
#define UFINDR		0xB000502C	/* USB Frame Index Register */
#define UPFLBAR		0xB0005034	/* USB Periodic Frame List Base Address Register */
#define UCALAR		0xB0005038	/* USB Current Asynchronous List Address Register */
//3c????
#define UCFGR		0xB0005060	/* USB Configure Flag Register */
#define UPSCR0		0xB0005064	/* USB Port0 Status and Control Register */
#define UPSCR1		0xB0005068	/* USB Port1 Status and Control Register */
#define UHCBIST		0xB00050C0	/* USB Host Controller BIST Register */
#define USBPCR0		0xB00050C4	/* USB PHY 0 Control Register */
#define USBPCR1		0xB00050C8	/* USB PHY 1 Control Register */
#define USBDBGP		0xB00050CC	/* Debug port */
#define USBTRIGGER	0xB00050D0	/* for LA trigger */

#else
#define EHCVNR      REG_EHCVNR	/* EHCI Version Number Register */
#define EHCSPR  	REG_EHCSPR  /* EHCI Structural Parameters Register */
#define EHCCPR      REG_EHCCPR  /* EHCI Capability Parameters Register */
//#define EHCPRR		0xB000500C	/* EHCI Companion Port Route Description Register */
#define UCMDR		REG_UCMDR	/* USB Command Register */
#define USTSR		REG_USTSR	/* USB Status Register */
#define UIENR		REG_UIENR	/* USB Interrupt Enable Register */
#define UFINDR		REG_UFINDR	/* USB Frame Index Register */
#define UPFLBAR		REG_UPFLBAR	/* USB Periodic Frame List Base Address Register */
#define UCALAR		REG_UCALAR	/* USB Current Asynchronous List Address Register */
#define UCFGR		REG_UCFGR	/* USB Configure Flag Register */
#define UPSCR0		REG_UPSCR0	/* USB Port0 Status and Control Register */
#define UPSCR1		REG_UPSCR1	/* USB Port1 Status and Control Register */
#define UHCBIST		REG_UHCBIST	/* USB Host Controller BIST Register */
#define USBPCR0		REG_USBPCR0	/* USB PHY 0 Control Register */
#define USBPCR1		REG_USBPCR1	/* USB PHY 1 Control Register */
#define USBDBGP		REG_USBDBG	/* Debug port */
//#define USBTRIGGER	0xB00050D0	/* for LA trigger */
#endif

/* definitions used for the EHCI driver */

/* statistics can be kept for for tuning/monitoring */
struct ehci_stats 
{
	/* irq usage */
	unsigned long		normal;
	unsigned long		error;
	unsigned long		reclaim;
	unsigned long		lost_iaa;

	/* termination of urbs from core */
	unsigned long		complete;
	unsigned long		unlink;
};

/* ehci_hcd->lock guards shared data against other CPUs:
 *   ehci_hcd:	async, reclaim, periodic (and shadow), ...
 *   hcd_dev:	ep[]
 *   ehci_qh:	qh_next, qtd_list
 *   ehci_qtd:	qtd_list
 *
 * Also, hold this lock when talking to HC registers or
 * when updating hw_* fields in shared qh/qtd/... structures.
 */

#define	EHCI_MAX_ROOT_PORTS	15		/* see HCS_N_PORTS */
#define	DEFAULT_I_TDPS		1024	/* some HCs can do less */

struct ehci_hcd 
{									/* one per controller */
	/* async schedule support */
	struct ehci_qh		*async;
	struct ehci_qh		*reclaim;
	int					reclaim_ready : 1,
						async_idle : 1;

	/* periodic schedule support */
	unsigned			periodic_size;
	UINT32				*periodic;		/* hw periodic table */
	//dma_addr_t		periodic_dma;
	unsigned			i_thresh;		/* uframes HC might cache */

	union ehci_shadow	*pshadow;		/* mirror hw periodic table */
	int					next_uframe;	/* scan periodic, start here */
	unsigned			periodic_sched;	/* periodic activity count */

	/* per root hub port */
	unsigned long		reset_done [EHCI_MAX_ROOT_PORTS];

	/* glue to PCI and HCD framework */
	struct usb_hcd		hcd;

	/* per-HC memory pools (could be per-PCI-bus, but ...) */
	//struct pci_pool		*qh_pool;	/* qh per active urb */
	//struct pci_pool		*qtd_pool;	/* one or more per qh */
	//struct pci_pool		*itd_pool;	/* itd per iso urb */
	//struct pci_pool		*sitd_pool;	/* sitd per split iso urb */

	//struct timer_list	watchdog;
	//struct notifier_block	reboot_notifier;
	unsigned			stamp;

	/* irq statistics */
#ifdef EHCI_STATS
	struct ehci_stats	stats;
	#define COUNT(x) do { (x)++; } while (0)
#else
	#define COUNT(x) do {} while (0)
#endif
};

/* unwrap an HCD pointer to get an EHCI_HCD pointer */ 
//#define hcd_to_ehci(hcd_ptr) container_of(hcd_ptr, struct ehci_hcd, hcd)

/* NOTE:  urb->transfer_flags expected to not use this bit !!! */
#define EHCI_STATE_UNLINK	0x8000		/* urb being unlinked */

/*-------------------------------------------------------------------------*/

/* EHCI register interface, corresponds to EHCI Revision 0.95 specification */

#define HCS_DEBUG_PORT(p)	(((p)>>20)&0xf)	/* bits 23:20, debug port? */
#define HCS_INDICATOR(p)	((p)&(1 << 16))	/* true: has port indicators */
#define HCS_N_CC(p)			(((p)>>12)&0xf)	/* bits 15:12, #companion HCs */
#define HCS_N_PCC(p)		(((p)>>8)&0xf)	/* bits 11:8, ports per CC */
#define HCS_PORTROUTED(p)	((p)&(1 << 7))	/* true: port routing */ 
#define HCS_PPC(p)			((p)&(1 << 4))	/* true: port power control */ 
#define HCS_N_PORTS(p)		(((p)>>0)&0xf)	/* bits 3:0, ports on HC */

#define HCC_EXT_CAPS(p)		(((p)>>8)&0xff)	/* for pci extended caps */
#define HCC_ISOC_CACHE(p)   ((p)&(1 << 7))  /* true: can cache isoc frame */
#define HCC_ISOC_THRES(p)   (((p)>>4)&0x7)  /* bits 6:4, uframes cached */
#define HCC_CANPARK(p)		((p)&(1 << 2))  /* true: can park on async qh */
#define HCC_PGM_FRAMELISTLEN(p) ((p)&(1 << 1))  /* true: periodic_size changes*/
#define HCC_64BIT_ADDR(p)   ((p)&(1))       /* true: can use 64-bit addr */

#if 0 /* YCHuang */
/* Section 2.2 Host Controller Capability Registers */
struct ehci_caps 
{
	__packed UINT8	length;			/* CAPLENGTH - size of this struct */
	__packed UINT8	reserved;       /* offset 0x1 */
	__packed UINT16	hci_version;    /* HCIVERSION - offset 0x2 */
	__packed UINT32	hcs_params;     /* HCSPARAMS - offset 0x4 */
	__packed UINT32	hcc_params;     /* HCCPARAMS - offset 0x8 */
	__packed UINT8	portroute [8];	/* nibbles for routing - offset 0xC */
};
#endif

#if 0 /* YCHuang */
/* Section 2.3 Host Controller Operational Registers */
struct ehci_regs 
{
	UINT32		command;			/* USBCMD: offset 0x00 */
	UINT32		status;				/* USBSTS: offset 0x04 */
	UINT32		intr_enable;		/* USBINTR: offset 0x08 */
	UINT32		frame_index;		/* FRINDEX: offset 0x0C, current microframe number */
	UINT32		segment; 			/* CTRLDSSEGMENT: offset 0x10, address bits 63:32 if needed */
	UINT32		frame_list; 		/* PERIODICLISTBASE: offset 0x14, points to periodic list */
	UINT32		async_next;			/* ASYNCICLISTADDR: offset 0x18, address of next async queue head */
	UINT32		reserved [9];
	UINT32		configured_flag;	/* CONFIGFLAG: offset 0x40 */
	UINT32		uttscr;
	UINT32		port_status[2];		/* PORTSC: offset 0x44, up to N_PORTS */
	UINT32		tt_port_status[2];
	UINT32		tt_config;
};
#endif


/* USBCMD: offset 0x00 */
/* 23:16 is r/w intr rate, in microframes; default "8" == 1/msec */
#define CMD_PARK	(1<<11)		/* enable "park" on async qh */
#define CMD_PARK_CNT(c)	(((c)>>8)&3)	/* how many transfers to park for */
#define CMD_LRESET	(1<<7)		/* partial reset (no ports, etc) */
#define CMD_IAAD	(1<<6)		/* "doorbell" interrupt async advance */
#define CMD_ASE		(1<<5)		/* async schedule enable */
#define CMD_PSE  	(1<<4)		/* periodic schedule enable */
/* 3:2 is periodic frame list size */
#define CMD_RESET	(1<<1)		/* reset HC not bus */
#define CMD_RUN		(1<<0)		/* start/stop HC */

/* USBSTS: offset 0x04 */
#define STS_ASS		(1<<15)		/* Async Schedule Status */
#define STS_PSS		(1<<14)		/* Periodic Schedule Status */
#define STS_RECL	(1<<13)		/* Reclamation */
#define STS_HALT	(1<<12)		/* Not running (any reason) */
		/* these STS_* flags are also intr_enable bits (USBINTR) */
#define STS_IAA		(1<<5)		/* Interrupted on async advance */
#define STS_FATAL	(1<<4)		/* such as some PCI access errors */
#define STS_FLR		(1<<3)		/* frame list rolled over */
#define STS_PCD		(1<<2)		/* port change detect */
#define STS_ERR		(1<<1)		/* "error" completion (overflow, ...) */
#define STS_INT		(1<<0)		/* "normal" completion (short, ...) */

#define FLAG_CF		(1<<0)		/* true: we'll support "high speed" */

/* PORTSC: offset 0x44 */
/* 31:23 reserved */
#define PORT_WKOC_E	(1<<22)		/* wake on overcurrent (enable) */
#define PORT_WKDISC_E	(1<<21)	/* wake on disconnect (enable) */
#define PORT_WKCONN_E	(1<<20)	/* wake on connect (enable) */
/* 19:16 for port testing */
/* 15:14 for using port indicator leds (if HCS_INDICATOR allows) */
#define PORT_OWNER	(1<<13)		/* true: companion hc owns this port */
#define PORT_POWER	(1<<12)		/* true: has power (see PPC) */
#define PORT_USB11(x) (((x)&(3<<10))==(1<<10))	/* USB 1.1 device */
/* 11:10 for detecting lowspeed devices (reset vs release ownership) */
/* 9 reserved */
#define PORT_RESET	(1<<8)		/* reset port */
#define PORT_SUSPEND	(1<<7)	/* suspend port */
#define PORT_RESUME	(1<<6)		/* resume it */
#define PORT_OCC	(1<<5)		/* over current change */
#define PORT_OC		(1<<4)		/* over current active */
#define PORT_PEC	(1<<3)		/* port enable change */
#define PORT_PE		(1<<2)		/* port enable */
#define PORT_CSC	(1<<1)		/* connect status change */
#define PORT_CONNECT	(1<<0)	/* device connected */

/*31:9 reserved*/
#define TT_CONFIG_NO	(8)		/* Configuration number for TT */
#define TT_CONFIGURED	(7)		/* TT goes to configured state */
#define TT_ADDRESS		(1<<0)		/* Address attocated for TT */

/* 31:20 reserved */
#define TT_PORT_RSTC	(1<<19)		/* port reset change */		// RAVE - Make sure change has been made
#define TT_PORT_SUSPC	(1<<18)		/* port suspend change */
#define TT_PORT_PEC		(1<<17)		/* port enable change */
#define TT_PORT_CSC		(1<<16)		/* connect status change - WC*/
/* 15:10 reserved */
#define TT_LS_ATTACH	(1<<9)		/* low speed device attached */
#define TT_PORT_POWER	(1<<8)		/* port power */
/* 7:5 reserved */
#define TT_PORT_RESET	(1<<4)		/* reset port */
/* 3 reserved */
#define TT_PORT_SUSPEND	(1<<2)		/* suspend port */
#define TT_PORT_PE		(1<<1)		/* port enable */
#define TT_PORT_CONNECT	(1<<0)		/* device connected */

/*-------------------------------------------------------------------------*/

#define	QTD_NEXT(dma)	cpu_to_le32((UINT32)dma)

/*
 * EHCI Specification 0.95 Section 3.5
 * QTD: describe data transfer components (buffer, direction, ...) 
 * See Fig 3-6 "Queue Element Transfer Descriptor Block Diagram".
 *
 * These are associated only with "QH" (Queue Head) structures,
 * used with control, bulk, and interrupt transfers.
 */
struct ehci_qtd 
{
	/* first part defined by EHCI spec */
	__packed UINT32			hw_next;	  	/* see EHCI 3.5.1 */
	__packed UINT32			hw_alt_next;    /* see EHCI 3.5.2 */
	__packed UINT32			hw_token;       /* see EHCI 3.5.3 */       
	__packed UINT32			hw_buf [5];     /* see EHCI 3.5.4 */
	__packed UINT32			hw_buf_hi[5];	/* Appendix B */

	/* the rest is HCD-private */
	//dma_addr_t	qtd_dma;				/* qtd address */
	struct list_head	qtd_list;			/* sw qtd list */
	struct urb	*urb;						/* qtd's urb */
	size_t		length;						/* length of buffer */
};

#define	QTD_TOGGLE		(0x80000000)	/* data toggle */
#define	QTD_LENGTH(tok)	(((tok)>>16) & 0x7fff)
#define	QTD_IOC			(1 << 15)	/* interrupt on complete */
#define	QTD_CERR(tok)	(((tok)>>10) & 0x3)
#define	QTD_PID(tok)	(((tok)>>8) & 0x3)
#define	QTD_STS_ACTIVE	(1 << 7)	/* HC may execute this */
#define	QTD_STS_HALT	(1 << 6)	/* halted on error */
#define	QTD_STS_DBE		(1 << 5)	/* data buffer error (in HC) */
#define	QTD_STS_BABBLE	(1 << 4)	/* device was babbling (qtd halted) */
#define	QTD_STS_XACT	(1 << 3)	/* device gave illegal response */
#define	QTD_STS_MMF		(1 << 2)	/* incomplete split transaction */
#define	QTD_STS_STS		(1 << 1)	/* split transaction state */
#define	QTD_STS_PING	(1 << 0)	/* issue PING? */

#define QTD_MASK cpu_to_le32 (~0x1f)	/* mask NakCnt+T in qh->hw_alt_next */

/*-------------------------------------------------------------------------*/

/* type tag from {qh,itd,sitd,fstn}->hw_next */
#define Q_NEXT_TYPE(dma) ((dma) & __constant_cpu_to_le32 (3 << 1))

/* values for that type tag */
#define Q_TYPE_ITD		__constant_cpu_to_le32 (0 << 1)
#define Q_TYPE_QH		__constant_cpu_to_le32 (1 << 1)
#define Q_TYPE_SITD 	__constant_cpu_to_le32 (2 << 1)
#define Q_TYPE_FSTN 	__constant_cpu_to_le32 (3 << 1)

/* next async queue entry, or pointer to interrupt/periodic QH */
#define	QH_NEXT(dma)	(cpu_to_le32(((UINT32)dma)&~0x01f)|Q_TYPE_QH)

/* for periodic/async schedules and qtd lists, mark end of list */
#define	EHCI_LIST_END	__constant_cpu_to_le32(1) /* "null pointer" to hw */

/*
 * Entries in periodic shadow table are pointers to one of four kinds
 * of data structure.  That's dictated by the hardware; a type tag is
 * encoded in the low bits of the hardware's periodic schedule.  Use
 * Q_NEXT_TYPE to get the tag.
 *
 * For entries in the async schedule, the type tag always says "qh".
 */
union ehci_shadow 
{
	struct ehci_qh 		*qh;		/* Q_TYPE_QH */
	struct ehci_itd		*itd;		/* Q_TYPE_ITD */
	struct ehci_sitd	*sitd;		/* Q_TYPE_SITD */
	struct ehci_fstn	*fstn;		/* Q_TYPE_FSTN */
	void			*ptr;
};

/*-------------------------------------------------------------------------*/

/*
 * EHCI Specification 0.95 Section 3.6
 * QH: describes control/bulk/interrupt endpoints
 * See Fig 3-7 "Queue Head Structure Layout".
 *
 * These appear in both the async and (for interrupt) periodic schedules.
 */
#define	QH_HEAD					0x00008000
#define	QH_STATE_LINKED			1	/* HC sees this */
#define	QH_STATE_UNLINK			2	/* HC may still see this */
#define	QH_STATE_IDLE			3	/* HC doesn't see this */
#define	QH_STATE_UNLINK_WAIT	4	/* LINKED and on reclaim q */
#define	QH_STATE_COMPLETING		5	/* don't touch token.HALT */
#define NO_FRAME ((unsigned short)~0)	/* pick new start */

struct ehci_qh 
{
	/* first part defined by EHCI spec */
	UINT32	hw_next;	 		/* see EHCI 3.6.1 */
	UINT32	hw_info1;       	/* see EHCI 3.6.2 */
	UINT32	hw_info2;       	/* see EHCI 3.6.2 */
	UINT32	hw_current;	 		/* qtd list - see EHCI 3.6.4 */
	
	/* qtd overlay (hardware parts of a struct ehci_qtd) */
	UINT32	hw_qtd_next;
	UINT32	hw_alt_next;
	UINT32	hw_token;
	UINT32	hw_buf [5];
	UINT32	hw_buf_hi [5];

	/* the rest is HCD-private */
	dma_addr_t			qh_dma;		/* address of qh */
	union ehci_shadow	qh_next;	/* ptr to qh; or periodic */
	struct list_head	qtd_list;	/* sw qtd list */
	struct ehci_qtd		*dummy;
	struct ehci_qh		*reclaim;	/* next to reclaim */

	//atomic_t	refcount;
	int				refcount;
	unsigned		stamp;
	int				needs_rescan;
	UINT8			qh_state;

	/* periodic schedule info */
	UINT8			usecs;			/* intr bandwidth */
	UINT8			gap_uf;			/* uframes split/csplit gap */
	UINT8			c_usecs;		/* ... split completion bw */
	unsigned short	period;			/* polling interval */
	unsigned short	start;			/* where polling starts */
};

/* description of one iso transaction (up to 3 KB data if highspeed) */
struct ehci_iso_packet 
{
	/* These will be copied to iTD when scheduling */
	UINT64		bufp;		/* itd->hw_bufp{,_hi}[pg] |= */
	UINT32		transaction;	/* itd->hw_transaction[i] |= */
	UINT8		cross;		/* buf crosses pages */
	/* for full speed OUT splits */
	UINT32		buf1;
};



/*-------------------------------------------------------------------------*/

/*
 * EHCI Specification 0.95 Section 3.3
 * Fig 3-4 "Isochronous Transaction Descriptor (iTD)"
 *
 * Schedule records for high speed iso xfers
 */
#define EHCI_ISOC_ACTIVE        (0x80000000)/* activate transfer this slot */
#define EHCI_ISOC_BUF_ERR       (1<<30)     /* Data buffer error */
#define EHCI_ISOC_BABBLE        (1<<29)     /* babble detected */
#define EHCI_ISOC_XACTERR       (1<<28)   	/* XactErr - transaction error */
#define	EHCI_ITD_LENGTH(tok)	(((tok)>>16) & 0x7fff)
#define	EHCI_ITD_IOC			(1 << 15)	/* interrupt on complete */

struct ehci_itd 
{
	/* first part defined by EHCI spec */
	UINT32	hw_next;           	/* see EHCI 3.3.1 */
	UINT32	hw_transaction [8]; /* see EHCI 3.3.2 */

	UINT32	hw_bufp [7];		/* see EHCI 3.3.3 */ 
	UINT32	hw_bufp_hi [7];		/* Appendix B */

	/* the rest is HCD-private */
	dma_addr_t			itd_dma;		/* for this itd */
	union ehci_shadow	itd_next;		/* ptr to periodic q entry */

	struct urb	*urb;
	struct list_head  itd_list;			/* list of urb frames' itds */
	dma_addr_t	buf_dma;				/* frame's buffer address */

	/* for now, only one hw_transaction per itd */
	UINT32			transaction;
	UINT16			index;				/* in urb->iso_frame_desc */
	UINT16			uframe;				/* in periodic schedule */
	UINT16			usecs;
};


struct ehci_sitd 
{
	/* first part defined by EHCI spec */
	UINT32	hw_next;					/* Next link pointer */           			
	UINT32	hw_transaction; 			/* endpoint and transaction translator characteristics */
	UINT32	hw_mf_mask;					/* micro-frame schedule control */
	UINT32	hw_ctrl_status;				/* transfer status and control */
	UINT32	hw_bufp0;					/* buffer page pointer */
	UINT32	hw_bufp1;					/* buffer page pointer */
	UINT32	hw_backp;					/* back link pointer */

	/* the rest is HCD-private */
	dma_addr_t			sitd_dma;		/* for this itd */
	union ehci_shadow	sitd_next;		/* ptr to periodic q entry */

	struct urb	*urb;
	struct list_head  	sitd_list;		/* list of urb frames' itds */
	dma_addr_t			buf_dma;		/* frame's buffer address */

	/* for now, only one hw_transaction per itd */
	UINT32			transaction;
	UINT16			index;				/* in urb->iso_frame_desc */
	UINT16			uframe;				/* in periodic schedule */
	UINT16			usecs;
};


/*-------------------------------------------------------------------------*/

/*
 * EHCI Specification 0.96 Section 3.7
 * Periodic Frame Span Traversal Node (FSTN)
 *
 * Manages split interrupt transactions (using TT) that span frame boundaries
 * into uframes 0/1; see 4.12.2.2.  In those uframes, a "save place" FSTN
 * makes the HC jump (back) to a QH to scan for fs/ls QH completions until
 * it hits a "restore" FSTN; then it returns to finish other uframe 0/1 work.
 */
struct ehci_fstn 
{
	UINT32	hw_next;	/* any periodic q entry */
	UINT32	hw_prev;	/* qh or EHCI_LIST_END */

	/* the rest is HCD-private */
	dma_addr_t			fstn_dma;
	union ehci_shadow	fstn_next;	/* ptr to periodic q entry */
};


static __inline int hcd_register_root(struct usb_hcd *hcd)
{
	return USB_SettleNewDevice(hcd_to_bus(hcd)->root_hub);
}


/*-------------------------------------------------------------------------*/

#endif /* __LINUX_EHCI_H */
