cmd_drivers/scsi/built-in.o :=  /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-ld -EL    -r -o drivers/scsi/built-in.o drivers/scsi/scsi_mod.o drivers/scsi/scsi_tgt.o drivers/scsi/arm/built-in.o drivers/scsi/sd_mod.o drivers/scsi/sg.o drivers/scsi/ch.o 
