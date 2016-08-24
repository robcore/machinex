#
# Makefile for the kernel mobicore drivers
#
GUD_ROOT_FOLDER := drivers/gud
# add our modules to kernel.
obj-$(CONFIG_MOBICORE_API) += mcKernelApi.o
obj-$(CONFIG_MOBICORE_SUPPORT) += mcDrvModule.o

mcDrvModule-objs := mobicore_driver/logging.o \
                mobicore_driver/ops.o \
                mobicore_driver/mem.o \
                mobicore_driver/api.o \
                mobicore_driver/main.o \
                mobicore_driver/pm.o

mcKernelApi-objs := mobicore_kernelapi/main.o \
                mobicore_kernelapi/clientlib.o \
                mobicore_kernelapi/device.o \
                mobicore_kernelapi/session.o \
                mobicore_kernelapi/connection.o

# Release mode by default
ccflags-y := -DNDEBUG -include $(PWD)/$(GUD_ROOT_FOLDER)/mobicore_driver/build_tag.h
ccflags-y += -Wno-declaration-after-statement

ccflags-$(CONFIG_MOBICORE_DEBUG) += -DDEBUG
ccflags-$(CONFIG_MOBICORE_VERBOSE) += -DDEBUG_VERBOSE

# Choose one platform from the folder
MOBICORE_PLATFORM := $(shell (ls -1 $(PWD)/$(GUD_ROOT_FOLDER)/mobicore_driver/platforms | tail -1) )
# Use the available platform folder
ccflags-y += -I$(GUD_ROOT_FOLDER)/mobicore_driver/platforms/$(MOBICORE_PLATFORM)
# MobiCore Driver includes
ccflags-y += -I$(GUD_ROOT_FOLDER)/mobicore_driver/public
# MobiCore KernelApi required incldes
ccflags-y += -I$(GUD_ROOT_FOLDER)/mobicore_kernelapi/include
ccflags-y += -I$(GUD_ROOT_FOLDER)/mobicore_kernelapi/public
