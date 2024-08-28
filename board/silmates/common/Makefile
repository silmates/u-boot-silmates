# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.

MINIMAL=

ifdef CONFIG_SPL_BUILD
ifdef CONFIG_SPL_INIT_MINIMAL
MINIMAL=y
endif
endif

ifdef MINIMAL
# necessary to create built-in.o
obj- := __dummy__.o
else

obj-$(CONFIG_DEEP_SLEEP)		+= arm_sleep.o

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_SM_CFG_BLOCK) += sm-config.o
obj-$(CONFIG_SM_CFG_BLOCK) += sm-common.o
obj-$(CONFIG_SM_CFG_BLOCK) += sm-eeprom.o
obj-$(CONFIG_SM_CFG_BLOCK) += i2c_common.o
endif

#ifneq (,$(filter $(SOC), mx25 mx31 mx35 mx5 mx6 mx7 mx7ulp imx8 imx8m vf610 imx8ulp imx9))
obj-y				+= mmc.o
#endif

ifdef CONFIG_NXP_ESBC
obj-$(CONFIG_CMD_ESBC_VALIDATE) += fsl_validate.o cmd_esbc_validate.o
endif
obj-$(CONFIG_CHAIN_OF_TRUST) += fsl_chain_of_trust.o

endif
