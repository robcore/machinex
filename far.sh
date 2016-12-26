#!/bin/bash
cd drivers
echo 'beginning arch conversions'
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/VM_RESERVED/VM_DONTDUMP/' {} \;
echo 'finito'
