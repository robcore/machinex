#!/bin/bash
cd drivers
echo 'beginning conversions'
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/VM_NODUMP/VM_DONTDUMP/' {} \;
echo 'finito'
