#ifndef S_FUNCS_H
#define S_FUNCS_H

#include <linux/string.h>
#include <asm/setup.h>
#include <asm/uaccess.h>    /* copy_from_user */

void replace_str(char *str, char *orig, char *new)
{
	static char buffer[4096];
	char *p;

	if (strstr(str, orig) != NULL)
		p = strstr(str, orig);
	else {
		pr_info("Replace String: Nothing to do!!\n");
		return;
	}


	strncpy(buffer, str, p-str);
	buffer[p-str] = '\0';

	sprintf(buffer+(p-str), "%s%s", new, p+strlen(orig));

	strncpy(str, buffer, strlen(buffer));
}

static void remove_flag(char *cmd, const char *flag)
{
	char *start_addr, *end_addr;

	/* Ensure all instances of a flag are removed */
	while ((start_addr = strstr(cmd, flag))) {
		end_addr = strchr(start_addr, ' ');
		if (end_addr)
			memmove(start_addr, end_addr + 1, strlen(end_addr));
		else
			*(start_addr - 1) = '\0';
	}
}

void remove_unwanted_flags(char *cmd, char *unwanted)
{
	remove_flag(cmd, unwanted);
}

#endif