/* favc_module.h */
#ifndef _FAVCMODULE_H_
#define _FAVCMODULE_H_


#define SUPPORT_DECODER_DEFAULT_YES 1
#define SUPPORT_ENCODER_DEFAULT_YES 1


int favc_encoder_open(void);
int favc_encoder_mmap(void);
int favc_encoder_release(void);
int favc_encoder_release_ex(int dev);

int favc_decoder_open(void);
int favc_decoder_mmap(void);
int favc_decoder_release(void);
int favc_decoder_release_ex(int dev);
int init_favc(void);
int favc_decoder_ioctl(void* handle, unsigned int cmd, void* arg);
int favc_encoder_ioctl(void* handle, unsigned int cmd, void* arg);


unsigned int TimeOutCheck(unsigned int tick);

#if 0
#define F_DEBUG(fmt, args...) printk(KERN_ALERT "FAVC: " fmt, ## args)
#else
#define F_DEBUG(a...)
#endif

#endif
