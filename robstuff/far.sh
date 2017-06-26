#!/bin/bash

echo 'beginning conversions'
cd ~/machinex/drivers
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_sector/bio->bi_iter.bi_sector/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_size/bio->bi_iter.bi_size/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_idx/bio->bi_iter.bi_idx/' {} \;
cd ~/machinex/block
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_sector/bio->bi_iter.bi_sector/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_size/bio->bi_iter.bi_size/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_idx/bio->bi_iter.bi_idx/' {} \;
cd ~/machinex/fs
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_sector/bio->bi_iter.bi_sector/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_size/bio->bi_iter.bi_size/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_idx/bio->bi_iter.bi_idx/' {} \;
cd ~/machinex/include
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_sector/bio->bi_iter.bi_sector/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_size/bio->bi_iter.bi_size/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_idx/bio->bi_iter.bi_idx/' {} \;
cd ~/machinex/mm
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_sector/bio->bi_iter.bi_sector/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_size/bio->bi_iter.bi_size/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_idx/bio->bi_iter.bi_idx/' {} \;
cd ~/machinex/kernel
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_sector/bio->bi_iter.bi_sector/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_size/bio->bi_iter.bi_size/' {} \;
find . -type f \( -iname \*.h \
				-o -iname \*.c \) \
					| parallel sed -i 's/bio->bi_idx/bio->bi_iter.bi_idx/' {} \;
echo 'finito'
