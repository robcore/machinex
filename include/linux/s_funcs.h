#ifndef S_FUNCS_H
#define S_FUNCS_H

#include <linux/string.h>

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
#endif