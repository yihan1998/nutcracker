#ifndef _INIT_H_
#define _INIT_H_

#define __init
#define __exit

extern int __init flax_init(void);

int pistachio_lcore_init(int lid);
extern int __init pistachio_init(void);

extern int __init cacao_init(void);
extern int cacao_attach_and_run(const char * filepath);

#endif  /* _INIT_H_ */