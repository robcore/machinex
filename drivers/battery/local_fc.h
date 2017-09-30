#ifndef _LOCAL_FC_H_
#define _LOCAL_FC_H_
#define STOCK_FULL_CONDITION_EUR 0	/* default */
#define STOCK_FULL_CONDITION_NA 1
#define FORCE_FULL_CONDITION_CUSTOM 2

#define FULL_PERCENTAGE_EUR 93	/* default */
#define FULL_PERCENTAGE_NA 97

extern unsigned int force_full_condition_soc;
extern unsigned int full_condition_percentage;
extern unsigned int real_full_condition_vcell(void);
#endif